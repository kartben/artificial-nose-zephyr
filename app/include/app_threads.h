#include <zephyr/kernel.h>

#ifndef __APP_THREADS_H_
#define __APP_THREADS_H_

/**********************************/
/* Sensor acquisition thread      */
/**********************************/
K_THREAD_STACK_DEFINE(sensor_acquisition_thread_stack, 1024);
struct k_thread sensor_acquisition_thread;
void sensor_acquisition_fn(void *arg1, void *arg2, void *arg3);

/**********************************/
/* GUI thread                     */
/**********************************/
#ifdef CONFIG_DISPLAY
K_THREAD_STACK_DEFINE(gui_thread_stack, 4000);
struct k_thread gui_thread;
void gui_fn(void *arg1, void *arg2, void *arg3);
#endif

/**********************************/
/* Inference thread               */
/**********************************/
K_THREAD_STACK_DEFINE(inference_thread_stack, 2000);
struct k_thread inference_thread;
void inference_fn(void *arg1, void *arg2, void *arg3);

#ifdef CONFIG_APP_MQTT
/**********************************/
/* MQTT pub thread                */
/**********************************/
K_THREAD_STACK_DEFINE(mqtt_pub_thread_stack, 512);
struct k_thread mqtt_pub_thread;
void mqtt_pub_fn(void *arg1, void *arg2, void *arg3);
#endif

#endif /* __APP_THREADS_H_ */