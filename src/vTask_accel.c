#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "vTask_accel.h"
#include "xEventBits.h" 

#define INT_PIN 4

#define INT_PIN 10
#define ACCEL_INT_BIT (1 << 0)

volatile bool interrupt_triggered = false;its.h"

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

extern EventGroupHandle_t xEventGroup = NULL;

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == INT_PIN) {
        interrupt_triggered = true;
    }
}
/**
 * @brief Callback function for handling GPIO interrupts.
 *
 * This function is triggered when a GPIO interrupt occurs. It checks if the
 * interrupt was caused by the specified interrupt pin (INT_PIN) and sets the
 * global flag `interrupt_triggered` to true if so.
 *
 * @param gpio The GPIO pin number that triggered the interrupt.
 * @param events The event mask indicating the type of interrupt event.
 */
static void accel_gpio_init(){
    gpio_init(INT_PIN);
    gpio_set_dir(INT_PIN, GPIO_IN);
    gpio_pull_up(INT_PIN);
    gpio_set_irq_enabled_with_callback(INT_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}
/**
 * @brief Initializes the ADXL345 accelerometer sensor.
 *
 * This function sets up the I2C communication and configures the ADXL345
 * sensor with the desired settings. It initializes the I2C interface,
 * configures the data format for full resolution and a range of ±16g,
 * enables measurement mode, sets the activity threshold to approximately 2g,
 * configures activity/inactivity control, enables interrupts for activity,
 * and maps the activity interrupt to INT1.
 */
static void adxl345_init() {
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

void vTask_accel(void *pvParameters) {
    accel_gpio_init();
    adxl345_init();
    //xEventGroupWaitBits(xEventGroup, );
    xEventGroupWaitBits(xEventGroup, ACCEL_INT_BIT, pdFALSE, pdTRUE);
    while (true) {
        if (interrupt_triggered) {
            interrupt_triggered = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}