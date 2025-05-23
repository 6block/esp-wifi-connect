#ifndef _WIFI_CONFIGURATION_BLE_H_
#define _WIFI_CONFIGURATION_BLE_H_

#include <esp_event.h>
#include <esp_timer.h>
#include <esp_wifi_types_generic.h>

class WifiConfigurationBle {
public:
    static WifiConfigurationBle& GetInstance();
    void Start();
private:
    WifiConfigurationBle();
    ~WifiConfigurationBle();
    // Delete copy constructor and assignment operator
    WifiConfigurationBle(const WifiConfigurationBle&) = delete;
    WifiConfigurationBle& operator=(const WifiConfigurationBle&) = delete;

    EventGroupHandle_t event_group_;

    static void WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void IpEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void WifiProvEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

#endif