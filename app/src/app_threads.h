#include <zephyr/kernel.h>

/**********************************/
/* Sensor acquisition thread      */
/**********************************/
K_THREAD_STACK_DEFINE(sensor_acquisition_thread_stack, 512);
struct k_thread sensor_acquisition_thread;
void sensor_acquisition_fn(void *arg1, void *arg2, void *arg3);

/**********************************/
/* GUI thread                     */
/**********************************/
K_THREAD_STACK_DEFINE(gui_thread_stack, 2048);
struct k_thread gui_thread;
void gui_fn(void *arg1, void *arg2, void *arg3);