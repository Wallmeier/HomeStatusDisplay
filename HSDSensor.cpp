#include "HSDSensor.hpp"

#include <Wire.h>

/*
I also bought five ITEAD's Si7021 sensors and noticed that those don't work with Tasmota. So, I took my logic analyzer 
and reverse engineered it. I can send more details later, if needed, but it looks their "one wire" protocol is quite 
simple. First, the host need to send 0.5ms low pulse, then go high for ~40us, and then move to tri-state (INPUT) to let 
the sensor send data back. The sensor sends totally 1bit+5 bytes/41 bits (MSB order). At first, the sensor sends a 
"start bit" that is "1". Next two bytes/16 bits are humidity with one decimal (i.e. 35.6% is sent as 356), two bytes/16 
bits after that are temperature in Celsius, again with one decimal (i.e. 23.4C is sent as 234), and finally the last 
byte/8 bits is a checksum. When sensor sends logical "1" it pulls the line HIGH to ~75us, when it sends logical "0", it 
pulls the line HIGH to ~25us. There is about 40us (LOW) between HIGH pulses. Checksum is basically a (8-bit) sum of 4 
data bytes. I also wrote a simple test code and got right values from the sensor without ITEAD's firmware, so, I believe 
my revers engineering is correct. @arendst, are you interested to add the support to some following release if I provide 
needed details?

Contact A | 1   Data
Contact C | 3   Gnd
Contact D | G   3,3V
*/
HSDSensor::HSDSensor(const HSDConfig& config) :
    m_bmp(nullptr),
    m_config(config),
    m_maxCycles(microsecondsToClockCycles(1000)), // 1 millisecond timeout for reading pulses from DHT sensor.
    m_tsl(nullptr)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::begin() {
    int sensorCnt(0);
    if (m_config.getSensorSonoffEnabled()) {
        sensorCnt += 2;
        m_pin = m_config.getSensorPin();
        Serial.print(F("Starting Sonoff Si7021 sensor on pin "));
        Serial.println(m_pin);
        pinMode(m_pin, INPUT_PULLUP);
    }
    if (m_config.getSensorI2CEnabled()) {
        Wire.begin();
        bool hasBmp(false), hasTsl(false);
        i2cscan(hasBmp, hasTsl);
        if (hasBmp) {
            m_bmp = new Adafruit_BMP085_Unified(sensorCnt++);
            if (!m_bmp->begin()) {
                /* There was a problem detecting the BMP085 ... check your connections */
                Serial.println("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
            } else {
                sensor_t sensor;
                m_bmp->getSensor(&sensor);
                printSensorDetails(sensor);
            }
        }
        if (hasTsl) {
            m_tsl = new Adafruit_TSL2561_Unified(0x39, sensorCnt++);
            if (!m_tsl->begin()) {
                /* There was a problem detecting the TSL2561 ... check your connections */
                Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
            } else {
                sensor_t sensor;
                m_tsl->getSensor(&sensor);
                printSensorDetails(sensor);
                
                m_tsl->enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
                // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
                m_tsl->setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
                // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */
            }
        }
    }
/*    m_sensorData = new SensorData[sensorCnt];
    sensorCnt = 0;
    if (m_config.getSensorSonoffEnabled()) {
        m_sensorData[sensorCnt++] = { SensorType::Temperature, F("Sonoff SI7021"), 0};
    }*/
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::handle(HSDWebserver& webServer, const HSDMqtt& mqtt) const {
    static unsigned long lastTime = 0;
    static bool first = true;
    if (first || millis() - lastTime >= m_config.getSensorInterval() * 60000) {
        first = false;
        lastTime = millis();
        String sensorData;
        
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        if (m_config.getSensorSonoffEnabled()) {
            float temp, hum;
            if (readSensor(temp, hum)) {
                Serial.print(F("Sonoff SI7021: Temp "));
                Serial.print(temp, 1);
                Serial.print(F("°C, Hum "));
                Serial.print(hum, 1);
                Serial.println(F("%"));
                json["Temp"] = temp;
                json["Hum"] = hum;
                
                sensorData  = F("Temp ");
                sensorData += String(temp, 1);
                sensorData += F("°C, Hum ");
                sensorData += String(hum, 1);
                sensorData += F("%");
            } else {
                Serial.println(F("Sonoff SI7021 failed"));
            }
        }
        if (m_bmp) {
            sensors_event_t event;
            m_bmp->getEvent(&event);
            if (event.pressure) {
                float press = round(m_bmp->seaLevelForAltitude(m_config.getSensorAltitude(), event.pressure) * 10) / 10.0;
                /* Display atmospheric pressue in hPa */
                Serial.print(F("BMP180: Pressure "));
                Serial.print(press, 1);
                Serial.println(F(" hPa"));
                json["Pressure"] = press;
                if (sensorData.length() > 0)
                    sensorData += F(", ");
                sensorData += F("Pressure ");
                sensorData += String(press, 1);
                sensorData += F(" hPa");
            } else {
                Serial.println("BMP180: Sensor error");
            }
        }
        if (m_tsl) {
            sensors_event_t event;
            m_tsl->getEvent(&event);
            if (event.light) {
                /* Display the results (light is measured in lux) */
                Serial.print(F("TSL2561: ")); Serial.print(event.light, 0); Serial.println(F(" lux"));
                json["Lux"] = event.light;
                
                if (sensorData.length() > 0)
                    sensorData += F(", ");
                sensorData += F("Light ");
                sensorData += String(event.light, 0);
                sensorData += F(" lux");
            } else {
                Serial.println("TSL2561: Sensor error");
            }            
        }
          
        webServer.setSensorData(sensorData);
        if (mqtt.connected()) {
            String topic = m_config.getMqttOutTopic("sensor");
            if (mqtt.isTopicValid(topic))
                mqtt.publish(topic, json);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

int32_t HSDSensor::expectPulse(bool level) const {
    uint32_t count(0);
    while (digitalRead(m_pin) == level) {
        if (count++ >= m_maxCycles)
            return -1;  // Timeout
    }
    return count;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDSensor::read(uint8_t* data) const {
    int32_t cycles[80];
    uint8_t error(0);

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    pinMode(m_pin, OUTPUT);
    digitalWrite(m_pin, LOW);
    delayMicroseconds(500);

    noInterrupts();
    digitalWrite(m_pin, HIGH);
    delayMicroseconds(40);
    pinMode(m_pin, INPUT_PULLUP);
    delayMicroseconds(10);
    if (-1 == expectPulse(LOW)) {
        Serial.println(F("Sensor: timeout waiting for start signal low pulse"));
        error = 1;
    } else if (-1 == expectPulse(HIGH)) {
        Serial.println(F("Sensor: timeout waiting for start signal high pulse"));
        error = 1;
    } else {
        for (uint32_t i = 0; i < 80; i += 2) {
            cycles[i]     = expectPulse(LOW);
            cycles[i + 1] = expectPulse(HIGH);
        }
    }
    interrupts();
    if (error) 
        return false;

    for (uint32_t i = 0; i < 40; ++i) {
        int32_t lowCycles  = cycles[2 * i];
        int32_t highCycles = cycles[2 * i + 1];
        if ((-1 == lowCycles) || (-1 == highCycles)) {
            Serial.println(F("Sensor: timeout waiting for pulse"));
            return false;
        }
        data[i/8] <<= 1;
        if (highCycles > lowCycles)
            data[i / 8] |= 1;
    }

    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (data[4] != checksum) {
        Serial.print(F("Sensor: checksum failure - exptected: "));
        Serial.print(checksum, HEX);
        Serial.print(F(", but got: "));
        Serial.println(data[4], HEX);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDSensor::readSensor(float& temp, float& hum) const {
    bool res(false);
    temp = NAN;
    hum = NAN;
    uint8_t data[5];
    
    if (read(data)) {
        hum  = ((data[0] << 8) | data[1]) * 0.1;
        temp = (((data[2] & 0x7F) << 8 ) | data[3]) * 0.1;
        if (data[2] & 0x80)
            temp *= -1;
        res =  true;
    }
    digitalWrite(m_pin, HIGH);
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::i2cscan(bool& hasBmp, bool& hasTsl) const {
    Serial.println(F("Starting i2c search"));
    Serial.println(F("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f"));
    Serial.print(  F("00:         "));
    for (byte address = 3; address <= 119; address++) {
        if (address % 16 == 0) { // new line
            Serial.print(F("\r\n"));
            Serial.print(String(address / 16));
            Serial.print(F("0:"));
        }
        Wire.beginTransmission(address);
        switch (Wire.endTransmission()) {
            case 0:
                Serial.print(F(" "));
                if (address < 16)
                    Serial.print(F("0"));
                Serial.print(address, HEX);
                if (address == 0x39)
                    hasTsl = true;
                else if (address == 0x77)
                    hasBmp = true;
                break;

            case 4: 
                Serial.print(F(" XX"));
                break;
                
            default:
                Serial.print(F(" --"));
                break;
        }
    }
    Serial.println(F(""));
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::printSensorDetails(sensor_t& sensor) const {
    Serial.print(F("Using "));
    Serial.print(sensor.name);
    Serial.print(F(" (driver ver: "));
    Serial.print(sensor.version);
    Serial.print(F(", id: "));
    Serial.print(sensor.sensor_id);
    Serial.print(F(", minVal: "));
    Serial.print(sensor.min_value); 
    Serial.print(F(", maxVal: "));
    Serial.print(sensor.max_value);
    Serial.print(F(", res: "));
    Serial.print(sensor.resolution);
    Serial.println(")");
}
