#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(g2l_thing, LOG_LEVEL_INF);

static int cmd_status(const struct shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "uptime: %lld ms", k_uptime_get());
    return 0;
}

SHELL_CMD_REGISTER(status, NULL, "Print device status", cmd_status);

int main(void) {
    LOG_INF("Application started");

    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
