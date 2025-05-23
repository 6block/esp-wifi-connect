# ESP32 Wi-Fi Connect

This component helps with Wi-Fi connection for the device.

It provides the ability of Bluetooth distribution network and wifi network configuration and connection.

## Changelog: v2.4.2
forked from https://github.com/6block/esp-wifi-connect, support ble Wi-Fi Configuration.


## Configuration

The Wi-Fi credentials are stored in the flash under the "wifi" namespace.

The keys are "ssid", "ssid1", "ssid2" ... "ssid9", "password", "password1", "password2" ... "password9".

## Usage

```cpp
// Initialize the default event loop
ESP_ERROR_CHECK(esp_event_loop_create_default());

// Initialize NVS flash for Wi-Fi configuration
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
}
ESP_ERROR_CHECK(ret);

// Get the Wi-Fi configuration
auto& ssid_list = SsidManager::GetInstance().GetSsidList();
if (ssid_list.empty()) {
    // Start the Wi-Fi configuration BLE
    auto& ble = WifiConfigurationBle::GetInstance();
    ble.Start();
    return;
}

// Otherwise, connect to the Wi-Fi network
WifiStation::GetInstance().Start();
```

