#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gui_thread, CONFIG_APP_LOG_LEVEL);

void gui_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	const struct device *display_dev;
	lv_obj_t *hello_world_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev))
	{
		LOG_ERR("Display device not ready");
		return 0;
	}

	hello_world_label = lv_label_create(lv_scr_act());
	lv_label_set_text(hello_world_label, "Hello world!");
	lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1)
	{
		k_sleep(K_FOREVER);
	}
}
