#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "ws2812.pio.h"
#include "vTask_led.h"
#include "ws2812_pio.h"
#include "xEventBits.h"
#define RED_IDX 0
#define GREEN_IDX 1
#define BLUE_IDX 2

extern volatile bool interrupt_triggered;
extern volatile bool mus_active;
extern volatile bool bat_ok;
extern volatile bool circuit_break_ok;
extern EventGroupHandle_t xEventGroup;
// Три кольори
static const uint8_t colors[3][3] = {
    {180,   0,   0}, // R
    {0,   180,   0}, // G
    {0,     0, 180}, // B
};

// LED task
void vTask_led(void *pvParams)
{
    uint16_t led_task_state = LED_TASK_INIT_STATE;
    (void)pvParams;

    // PIO init
    ws2812_pio_init();

    uint8_t index = 0;

    for (;;)
    {
        if (led_task_state == LED_TASK_INIT_STATE){
            xEventGroupGetBits(xEventGroup);
            led_task_state = LED_TASK_RUN_STATE;

        } else {

        }
        
        printf("Output color %d\n", index);
        // 1) Send 1 pixel (3 bytes GRB)
        ws2812_pio_send_pixel(
            colors[index][0], // R
            colors[index][1], // G
            colors[index][2]  // B
        );

        // 2) Pause ~80 мкс, to WS2812 fix the data
        ws2812_pio_reset();

        // Change color
        index = (index + 1) % 3;

        // 1s delay
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
