#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "gui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gui_thread, CONFIG_APP_LOG_LEVEL);

/*
 * Entry point for the GUI thread.
 *
 * This thread is responsible for creating the GUI and running the LVGL event loop.
 */
void gui_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready");
		while (1) {
		}
	}

	create_sensor_chart(lv_scr_act());

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}
