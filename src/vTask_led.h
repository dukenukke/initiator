#ifndef _WS2812_TASK_H
#define _WS2812_TASK_H
    #define LED_TASK_INIT_STATE 0
    #define LED_TASK_RUN_STATE  1
    void vTask_led(void *pvParams);
#endif