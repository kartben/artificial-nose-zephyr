#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gui_thread, CONFIG_APP_LOG_LEVEL);

void gui_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev))
	{
		LOG_ERR("Display device not ready");
		return 0;
	}

	lv_label_set_text(hello_world_label, "Hello world!");
	lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1)
	{
		k_msleep(lv_task_handler());
	}
}
