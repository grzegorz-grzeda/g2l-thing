#include <stdio.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/shell/shell.h>
#include "rgbled.h"

LOG_MODULE_REGISTER(g2l_wifi, LOG_LEVEL_INF);

static struct net_if* sta_iface;
static struct net_if* ap_iface;
static struct wifi_connect_req_params sta_config;
static struct net_mgmt_event_callback cb;

#define LED_BRIGHTNESS 32

#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK                                               \
    (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT |   \
     NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
     NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

static bool connect_persistently = true;
static char wifi_ssid[WIFI_SSID_MAX_LEN + 1];
static char wifi_psk[WIFI_PSK_MAX_LEN + 1];

static void wifi_event_handler(struct net_mgmt_event_callback* cb,
                               uint64_t mgmt_event,
                               struct net_if* iface) {
    switch (mgmt_event) {
        case NET_EVENT_WIFI_CONNECT_RESULT: {
            LOG_INF("Connected to network '%s'", wifi_ssid);
            rgbled_set_color(0, 0, LED_BRIGHTNESS,
                             0);  // Set first LED to green
            rgbled_update();
            break;
        }
        case NET_EVENT_WIFI_DISCONNECT_RESULT: {
            if (connect_persistently) {
                LOG_INF("Reconnecting to '%s'", wifi_ssid);
                int ret =
                    net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
                             sizeof(struct wifi_connect_req_params));
                if (ret) {
                    LOG_ERR("Unable to Connect to (%s)", wifi_ssid);
                }
            } else {
                LOG_INF("Disconnected from %s", wifi_ssid);
            }
            rgbled_set_color(0, LED_BRIGHTNESS, 0, 0);  // Set first LED to red
            rgbled_update();
            break;
        }
        case NET_EVENT_WIFI_AP_ENABLE_RESULT: {
            LOG_INF("AP Mode is enabled. Waiting for station to connect");
            break;
        }
        case NET_EVENT_WIFI_AP_DISABLE_RESULT: {
            LOG_INF("AP Mode is disabled.");
            break;
        }
        case NET_EVENT_WIFI_AP_STA_CONNECTED: {
            struct wifi_ap_sta_info* sta_info =
                (struct wifi_ap_sta_info*)cb->info;

            LOG_INF("station: " MACSTR " joined ", sta_info->mac[0],
                    sta_info->mac[1], sta_info->mac[2], sta_info->mac[3],
                    sta_info->mac[4], sta_info->mac[5]);
            break;
        }
        case NET_EVENT_WIFI_AP_STA_DISCONNECTED: {
            struct wifi_ap_sta_info* sta_info =
                (struct wifi_ap_sta_info*)cb->info;

            LOG_INF("station: " MACSTR " leave ", sta_info->mac[0],
                    sta_info->mac[1], sta_info->mac[2], sta_info->mac[3],
                    sta_info->mac[4], sta_info->mac[5]);
            break;
        }
        default:
            break;
    }
}

static int get_ssid_from_storage(char* ssid, size_t max_len) {
    struct fs_file_t file;
    fs_file_t_init(&file);
    const char* path = "/lfs/wifi_credentials_ssid.txt";
    if (fs_open(&file, path, FS_O_READ) != 0) {
        LOG_ERR("Failed to create file %s", path);
        return -1;
    }
    fs_read(&file, ssid, max_len);
    fs_close(&file);
    return 0;
}

static int get_psk_from_storage(char* psk, size_t max_len) {
    struct fs_file_t file;
    fs_file_t_init(&file);
    const char* path = "/lfs/wifi_credentials_psk.txt";
    if (fs_open(&file, path, FS_O_READ) != 0) {
        return -EIO;
    }
    fs_read(&file, psk, max_len);
    fs_close(&file);
    return 0;
}

static int disconnect_from_wifi(void) {
    int ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);
    if (ret) {
        LOG_ERR("Unable to Disconnect");
        return ret;
    }
    return 0;
}

