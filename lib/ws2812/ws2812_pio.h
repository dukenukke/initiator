#pragma once
void ws2812_pio_init();
void ws2812_pio_send_pixel(uint8_t r, uint8_t g, uint8_t b);
void ws2812_pio_reset(void);
