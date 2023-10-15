#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/ring_buffer.h>

#include "drivers/sensor/multichannel_gas_v2/grove_multichannel_gas_v2.h"

#define NUM_SAMPLES CONFIG_GAS_SENSOR_NUM_SAMPLES

struct sensor_value ring_buffer_data[NUM_SAMPLES];
struct ring_buf sensor_data_ringbuf;

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_thread, CONFIG_APP_LOG_LEVEL);

const struct device *gas_sensor = DEVICE_DT_GET_ONE(seeed_grove_multichannel_gas_v2);

void sensor_acquisition_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct sensor_value values[4];
	uint8_t buffer[NUM_SAMPLES];

	ring_buf_init(&sensor_data_ringbuf, NUM_SAMPLES, (uint8_t *)ring_buffer_data);

	if (!device_is_ready(gas_sensor))
	{
		printf("Gas sensor %s is not ready\n", gas_sensor->name);
		return 0;
	}

	while (1)
	{
		k_sleep(K_MSEC(1000 / CONFIG_GAS_SENSOR_SAMPLING_RATE)); // everything happens in the timer expiry handler

		if (sensor_sample_fetch(gas_sensor))
		{
			LOG_ERR("Failed to fetch sample\n");
		}

		if (sensor_sample_fetch(gas_sensor))
		{
			LOG_ERR("Failed to fetch sample\n");
		}

		sensor_channel_get(gas_sensor, SENSOR_CHAN_NO2, &values[0]);
		sensor_channel_get(gas_sensor, SENSOR_CHAN_CO, &values[1]);
		sensor_channel_get(gas_sensor, SENSOR_CHAN_C2H5OH, &values[2]);
		sensor_channel_get(gas_sensor, SENSOR_CHAN_VOC, &values[3]);

		// If buffer is full, remove the oldest sample
		if (ring_buf_space_get(&sensor_data_ringbuf) == 0)
		{
			ring_buf_get(&sensor_data_ringbuf, NULL, 4);
		}

		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[0].val1, 1);
		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[1].val1, 1);
		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[2].val1, 1);
		ring_buf_put(&sensor_data_ringbuf, (uint8_t *)&values[3].val1, 1);

		// ring_buf_peek(&sensor_data_ringbuf, buffer, NUM_SAMPLES);
		// for (int i = 0; i < NUM_SAMPLES; i++)
		// {
		// 	printk("%02d,", buffer[i]);
		// }
		// printk("\n");
	}
}
