#include <stdio.h>
#include "hardware/adc.h"
#include "event_groups.h"
#include "vTask_relay.h"
#include "xEventBits.h"
void vTask_Relay(void *pvParameters) {
    int16_t relay_state = set_rele_state(false); // Initial state

    while (true) {
        if (bat_ok && circuit_break_ok) {
            if (mus_active || interrupt_triggered) {
                if (relay_state != 1) set_rele_state(true);
            }
        } else {
            if (relay_state != 0) set_rele_state(false);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}