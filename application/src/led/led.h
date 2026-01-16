#include <stdint.h>

int led_init(void);

int led_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

int led_set_all_colors(uint8_t r, uint8_t g, uint8_t b);

int led_update(void);