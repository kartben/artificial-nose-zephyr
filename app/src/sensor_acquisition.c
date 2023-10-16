#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/ring_buffer.h>

#include "drivers/sensor/multichannel_gas_v2/grove_multichannel_gas_v2.h"
#include "sensor_acquisition.h"

#include "model-parameters/model_metadata.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_thread, CONFIG_APP_LOG_LEVEL);

const struct device *gas_sensor = DEVICE_DT_GET_ONE(seeed_grove_multichannel_gas_v2);

RING_BUF_DECLARE(sensor_data_ringbuf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
K_SEM_DEFINE(sensor_data_ringbuf_sem, 1, 1);

/**
 * Entry point for the sensor acquisition thread.
 *
 * This thread is responsible for fetching sensor data from the gas sensor and storing it in a
 * ring buffer.
 */
void sensor_acquisition_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct sensor_value values[4];

	if (!device_is_ready(gas_sensor)) {
		LOG_ERR("Gas sensor %s is not ready\n", gas_sensor->name);
		while (1) {
		}
	}

	while (1) {
		k_sleep(K_MSEC(EI_CLASSIFIER_INTERVAL_MS));

		if (sensor_sample_fetch(gas_sensor)) {
			LOG_ERR("Failed to fetch sample\n");
		}

		sensor_channel_get(gas_sensor, SENSOR_CHAN_NO2, &values[0]);
		sensor_channel_get(gas_sensor, SENSOR_CHAN_CO, &values[1]);
		sensor_channel_get(gas_sensor, SENSOR_CHAN_C2H5OH, &values[2]);
		sensor_channel_get(gas_sensor, SENSOR_CHAN_VOC, &values[3]);

		k_sem_take(&sensor_data_ringbuf_sem, K_FOREVER);

		/* If buffer is full, remove the oldest set of samples */
		if (ring_buf_space_get(&sensor_data_ringbuf) == 0) {
			ring_buf_get(&sensor_data_ringbuf, NULL, 4);
		}

		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[0].val1, 1);
		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[1].val1, 1);
		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[2].val1, 1);
		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[3].val1, 1);

		k_sem_give(&sensor_data_ringbuf_sem);
	}
}
