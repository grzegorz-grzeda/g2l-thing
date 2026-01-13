#include <errno.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/storage/flash_map.h>

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
    return 0;
}

int main(void) {
    storage_init();
    LOG_INF("Application started");

    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
