#include <errno.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>
#include <zephyr/storage/flash_map.h>
#include "rgbled.h"
#include "wifi.h"

LOG_MODULE_REGISTER(g2l_thing, LOG_LEVEL_INF);

#define FS_MOUNT_POINT "/lfs"

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void*)FIXED_PARTITION_ID(storage_partition),
    .mnt_point = FS_MOUNT_POINT,
};

static int storage_init(void) {
    int rc = fs_mount(&lfs_storage_mnt);

    if (rc != 0) {
        LOG_ERR("Error mounting storage (%d)", rc);
        return rc;
    }

    LOG_INF("Storage mounted at %s", FS_MOUNT_POINT);

    rc = settings_subsys_init();
    if (rc != 0) {
        LOG_ERR("Error initializing settings subsystem (%d)", rc);
        return rc;
    }
    return 0;
}

SYS_INIT(storage_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int main(void) {
    LOG_INF("Application started");

    int rc = rgbled_init();
    if (rc != 0) {
        LOG_ERR("RGB LED initialization failed (%d)", rc);
        return rc;
    }

    rc = wifi_init();
    if (rc != 0) {
        LOG_ERR("WiFi initialization failed (%d)", rc);
        return rc;
    }

    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
