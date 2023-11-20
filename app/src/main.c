/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/zbus/zbus.h>
#include <app_version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#include "app_threads.h"
#include "zbus_messages.h"

ZBUS_CHAN_DEFINE(inference_result_chan,       /* Name */
		 struct inference_result_msg, /* Message type */
		 NULL,                        /* Validator */
		 NULL,                        /* User data */
		 ZBUS_OBSERVERS_EMPTY,        /* observers */
		 ZBUS_MSG_INIT(0)             /* Initial value */
);

/*
 * The main thread is used to initialize and start all the threads that participate in the
 * application.
 *
 * - Sensor acquisition thread
 * - GUI thread
 * - Inference thread
 */
int main(void)
{
	/* Create sensor thread */
	k_thread_create(&sensor_acquisition_thread, sensor_acquisition_thread_stack,
			K_THREAD_STACK_SIZEOF(sensor_acquisition_thread_stack),
			sensor_acquisition_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_thread_name_set(&sensor_acquisition_thread, "Sensor Acquisition");

#ifdef CONFIG_DISPLAY
	/* Create GUI thread */
	k_thread_create(&gui_thread, gui_thread_stack, K_THREAD_STACK_SIZEOF(gui_thread_stack),
			gui_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	k_thread_name_set(&gui_thread, "GUI");
#endif

#ifdef CONFIG_APP_MQTT
	/* Create MQTT pub thread */
	k_thread_create(&mqtt_pub_thread, mqtt_pub_thread_stack,
			K_THREAD_STACK_SIZEOF(mqtt_pub_thread_stack), mqtt_pub_fn, NULL, NULL, NULL,
			K_PRIO_PREEMPT(20), 0, K_NO_WAIT);
	k_thread_name_set(&mqtt_pub_thread, "MQTT Publisher");
#endif

	/* Create inference thread */
	k_thread_create(&inference_thread, inference_thread_stack,
			K_THREAD_STACK_SIZEOF(inference_thread_stack), inference_fn, NULL, NULL,
			NULL, K_PRIO_PREEMPT(15), 0, K_NO_WAIT);
	k_thread_name_set(&inference_thread, "Inference (Edge Impulse)");

	printk("Artificial Nose - Zephyr powered %s\n", APP_VERSION_STRING);

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
