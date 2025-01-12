#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "vTask_gpio.h"

/* ************************************************
 * gpio task
 * 1. read the state of the MUS_PIN
 * ************************************************ */

static void mus_init(){
    gpio_init(MUS_PIN);
    gpio_set_dir(MUS_PIN, GPIO_IN);
    gpio_pull_up(MUS_PIN);
}

void vTask_gpio(void *pvParameters) {
    bool last_state = false;
    uint32_t last_time = 0;
    uint16_t mus_counter = 0;
    mus_init();
    while (true) {
        bool current_state = gpio_get(MUS_PIN);
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        if (current_state != last_state) {
            if (current_state == true && (current_time - last_time) > 1) {
                mus_active = true;
            }
            last_time = current_time;
        }
        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(1));
        if (++mus_counter >= 1000){
            mus_counter = 0;
            printf("MUS_TASK * 1000cycles\n");
        }
    }
}