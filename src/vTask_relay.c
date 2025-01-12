#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/pwm.h"
#include "event_groups.h"
#include "vTask_relay.h"
#include "xEventBits.h"

static void relay_gpio_init(){
    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_put(RELAY_PIN, 0);
}
static void configure_pwm_for_relay() {
    gpio_set_function(RELAY_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(RELAY_PIN);
    pwm_set_wrap(slice_num, 249); // Set frequency to 10 kHz (125 MHz / (249+1))
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 125); // 50% duty cycle
    pwm_set_enabled(slice_num, false); // Start disabled
}

static int16_t set_relay_state(bool active) {
    int16_t rc;
    uint slice_num = pwm_gpio_to_slice_num(RELAY_PIN);
    if (active) {
        configure_pwm_for_relay();
        pwm_set_enabled(slice_num, true);
        rc = 1;
    } else {
        pwm_set_enabled(slice_num, false);
        gpio_set_function(RELAY_PIN, GPIO_FUNC_SIO);
        gpio_set_dir(RELAY_PIN, GPIO_OUT);
        gpio_put(RELAY_PIN, 0);
        rc = 0;
    }
    return rc;
}

void vTask_relay(void *pvParameters) {
    int16_t relay_state = 0;
    relay_gpio_init();
    while (true) {
        if (bat_ok && circuit_break_ok) {
            if (mus_active || interrupt_triggered) {
                if (relay_state != 1) set_relay_state(true);
            }
        } else {
            if (relay_state != 0) set_relay_state(false);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}