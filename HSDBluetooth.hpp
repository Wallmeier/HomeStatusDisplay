#ifdef ARDUINO_ARCH_ESP32

#ifndef HSDBLUETOOTH_H
#define HSDBLUETOOTH_H

#include <ArduinoJson.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>

#include "HSDConfig.hpp"
#include "HSDMqtt.hpp"

class HSDBluetooth : public BLEAdvertisedDeviceCallbacks {
public:
    HSDBluetooth(const HSDConfig& config, const HSDMqtt& mqtt);

    void begin();
    void handle();
    void onResult(BLEAdvertisedDevice advertisedDevice);
    
private:
    bool process_data(JsonObject& json, int offset, const char* rest_data) const;
    void revert_hex_data(const char* in, char* out, int l) const;
    
    BLEScan*         m_BLEScan;
    const HSDConfig& m_config;
    const HSDMqtt&   m_mqtt;
};

#endif // HSDBLUETOOTH_H
#endif // ARDUINO_ARCH_ESP32
