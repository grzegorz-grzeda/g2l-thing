#include "led.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led);

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#define STRIP_NODE DT_ALIAS(led_strip)

#define CONFIG_SAMPLE_LED_BRIGHTNESS 128

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif
#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device* const strip = DEVICE_DT_GET(STRIP_NODE);

int led_init(void) {
    if (!device_is_ready(strip)) {
        LOG_ERR("LED strip device %s is not ready", strip->name);
        return -ENODEV;
    }
    LOG_INF("Found LED strip device %s", strip->name);
    return 0;
}

int led_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= STRIP_NUM_PIXELS) {
        return -EINVAL;
    }
    struct led_rgb color = RGB(r, g, b);
    pixels[index] = color;
    return 0;
}

int led_set_all_colors(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < STRIP_NUM_PIXELS; i++) {
        led_set_color(i, r, g, b);
    }
    return 0;
}

int led_update(void) {
    return led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static int cmd_led_set_color(const struct shell* sh, size_t argc, char** argv) {
    if (argc != 5) {
        shell_error(sh, "Usage: led set <index> <r> <g> <b>");
        return -EINVAL;
    }

    uint8_t index = (uint8_t)atoi(argv[1]);
    uint8_t r = (uint8_t)atoi(argv[2]);
    uint8_t g = (uint8_t)atoi(argv[3]);
    uint8_t b = (uint8_t)atoi(argv[4]);

    int ret = led_set_color(index, r, g, b);
    if (ret < 0) {
        shell_error(sh, "Failed to set LED color: %d", ret);
        return ret;
    }

    ret = led_update();
    if (ret < 0) {
        shell_error(sh, "Failed to update LED strip: %d", ret);
        return ret;
    }

    shell_print(sh, "LED %d set to RGB(%d, %d, %d)", index, r, g, b);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(led_cmds,
                               SHELL_CMD(set,
                                         NULL,
                                         "Set LED color <index> <r> <g> <b>",
                                         cmd_led_set_color),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(rgbled, &led_cmds, "RGB LED commands", NULL);