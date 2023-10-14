/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#include "app_threads.h"

int main(void)
{
	// Create sensor thread
	k_thread_create(&sensor_acquisition_thread, sensor_acquisition_thread_stack,
			K_THREAD_STACK_SIZEOF(sensor_acquisition_thread_stack),
			sensor_acquisition_fn,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_thread_name_set(&sensor_acquisition_thread, "sensor_acquisition_thread");

	printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

	int i = 0;
	while (1)
	{
		k_sleep(K_FOREVER);
	}

	return 0;
}
