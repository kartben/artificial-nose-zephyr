#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/ring_buffer.h>

#include "zbus_messages.h"
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

ZBUS_CHAN_DECLARE(inference_result_chan);

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)
#define DISPLAY_WIDTH  DT_PROP(DISPLAY_NODE, width)
#define DISPLAY_HEIGHT DT_PROP(DISPLAY_NODE, height)
#define IS_LARGE_SCREEN (DISPLAY_WIDTH > 240)

static lv_obj_t *chart;
static lv_chart_series_t *series[4];
static lv_obj_t *inference_result_label;
#if IS_LARGE_SCREEN
static lv_obj_t *inference_result_confidence_meter;
static lv_meter_indicator_t *inference_result_confidence_indic;
#endif

static lv_palette_t palette_colors[4] = {
	LV_PALETTE_RED,
	LV_PALETTE_GREEN,
	LV_PALETTE_BLUE,
	LV_PALETTE_YELLOW,
};

static lv_timer_t *sensor_timer;

/**
 * Zbus callback for inference results
 *
 * Called every time a new message is published to the inference result channel
 */
static void inference_cb(const struct zbus_channel *chan)
{
	struct inference_result_msg msg;
	zbus_chan_read(chan, &msg, K_MSEC(200));

	lv_label_set_text(inference_result_label, msg.label);
#if IS_LARGE_SCREEN
	lv_meter_set_indicator_value(inference_result_confidence_meter,
				     inference_result_confidence_indic, msg.confidence * 100);
#endif
}

ZBUS_LISTENER_DEFINE(inference_ui_listener, inference_cb);

extern struct k_sem sensor_data_ringbuf_sem;
extern struct ring_buf sensor_data_ringbuf;

/**
 * LVLGL timer handler
 * Gets sensor data directly from ring buffer and append it to the chart
 */
static void sensor_timer_cb(lv_timer_t *timer)
{
	uint32_t buffer[4];

	k_sem_take(&sensor_data_ringbuf_sem, K_FOREVER);
	int ret = ring_buf_peek(&sensor_data_ringbuf, (uint8_t*)buffer, 4 * sizeof(uint32_t));
	k_sem_give(&sensor_data_ringbuf_sem);

	if (ret < 4) {
		return;
	}

	for (int i = 0; i < 4; i++) {
		lv_chart_set_next_value(chart, series[i], buffer[i]);
	}
}

void create_sensor_chart(lv_obj_t *parent)
{
	/* Add zbus observer to be notified of new inference results */
	zbus_chan_add_obs(&inference_result_chan, &inference_ui_listener, K_MSEC(200));

	chart = lv_chart_create(parent);
	lv_obj_set_size(chart, LV_HOR_RES, LV_VER_RES);
	lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(chart, 5, 8);
	lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 800);
	lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);

	for (int i = 0; i < 4; i++) {
		series[i] = lv_chart_add_series(chart, lv_palette_main(palette_colors[i]),
						LV_CHART_AXIS_PRIMARY_Y);
	}

	lv_chart_set_point_count(chart, 400);

	#if IS_LARGE_SCREEN
	inference_result_confidence_meter = lv_meter_create(lv_scr_act());
	lv_obj_center(inference_result_confidence_meter);
	lv_obj_set_size(inference_result_confidence_meter, 110, 110);
	lv_obj_set_style_text_font(inference_result_confidence_meter, &lv_font_montserrat_8,
				   LV_PART_TICKS);
	lv_obj_set_style_bg_opa(inference_result_confidence_meter, LV_OPA_80, LV_PART_MAIN);

	/*Add a scale first*/
	lv_meter_scale_t *scale = lv_meter_add_scale(inference_result_confidence_meter);
	lv_meter_set_scale_ticks(inference_result_confidence_meter, scale, 41, 2, 5,
				 lv_palette_main(LV_PALETTE_GREY));
	lv_meter_set_scale_major_ticks(inference_result_confidence_meter, scale, 8, 2, 5,
				       lv_color_black(), 10);

	inference_result_confidence_indic = lv_meter_add_needle_line(
		inference_result_confidence_meter, scale, 4, lv_palette_main(LV_PALETTE_GREEN), -3);
	#endif

	inference_result_label = lv_label_create(lv_scr_act());
#if IS_LARGE_SCREEN
	lv_obj_align(inference_result_label, LV_ALIGN_CENTER, 0, 80);
#else
	lv_obj_align(inference_result_label, LV_ALIGN_CENTER, 0, 0);
#endif

	sensor_timer = lv_timer_create(sensor_timer_cb, 50, NULL);
}

#ifdef __cplusplus
}
#endif
