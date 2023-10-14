#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/ring_buffer.h>

#define NUM_SAMPLES CONFIG_GAS_SENSOR_NUM_SAMPLES

struct sensor_value ring_buffer_data[NUM_SAMPLES];
struct ring_buf sensor_data_ringbuf;

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_thread, CONFIG_APP_LOG_LEVEL);

void timer_expiry_handler(struct k_timer *timer_id)
{
    ARG_UNUSED(timer_id);

    LOG_INF("sampling");
    static int k = 0;
    struct sensor_value value;
    uint8_t buffer[NUM_SAMPLES];

    value.val1 = k++;

    if (ring_buf_space_get(&sensor_data_ringbuf) == 0) {
        printk("Ring buffer full\n");
        ring_buf_get(&sensor_data_ringbuf, NULL, 1);
    }

    ring_buf_put(&sensor_data_ringbuf, (uint8_t*)&value.val1, 1);

    ring_buf_peek(&sensor_data_ringbuf, buffer, NUM_SAMPLES);
    // for (int i = 0; i < NUM_SAMPLES; i++) {
    //     printk("%02d,", buffer[i]);
    // }
    printk("\n");
}

K_TIMER_DEFINE(sensor_timer, timer_expiry_handler, NULL);

void sensor_acquisition_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    ring_buf_init(&sensor_data_ringbuf, NUM_SAMPLES, (uint8_t *)ring_buffer_data);

    // Start the timer to expire at the rate specified by CONFIG_GAS_SENSOR_SAMPLING_RATE.
    k_timer_start(&sensor_timer, K_MSEC(1000 / CONFIG_GAS_SENSOR_SAMPLING_RATE), K_MSEC(1000 / CONFIG_GAS_SENSOR_SAMPLING_RATE));

    while (1) {
		k_sleep(K_FOREVER); // everything happens in the timer expiry handler
    }
}