static int connect_to_wifi(void) {
    int rc = get_ssid_from_storage(wifi_ssid, sizeof(wifi_ssid));
    if (rc != 0) {
        LOG_ERR("Failed to get SSID from storage (%d)", rc);
        return rc;
    }
    rc = get_psk_from_storage(wifi_psk, sizeof(wifi_psk));
    if (rc != 0) {
        LOG_ERR("Failed to get PSK from storage (%d)", rc);
        return rc;
    }

    sta_config.ssid = (const uint8_t*)wifi_ssid;
    sta_config.ssid_length = strlen(wifi_ssid);
    sta_config.psk = (const uint8_t*)wifi_psk;
    sta_config.psk_length = strlen(wifi_psk);
    sta_config.security = WIFI_SECURITY_TYPE_PSK;
    sta_config.channel = WIFI_CHANNEL_ANY;
    sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

    LOG_INF("Connecting to SSID: '%s'", sta_config.ssid);
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
                       sizeof(struct wifi_connect_req_params));
    if (ret) {
        LOG_ERR("Unable to Connect to (%s)", wifi_ssid);
        disconnect_from_wifi();
        return ret;
    }
    return 0;
}

int wifi_init(void) {
    rgbled_set_color(0, 0, 0, 0);  // Set first LED to red
    rgbled_update();
    net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&cb);

    sta_iface = net_if_get_wifi_sta();
    if (!sta_iface) {
        LOG_INF("STA: interface no initialized");
        return -EIO;
    }
    ap_iface = net_if_get_wifi_sap();
    if (!ap_iface) {
        LOG_INF("AP: interface no initialized");
        return -EIO;
    }

    int ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, ap_iface, NULL, 0);
    if (ret) {
        LOG_ERR("NET_REQUEST_WIFI_AP_DISABLE failed, err: %d", ret);
        return ret;
    }

    ret = connect_to_wifi();

    return ret;
}

static int cmd_wifi_set_ssid_psk(const struct shell* sh,
                                 size_t argc,
                                 char** argv) {
    if (argc > 3 || argc < 2) {
        shell_error(sh, "Usage: wifi_set <ssid> <psk>");
        return -EINVAL;
    }

    struct fs_file_t file;
    fs_file_t_init(&file);
    if (fs_open(&file, "/lfs/wifi_credentials_ssid.txt",
                FS_O_WRITE | FS_O_CREATE) != 0) {
        shell_error(sh, "Failed to open SSID file for writing");
        return -EIO;
    }

    fs_write(&file, argv[1], strlen(argv[1]));
    fs_close(&file);

    shell_print(sh, "SSID set to: %s", argv[1]);

    if (argc == 3) {
        fs_file_t_init(&file);
        if (fs_open(&file, "/lfs/wifi_credentials_psk.txt",
                    FS_O_WRITE | FS_O_CREATE) != 0) {
            shell_error(sh, "Failed to open PSK file for writing");
            return -EIO;
        }

        fs_write(&file, argv[2], strlen(argv[2]));
        fs_close(&file);

        shell_print(sh, "PSK set.");
    }

    shell_print(sh, "Reboot to apply changes");

    return 0;
}

static int cmd_wifi_disconnect(const struct shell* sh,
                               size_t argc,
                               char** argv) {
    int ret = disconnect_from_wifi();
    if (ret) {
        shell_error(sh, "Unable to Disconnect");
        return ret;
    }

    shell_print(sh, "Disconnected from WiFi");

    return 0;
}

static int cmd_wifi_connect(const struct shell* sh, size_t argc, char** argv) {
    int ret = connect_to_wifi();
    if (ret) {
        shell_error(sh, "Unable to Connect to WiFi");
        return ret;
    }

    shell_print(sh, "Connecting to WiFi...");

    return 0;
}

static int cmd_wifi_connect_persistent(const struct shell* sh,
                                       size_t argc,
                                       char** argv) {
    if (argc < 2) {
        shell_error(sh, "Usage: wifi_connect_persistent <0|1>");
        return -EINVAL;
    }
    connect_persistently = (strcmp(argv[1], "1") == 0);

    shell_print(sh, "Set to connect %spersistently",
                connect_persistently ? "" : "NOT ");
    return 0;
}

SHELL_CMD_REGISTER(wifi_set_ssid_psk,
                   NULL,
                   "Set WiFi SSID and PSK",
                   cmd_wifi_set_ssid_psk);

SHELL_CMD_REGISTER(wifi_connect, NULL, "Connect to WiFi", cmd_wifi_connect);

SHELL_CMD_REGISTER(wifi_disconnect,
                   NULL,
                   "Disconnect from WiFi",
                   cmd_wifi_disconnect);

SHELL_CMD_REGISTER(wifi_connect_persistent,
                   NULL,
                   "Set to connect persistently",
                   cmd_wifi_connect_persistent);