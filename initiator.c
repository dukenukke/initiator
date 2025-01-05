#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "vTask_led.h"
#include "vTask_adc.h"

#define I2C_PORT i2c0
#define SDA_PIN 8
#define SCL_PIN 9

#define ADXL345_ADDR 0x53
#define ADXL345_THRESH_ACT 0x24
#define ADXL345_ACT_INACT_CTL 0x27
#define ADXL345_INT_ENABLE 0x2E
#define ADXL345_INT_MAP 0x2F
#define ADXL345_INT_SOURCE 0x30
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_POWER_CTL 0x2D

#define MUS_PIN 2
#define ARM_PIN 3
#define INT_PIN 4
#define RELE_PIN 5

volatile bool interrupt_triggered = false;
volatile bool mus_active = false;

/* The event group used by all the task based tests. */
EventGroupHandle_t xEventGroup = NULL;

/* The event group used by the interrupt based tests. */
EventGroupHandle_t xISREventGroup = NULL;

void adxl345_init() {
    uint8_t buf[2];

    // Initialize I2C
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Set data format (range +-16g, full resolution)
    buf[0] = ADXL345_DATA_FORMAT;
    buf[1] = 0x0B;
    i2c_write_blocking(I2C_PORT, ADXL345_ADDR, buf, 2, false);

    // Enable measurement mode
    buf[0] = ADXL345_POWER_CTL;
    buf[1] = 0x08;
    i2c_write_blocking(I2C_PORT, ADXL345_ADDR, buf, 2, false);

    // Set activity threshold to ~2g (0x32 = ~2g at 62.5mg/LSB)
    buf[0] = ADXL345_THRESH_ACT;
    buf[1] = 0x32;
    i2c_write_blocking(I2C_PORT, ADXL345_ADDR, buf, 2, false);

    // Configure activity/inactivity control (enable activity detection)
    buf[0] = ADXL345_ACT_INACT_CTL;
    buf[1] = 0x70;
    i2c_write_blocking(I2C_PORT, ADXL345_ADDR, buf, 2, false);

    // Enable interrupts for activity
    buf[0] = ADXL345_INT_ENABLE;
    buf[1] = 0x10;
    i2c_write_blocking(I2C_PORT, ADXL345_ADDR, buf, 2, false);

    // Map activity interrupt to INT1
    buf[0] = ADXL345_INT_MAP;
    buf[1] = 0x00;
    i2c_write_blocking(I2C_PORT, ADXL345_ADDR, buf, 2, false);
}

float read_adc_voltage(uint adc_channel) {
    adc_select_input(adc_channel);
    uint16_t raw = adc_read();
    return raw * 3.3f / 4096.0f; // Convert ADC value to voltage
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == INT_PIN) {
        interrupt_triggered = true;
    }
}

static void configure_pwm_for_rele() {
    gpio_set_function(RELE_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(RELE_PIN);
    pwm_set_wrap(slice_num, 249); // Set frequency to 10 kHz (125 MHz / (249+1))
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 125); // 50% duty cycle
    pwm_set_enabled(slice_num, false); // Start disabled
}

static int16_t set_rele_state(bool active) {
    int16_t rc;
    uint slice_num = pwm_gpio_to_slice_num(RELE_PIN);
    if (active) {
        configure_pwm_for_rele();
        pwm_set_enabled(slice_num, true);
        rc = 1;
    } else {
        pwm_set_enabled(slice_num, false);
        gpio_set_function(RELE_PIN, GPIO_FUNC_SIO);
        gpio_set_dir(RELE_PIN, GPIO_OUT);
        gpio_put(RELE_PIN, 0);
        rc = 0;
    }
    return rc;
}

void mus_task(void *pvParameters) {
    bool last_state = false;
    uint32_t last_time = 0;
    uint16_t mus_counter = 0;

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

void adxl_task(void *pvParameters) {
    xEventGroupWaitBits(xEventGroup, );
    xEventGroupWaitBits(xEventGroup, ACCEL_INT_BIT, pdFALSE, pdTRUE);
    while (true) {
        if (interrupt_triggered) {
            interrupt_triggered = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    gpio_init(MUS_PIN);
    gpio_set_dir(MUS_PIN, GPIO_IN);
    gpio_pull_up(MUS_PIN);

    gpio_init(ARM_PIN);
    gpio_set_dir(ARM_PIN, GPIO_IN);
    gpio_pull_down(ARM_PIN);

    gpio_init(INT_PIN);
    gpio_set_dir(INT_PIN, GPIO_IN);
    gpio_pull_up(INT_PIN);
    
    gpio_init(RELE_PIN);
    gpio_set_dir(RELE_PIN, GPIO_OUT);
    gpio_put(RELE_PIN, 0);

    
    xEventGroup = xEventGroupCreate();
    configASSERT( xEventGroup );
    xISREventGroup = xEventGroupCreate();
    configASSERT( xISREventGroup );
    xEventGroupSetBits(xEventGroup, 0x00);

    adxl345_init();
    gpio_set_irq_enabled_with_callback(INT_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    xTaskCreate(vTask_led, "LED Task", 256, NULL, 1, NULL);
    xTaskCreate(vTask_adc, "ADC Task", 256, NULL, 2, NULL);
    xTaskCreate(rele_task, "RELE Task", 256, NULL, 2, NULL);
    xTaskCreate(mus_task, "MUS Task", 256, NULL, 3, NULL);
    xTaskCreate(adxl_task, "ADXL Task", 256, NULL, 2, NULL);

    vTaskStartScheduler();

    while (true);

    return 0;
}
