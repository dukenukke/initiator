#include <stdio.h>
#include "hardware/adc.h"
#include "event_groups.h"
#include "vTask_adc.h"
#include "xEventBits.h"

#define CIRCUIT_BREAK_ADC_CHANNEL 0
#define BAT_VOLT_ADC_CHANNEL 1
#define BAT_VOLT_THRESHOLD 1.0f
#define CIRCUIT_BREAK_THRESHOLD 1.0f
#define INIT_TIMEOUT_MS 30000
#define INIT_TIMEOUT_S  30
#define ADC_STAGE_INIT 0
#define ADC_STAGE_RUN 1

volatile bool bat_ok = false;
volatile bool circuit_break_ok = false;
volatile bool adc_init_ok = false;

extern EventGroupHandle_t xEventGroup;

static float read_adc_voltage(uint adc_channel) {
    adc_select_input(adc_channel);
    uint16_t raw = adc_read();
    return raw * 3.3f / 4096.0f; // Convert ADC value to voltage
}
void vTask_adc(void *pvParameters) {
    uint16_t adc_stage = (uint16_t)ADC_STAGE_INIT;
    uint32_t bat_timer = 0;
    uint32_t circuit_break_timer = 0;
    uint16_t adc_clk_counter = 0;
    float bat_voltage;
    float circuit_break_voltage;
    adc_init();
    adc_gpio_init(26); // BAT_VOLT_ADC_CHANNEL
    adc_gpio_init(27); // CIRCUIT_BREAK_ADC_CHANNEL
    //stage #1 - initializing
    while (true) {
        bat_voltage = read_adc_voltage(BAT_VOLT_ADC_CHANNEL);
        circuit_break_voltage = read_adc_voltage(CIRCUIT_BREAK_ADC_CHANNEL);
        if(++adc_clk_counter > 100){
            printf("BAT: %.2f");
        }
        if (adc_stage==ADC_STAGE_INIT){
            if (bat_voltage > BAT_VOLT_THRESHOLD) {
                bat_timer += 10;
                if (bat_timer >= 30000)  {
                    xEventGroupSetBits(xEventGroup, BAT_OK_BIT);
                }
            } else  
                bat_timer = 0;
            if(circuit_break_voltage <= CIRCUIT_BREAK_THRESHOLD){
                circuit_break_timer += 10;
                if (circuit_break_timer >= INIT_TIMEOUT_MS) {
                    
                    xEventGroupSetBits(xEventGroup, CIRCUIT_BREAK_OK_BIT);
                }
            } else{
                xEventgroupClearBits(xEventGroup, CIRCUIT_BREAK_OK_BIT);
            }
                circuit_break_timer = 0;

            if(circuit_break_ok & bat_ok) {
                adc_stage = ADC_STAGE_RUN;
                xEventGroupSetBits(xEventGroup, BAT_OK_BIT | CIRCUIT_BREAK_OK_BIT);
            }
        } else {

        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}