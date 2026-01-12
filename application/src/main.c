#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(g2l_thing, LOG_LEVEL_INF);

int main(void) {
    LOG_INF("Application started");

    while (1) {
        LOG_INF("tick");
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
