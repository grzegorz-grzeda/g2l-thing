#include <stdint.h>

int rgbled_init(void);

int rgbled_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

int rgbled_set_all_colors(uint8_t r, uint8_t g, uint8_t b);

int rgbled_update(void);