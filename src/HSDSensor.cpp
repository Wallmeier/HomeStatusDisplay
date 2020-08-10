#include "HSDSensor.hpp"

#include <Wire.h>

void IRAM_ATTR detectsMotion(void* arg) {
    HSDSensor* sensor = reinterpret_cast<HSDSensor*>(arg);
#ifdef ARDUINO_ARCH_ESP32
    portENTER_CRITICAL_ISR(&sensor->m_pirMux);
#endif    
    uint8_t val = digitalRead(sensor->m_pirPin);
    if (sensor->m_pirValue != val) {
        sensor->m_pirValue = val;
        sensor->m_pirInterruptCounter++;
    }
#ifdef ARDUINO_ARCH_ESP32
    portEXIT_CRITICAL_ISR(&sensor->m_pirMux);
#endif    
}

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
HSDSensor::HSDSensor(const HSDConfig* config) :
    m_bmp(nullptr),
    m_config(config),
    m_maxCycles(microsecondsToClockCycles(1000)), // 1 millisecond timeout for reading pulses from DHT sensor.
    m_pin(0),
    m_pirInterruptCounter(0),
#ifdef ARDUINO_ARCH_ESP32    
    m_pirMux(portMUX_INITIALIZER_UNLOCKED),
#endif    
    m_pirPin(0),
    m_pirValue(LOW),
    m_tsl(nullptr)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::begin(HSDWebserver* webServer) {
    int sensorCnt(0);
    if (m_config->getSensorSonoffEnabled()) {
        sensorCnt += 2;
        m_pin = m_config->getSensorPin();
        Serial.print("Starting Sonoff Si7021 sensor on pin ");
        Serial.println(m_pin);
        pinMode(m_pin, INPUT_PULLUP);
        webServer->registerStatusEntry(HSDWebserver::StatusClass::Sensor, "Temperature", String(), "°C", "temperature");
        webServer->registerStatusEntry(HSDWebserver::StatusClass::Sensor, "Humidity", String(), "%", "humidity");
    }
    if (m_config->getSensorI2CEnabled()) {
        Wire.begin();
        bool hasBmp(false), hasTsl(false);
        i2cscan(hasBmp, hasTsl);
        if (hasBmp) {
            m_bmp = new Adafruit_BMP085_Unified(sensorCnt++);
            if (!m_bmp->begin()) {
                /* There was a problem detecting the BMP085 ... check your connections */
                Serial.println("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
            } else {
                webServer->registerStatusEntry(HSDWebserver::StatusClass::Sensor, "Pressure", String(), "hPa", "pressure");
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
                webServer->registerStatusEntry(HSDWebserver::StatusClass::Sensor, "Light", String(), "lux", "lux");
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
    if (m_config->getSensorPirEnabled()) {
        m_pirPin = m_config->getSensorPirPin();
        pinMode(m_pirPin, INPUT_PULLUP);
        attachInterruptArg(digitalPinToInterrupt(m_pirPin), detectsMotion, this, CHANGE);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::handle(HSDWebserver* webServer, const HSDMqtt* mqtt) {
    static unsigned long lastTime = 0;
    static bool first = true;

#ifdef ARDUINO_ARCH_ESP32
    portENTER_CRITICAL(&m_pirMux);
#endif    
    uint8_t cnt = m_pirInterruptCounter;
    uint8_t val = m_pirValue;
    m_pirInterruptCounter = 0;
#ifdef ARDUINO_ARCH_ESP32
    portEXIT_CRITICAL(&m_pirMux);
#endif    
    if (cnt > 0) {
        if (cnt > 1) {
            Serial.print("PIR interrupt triggered more than once: ");
            Serial.println(cnt);
        }
        String topic = m_config->getMqttOutTopic("motion");
        if (val == LOW) {
            Serial.println("Motion ended");
            mqtt->publish(topic, "0");
        } else if (val == HIGH) {
            Serial.println("Motion detected");
            mqtt->publish(topic, "1");
        }
    }    

    if (first || millis() - lastTime >= m_config->getSensorInterval() * 60000) {
        first = false;
        lastTime = millis();
        
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        if (m_config->getSensorSonoffEnabled()) {
            float temp, hum;
            if (readSensor(temp, hum)) {
                Serial.print("Sonoff SI7021: Temp ");
                Serial.print(temp, 1);
                Serial.print("°C, Hum ");
                Serial.print(hum, 1);
                Serial.println("%");
                
                webServer->updateStatusEntry("temperature", String(temp, 1));
                webServer->updateStatusEntry("humidity", String(hum, 1));
                
                json["Temp"] = temp;
                json["Hum"] = hum;
            } else {
                Serial.println("Sonoff SI7021 failed");
            }
        }
        if (m_bmp) {
            sensors_event_t event;
            m_bmp->getEvent(&event);
            if (event.pressure) {
                float press = round(m_bmp->seaLevelForAltitude(m_config->getSensorAltitude(), event.pressure) * 10) / 10.0;
                /* Display atmospheric pressue in hPa */
                Serial.print("BMP180: Pressure ");
                Serial.print(press, 1);
                Serial.println(" hPa");
                
                webServer->updateStatusEntry("pressure", String(press, 1));
                
                json["Pressure"] = press;
            } else {
                Serial.println("BMP180: Sensor error");
            }
        }
        if (m_tsl) {
            sensors_event_t event;
            m_tsl->getEvent(&event);
            if (event.light) {
                /* Display the results (light is measured in lux) */
                Serial.print("TSL2561: "); Serial.print(event.light, 0); Serial.println(" lux");
                
                webServer->updateStatusEntry("lux", String(event.light, 0));
                
                json["Lux"] = event.light;
            } else {
                Serial.println("TSL2561: Sensor error");
            }            
        }
          
        if (mqtt->connected()) {
            String topic = m_config->getMqttOutTopic("sensor");
            if (mqtt->isTopicValid(topic))
                mqtt->publish(topic, json);
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
        Serial.println("Sensor: timeout waiting for start signal low pulse");
        error = 1;
    } else if (-1 == expectPulse(HIGH)) {
        Serial.println("Sensor: timeout waiting for start signal high pulse");
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
            Serial.println("Sensor: timeout waiting for pulse");
            return false;
        }
        data[i/8] <<= 1;
        if (highCycles > lowCycles)
            data[i / 8] |= 1;
    }

    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (data[4] != checksum) {
        Serial.print("Sensor: checksum failure - exptected: ");
        Serial.print(checksum, HEX);
        Serial.print(", but got: ");
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
    Serial.println("Starting i2c search");
    Serial.println("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
    Serial.print(  "00:         ");
    for (byte address = 3; address <= 119; address++) {
        if (address % 16 == 0) { // new line
            Serial.print("\n");
            Serial.print(String(address / 16));
            Serial.print("0:");
        }
        Wire.beginTransmission(address);
        switch (Wire.endTransmission()) {
            case 0:
                Serial.print(" ");
                if (address < 16)
                    Serial.print("0");
                Serial.print(address, HEX);
                if (address == 0x39)
                    hasTsl = true;
                else if (address == 0x77)
                    hasBmp = true;
                break;

            case 4: 
                Serial.print(" XX");
                break;
                
            default:
                Serial.print(" --");
                break;
        }
    }
    Serial.println("");
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::printSensorDetails(sensor_t& sensor) const {
    Serial.print("Using ");
    Serial.print(sensor.name);
    Serial.print(" (driver ver: ");
    Serial.print(sensor.version);
    Serial.print(", id: ");
    Serial.print(sensor.sensor_id);
    Serial.print(", minVal: ");
    Serial.print(sensor.min_value); 
    Serial.print(", maxVal: ");
    Serial.print(sensor.max_value);
    Serial.print(", res: ");
    Serial.print(sensor.resolution);
    Serial.println(")");
}
