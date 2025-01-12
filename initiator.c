#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "vTask_led.h"
#include "vTask_adc.h"
#include "vTask_relay.h"
#include "vTask_accel.h"
#include "vTask_gpio.h"
#include "xEventBits.h"



volatile bool interrupt_triggered = false;
volatile bool mus_active = false;

/* The event group used by all the task based tests. */
EventGroupHandle_t xEventGroup = NULL;

/* The event group used by the interrupt based tests. */
EventGroupHandle_t xISREventGroup = NULL;

int main() {
    stdio_init_all();

    gpio_init(ARM_PIN);
    gpio_set_dir(ARM_PIN, GPIO_IN);
    gpio_pull_down(ARM_PIN);
    
    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_put(RELAY_PIN, 0);

    xEventGroup = xEventGroupCreate();
    configASSERT( xEventGroup );
    xISREventGroup = xEventGroupCreate();
    configASSERT( xISREventGroup );
    xEventGroupSetBits(xEventGroup, 0x00);

    xTaskCreate(vTask_led, "LED Task", 256, NULL, 1, NULL);
    xTaskCreate(vTask_adc, "ADC Task", 256, NULL, 2, NULL);
    xTaskCreate(vTask_relay, "RELE Task", 256, NULL, 2, NULL);
    xTaskCreate(vTask_mus, "MUS Task", 256, NULL, 3, NULL);
    xTaskCreate(vTask_accel, "ADXL Task", 256, NULL, 2, NULL);
    vTaskStartScheduler();
    while (true);
    return 0;
}
