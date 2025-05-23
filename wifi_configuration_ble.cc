#include <stdio.h>
#include <string.h>
#include "wifi_configuration_ble.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include "ssid_manager.h"
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#define TAG "wifi_config_ble"
#define WIFI_EVENT_CONNECTED BIT0
#define MAX_RECONNECT_COUNT 5

WifiConfigurationBle& WifiConfigurationBle::GetInstance() {
    static WifiConfigurationBle instance;
    return instance;
}

WifiConfigurationBle::WifiConfigurationBle() {
    // Create the event group
    event_group_ = xEventGroupCreate();
}

WifiConfigurationBle::~WifiConfigurationBle() {
    vEventGroupDelete(event_group_);
}

void WifiConfigurationBle::Start() {
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WifiConfigurationBle::WifiEventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &WifiConfigurationBle::IpEventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_PROV_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WifiConfigurationBle::WifiProvEventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(PROTOCOMM_TRANSPORT_BLE_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WifiConfigurationBle::WifiProvEventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(PROTOCOMM_SECURITY_SESSION_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WifiConfigurationBle::WifiProvEventHandler,
                                                        this,
                                                        nullptr));
    // Create the default event loop
    esp_netif_create_default_wifi_sta();

    // Initialize the Wi-Fi stack in station mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_prov_mgr_config_t prov_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
        .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
        .wifi_prov_conn_cfg = {
            .wifi_conn_attempts = MAX_RECONNECT_COUNT,
        },
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));
    ESP_ERROR_CHECK(wifi_prov_mgr_reset_provisioning());
    char service_name[12];
    uint8_t eth_mac[6];
    const char *ssid_prefix = "Lumi_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, sizeof(service_name),
            "%s%02X%02X%02X",ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    const char *pop = "abcd1234";
    wifi_prov_security1_params_t *sec_params = pop;
    const char *service_key = NULL;
    uint8_t custom_service_uuid[] = {
        /* LSB <---------------------------------------
         * ---------------------------------------> MSB */
        0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
        0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
    };
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
    wifi_prov_mgr_endpoint_create("custom-data");
     /* Start provisioning service */
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, service_name, service_key));
}

void WifiConfigurationBle::WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
   //auto* this_ = static_cast<WifiConfigurationBle*>(arg);
    if (event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi station started");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi station disconnected");
        // xEventGroupClearBits(this_->event_group_, WIFI_EVENT_CONNECTED);
        // esp_wifi_connect();
    }
}

void WifiConfigurationBle::IpEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto* this_ = static_cast<WifiConfigurationBle*>(arg);
    auto* event = static_cast<ip_event_got_ip_t*>(event_data);

    char ip_address[16];
    esp_ip4addr_ntoa(&event->ip_info.ip, ip_address, sizeof(ip_address));
    ESP_LOGI(TAG, "Got IP: %s", ip_address);
     xEventGroupSetBits(this_->event_group_, WIFI_EVENT_CONNECTED);
}

void WifiConfigurationBle::WifiProvEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto* this_ = static_cast<WifiConfigurationBle*>(arg);
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Wi-Fi provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                    "\n\tSSID     : %s\n\tPassword : %s",
                    (const char *) wifi_sta_cfg->ssid,
                    (const char *) wifi_sta_cfg->password);
                    SsidManager::GetInstance().AddSsid((const char*)wifi_sta_cfg->ssid, (const char*)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            ESP_LOGI(TAG, "Wi-Fi provisioning failed");
            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        }
        case WIFI_PROV_END: {
            ESP_LOGI(TAG, "Wi-Fi provisioning ended");
            wifi_prov_mgr_deinit();
            // 创建一个延迟重启任务
            ESP_LOGI(TAG, "Rebooting...");
            xTaskCreate([](void *ctx) {
                // 等待500ms
                vTaskDelay(pdMS_TO_TICKS(500));
                // 执行重启
                esp_restart();
            }, "reboot_task", 4096, this_, 5, NULL);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "WiFi credentials received successfully");
            break;
        default:
            break;
        }
    } else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT) {
        switch (event_id)
        {
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            ESP_LOGI(TAG, "BLE transport connected");
            break;
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            ESP_LOGI(TAG, "BLE transport disconnected");
            break;
        default:
            break;
        }
    } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id)
        {
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            ESP_LOGI(TAG, "Security session setup OK");
            break;
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            ESP_LOGI(TAG, "Invalid security parameters");
            break;
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
            break;
        default:
            break;
        }
    }
}