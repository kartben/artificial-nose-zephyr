/*
 * Copyright (c) 2023 Benjamin Cabé <benjamin@zephyrproject.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the Seeed Studio Grove Multichannel Gas Sensor V2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_GROVE_MULTIGAS_V2_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_GROVE_MULTIGAS_V2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_channel_grove_multigas_v2 {
	SENSOR_CHAN_CO = SENSOR_CHAN_PRIV_START, /* Carbon Monoxide */
	SENSOR_CHAN_NO2,			 /* Nitrogen Dioxide */
	SENSOR_CHAN_C2H5OH,			 /* Ethanol */
};

#define GM_102B		0x01	/* NO2 */
#define GM_302B		0x03	/* Ethanol */
#define GM_502B		0x05	/* VOC */
#define GM_702B		0x07	/* CO */
#define PREHEAT_CMD	0xFE

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_GROVE_MULTIGAS_V2_H_ */