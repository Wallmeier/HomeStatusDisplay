#ifdef ARDUINO_ARCH_ESP32

#include "HSDBluetooth.hpp"

// #include <functional>
#include <BLEDevice.h>

void onScanResult(BLEScanResults result) {
    Serial.print("Scan finished: "); Serial.println(result.getCount());
}

HSDBluetooth::HSDBluetooth(const HSDConfig& config, const HSDMqtt& mqtt) :
    m_BLEScan(nullptr),
    m_config(config),
    m_mqtt(mqtt)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDBluetooth::begin() {
    Serial.println(F("Starting Bluetooth LE support..."));
    BLEDevice::init("");
    m_BLEScan = BLEDevice::getScan(); //create new scan
    m_BLEScan->setAdvertisedDeviceCallbacks(this);
    m_BLEScan->setActiveScan(false);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDBluetooth::handle() {
    static bool first = true;
    static unsigned long millisLastScan = 0;
    if (first || ((millis() - millisLastScan) >= 65555)) { // 10100
        first = false;
        millisLastScan = millis();
        Serial.println(F("Starting Bluetooth scan"));
        m_BLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
        if (!m_BLEScan->start(10, onScanResult, true))
            Serial.println(F("Failed to start Bluetooth Scan"));
    }    
}

// ---------------------------------------------------------------------------------------------------------------------

int strpos(const char *haystack, const char *needle) //from @miere https://stackoverflow.com/users/548685/miere
{
   char *p = strstr(haystack, needle);
   if (p)
      return p - haystack;
   return -1;
} 

// ---------------------------------------------------------------------------------------------------------------------

void HSDBluetooth::onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print(F("Advertised Device: ")); Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceData()) {
        if (strstr(advertisedDevice.getServiceDataUUID().toString().c_str(), "fe95") != nullptr) {
            std::string serviceData = advertisedDevice.getServiceData();
            int serviceDataLength = serviceData.length();
            String returnedString;
            for (int i = 0; i < serviceDataLength; i++) {
                int a = serviceData[i];
                if (a < 16)
                    returnedString = returnedString + "0";
                returnedString = returnedString + String(a, HEX);  
            }
            char service_data[returnedString.length() + 1];
            returnedString.toCharArray(service_data, returnedString.length() + 1);
            service_data[returnedString.length()] = '\0';
            int pos = strpos(service_data, "209800");
            if (pos != -1) {             
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.createObject();
                Serial.println(F("mi flora data reading"));
                if (process_data(json, pos - 24, service_data)) {
                    String id = advertisedDevice.getAddress().toString().c_str();
                    json["id"] = id;
                    if (advertisedDevice.haveName())
                        json["name"] = advertisedDevice.getName().c_str();
                    if (advertisedDevice.haveRSSI())
                        json["rssi"] = advertisedDevice.getRSSI();
                    if (advertisedDevice.haveTXPower())
                        json["txPower"] = advertisedDevice.getTXPower();
                    String mac = advertisedDevice.getAddress().toString().c_str();
                    mac.replace(":", "");
                    mac.toUpperCase();
                    String topic = m_config.getMqttOutTopic("sensors/" + mac);
                    if (m_mqtt.connected() && m_mqtt.isTopicValid(topic))
                        m_mqtt.publish(topic, json);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDBluetooth::process_data(JsonObject& json, int offset, const char* rest_data) const {
    int data_length(0);
    switch (rest_data[51 + offset]) {
        case '1':
        case '2':
        case '3':
        case '4':
            data_length = ((rest_data[51 + offset] - '0') * 2) + 1;
            break;
        default:
            Serial.println("Can't read data length");
            return false;
    }
    
    char rev_data[data_length];
    char data[data_length];
    memcpy(rev_data, &rest_data[52 + offset], data_length);
    rev_data[data_length] = '\0';
  
    // reverse data order
    revert_hex_data(rev_data, data, data_length);
    double value = strtol(data, nullptr, 16);

    // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
    // Mi Jia provides tem(perature), batt(erry) and hum(idity)
    // following the value of digit 47 we determine the type of data we get from the sensor
    switch (rest_data[47 + offset]) {
        case '9':
            json.set("fer", (double)value);
            break;
        case '4':
            if (value > 65000) 
                value = value - 65535;
            json.set("tem", (double)value / 10);
            break;
        case '6':
            if (value > 65000) 
                value = value - 65535;
            json.set("hum", (double)value / 10);
            break;
        case '7':
            json.set("lux", (double)value);
            break;
        case '8':
            json.set("moi", (double)value);
            break;
        case 'a': // batteryLevel
            json.set("batt", (double)value);
            break;
        case 'd': // temp+hum
            char tempAr[8];
            // humidity
            memcpy(tempAr, data, 4);
            tempAr[4] = '\0';
            value = strtol(tempAr, nullptr, 16);
            if (value > 65000) 
                value = value - 65535;
            json.set("hum", (double)value / 10);
            // temperature
            memcpy(tempAr, &data[4], 4);
            tempAr[4] = '\0';
            value = strtol(tempAr, nullptr, 16);
            if (value > 65000) 
                value = value - 65535;
            json.set("tem", (double)value / 10);
            break;
        default:
            Serial.println("can't read values");
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDBluetooth::revert_hex_data(const char* in, char* out, int l) const {
    //reverting array 2 by 2 to get the data in good order
    int i = l - 2 , j = 0; 
    while (i != -2) {
        if (i % 2 == 0) 
            out[j] = in[i + 1];
        else  
            out[j] = in[i - 1];
        j++;
        i--;
    }
    out[l - 1] = '\0';
} 

#endif // ARDUINO_ARCH_ESP32
