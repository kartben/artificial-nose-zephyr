#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/zbus/zbus.h>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"
#include <cfloat>

#include "sensor_acquisition.h"
#include "zbus_messages.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(inference_thread, CONFIG_APP_LOG_LEVEL);

#ifdef __cplusplus
extern "C" {
#endif

extern struct k_sem sensor_data_ringbuf_sem;
extern struct ring_buf sensor_data_ringbuf;

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr)
{
	__ASSERT(offset == 0, "Only offset 0 is supported");

	k_sem_take(&sensor_data_ringbuf_sem, K_FOREVER);

	uint32_t buf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
	ring_buf_peek(&sensor_data_ringbuf, (uint8_t *)&buf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE * sizeof(uint32_t));
	k_sem_give(&sensor_data_ringbuf_sem);

	for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i++) {
		out_ptr[i] = (float)buf[i];
	}

	return 0;
}

ZBUS_CHAN_DECLARE(inference_result_chan);

/**
 * Entry point for the inference thread.
 *
 * This thread is responsible for running inference on the sensor data, using a model trained in
 * Edge Impulse.
 * The result of the inference are published on a zbus channel for other threads to
 * consume (ex. the GUI thread might want to display the results).
 */
void inference_fn(void *arg1, void *arg2, void *arg3)
{
	ei_impulse_result_t result = {0};

	while (1) {
		/* do not run inference until we have enough data */
		k_sem_take(&sensor_data_ringbuf_sem, K_FOREVER);
		if (ring_buf_space_get(&sensor_data_ringbuf) > 0) {
			k_sem_give(&sensor_data_ringbuf_sem);
			k_msleep(500);
			continue;
		}
		k_sem_give(&sensor_data_ringbuf_sem);

		signal_t features_signal;
		features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
		features_signal.get_data = &raw_feature_get_data;

		/* invoke the impulse */
		EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

		if (res != 0) {
			LOG_ERR("run_classifier returned: %d\n", res);
			continue;
		}

		LOG_DBG("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d "
			"ms.): \n",
			result.timing.dsp, result.timing.classification, result.timing.anomaly);
		for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
			LOG_DBG("    %s: %.5f\n", result.classification[ix].label,
				result.classification[ix].value);
		}
#if EI_CLASSIFIER_HAS_ANOMALY == 1
		LOG_DBG("    anomaly score: %.3f\n", result.anomaly);
#endif

		/* Figure out what the strongest prediction is */
		size_t best_prediction = 0;
		for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
			if (result.classification[ix].value >=
			    result.classification[best_prediction].value) {
				best_prediction = ix;
			}
		}

		/* Make inference result available via dedicated zbus channel */
		inference_result_msg inf;
		strncpy(inf.label, result.classification[best_prediction].label, sizeof(inf.label));
		inf.confidence = result.classification[best_prediction].value;
		inf.anomaly_score = result.anomaly;

		zbus_chan_pub(&inference_result_chan, &inf, K_MSEC(200));

		k_msleep(CONFIG_APP_INFERENCE_PERIOD);
	}
}

#ifdef __cplusplus
}
#endif
