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
K_THREAD_STACK_DEFINE(gui_thread_stack, 4000);
struct k_thread gui_thread;
void gui_fn(void *arg1, void *arg2, void *arg3);

/**********************************/
/* Inference thread               */
/**********************************/
K_THREAD_STACK_DEFINE(inference_thread_stack, 2000);
struct k_thread inference_thread;
void inference_fn(void *arg1, void *arg2, void *arg3);

#endif /* __APP_THREADS_H_ */