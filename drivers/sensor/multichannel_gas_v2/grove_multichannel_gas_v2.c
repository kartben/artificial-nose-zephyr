/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT seeed_grove_multichannel_gas_v2

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(GROVE_MULTICHANNEL_GAS_V2, CONFIG_SENSOR_LOG_LEVEL);

#include "drivers/sensor/multichannel_gas_v2/grove_multichannel_gas_v2.h"

struct grove_multichannel_gas_v2_data
{
	uint32_t voc;
	uint32_t co;
	uint32_t no2;
	uint32_t c2h5oh;
};

struct grove_multichannel_gas_v2_config
{
	struct i2c_dt_spec i2c;
};

static int grove_multichannel_gas_v2_read_gas_sensor(const struct i2c_dt_spec *i2c,
						     uint8_t cmd,
						     uint32_t *res)
{
	uint8_t buff[4];

	if (i2c_write_read_dt(i2c, &cmd, 1, buff, sizeof(buff)) < 0)
	{
		LOG_ERR("Failed to read gas sensor");
		return -EIO;
	}
	k_sleep(K_MSEC(2));

	*res = (uint32_t)sys_get_le32(buff);

	return 0;
}

static int grove_multichannel_gas_v2_sample_fetch(const struct device *dev,
						  enum sensor_channel chan)
{
	struct grove_multichannel_gas_v2_data *drv_data = dev->data;
	const struct grove_multichannel_gas_v2_config *config = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	// TODO handle errors
	grove_multichannel_gas_v2_read_gas_sensor(&config->i2c, GM_102B, &drv_data->no2);
	grove_multichannel_gas_v2_read_gas_sensor(&config->i2c, GM_302B, &drv_data->c2h5oh);
	grove_multichannel_gas_v2_read_gas_sensor(&config->i2c, GM_502B, &drv_data->voc);
	grove_multichannel_gas_v2_read_gas_sensor(&config->i2c, GM_702B, &drv_data->co);

	return 0;
}

static int grove_multichannel_gas_v2_channel_get(const struct device *dev,
						 enum sensor_channel chan,
						 struct sensor_value *val)
{
	struct grove_multichannel_gas_v2_data *drv_data = dev->data;
	val->val2 = 0;

	switch ((uint32_t)chan)
	{
	case SENSOR_CHAN_VOC:
		val->val1 = drv_data->voc;
		break;
	case SENSOR_CHAN_CO:
		val->val1 = drv_data->co;
		break;
	case SENSOR_CHAN_NO2:
		val->val1 = drv_data->no2;
		break;
	case SENSOR_CHAN_C2H5OH:
		val->val1 = drv_data->c2h5oh;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api grove_multichannel_gas_v2_api = {
    .sample_fetch = &grove_multichannel_gas_v2_sample_fetch,
    .channel_get = &grove_multichannel_gas_v2_channel_get,
};

static int grove_multichannel_gas_v2_init(const struct device *dev)
{
	const struct grove_multichannel_gas_v2_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c))
	{
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	if (i2c_reg_write_byte_dt(&config->i2c, 0, PREHEAT_CMD) < 0)
	{
		LOG_ERR("Failed to preheat");
		return -EIO;
	}
	k_sleep(K_MSEC(1));

	return 0;
}

#define GROVE_MULTICHANNEL_GAS_V2_INIT(i)                                   \
	static struct grove_multichannel_gas_v2_data data_##i;              \
                                                                            \
	static const struct grove_multichannel_gas_v2_config config_##i = { \
	    .i2c = I2C_DT_SPEC_INST_GET(i),                                 \
	};                                                                  \
                                                                            \
	DEVICE_DT_INST_DEFINE(i, grove_multichannel_gas_v2_init, NULL,      \
			      &data_##i,                                    \
			      &config_##i, POST_KERNEL,                     \
			      CONFIG_SENSOR_INIT_PRIORITY, &grove_multichannel_gas_v2_api);

DT_INST_FOREACH_STATUS_OKAY(GROVE_MULTICHANNEL_GAS_V2_INIT)
