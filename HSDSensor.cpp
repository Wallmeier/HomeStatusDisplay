#include "HSDSensor.hpp"
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
    m_config(config)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDSensor::begin() {
    if (m_config.getSensorSonoffEnabled()) {
        m_maxCycles = microsecondsToClockCycles(1000);  // 1 millisecond timeout for reading pulses from DHT sensor.
        m_pin = m_config.getSensorPin();
        Serial.print(F("Enabling sensor on pin "));
        Serial.println(m_pin);
        pinMode(m_pin, INPUT_PULLUP);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

int32_t HSDSensor::expectPulse(bool level) const {
  int32_t count(0);

    while (digitalRead(m_pin) == level) {
        if (count++ >= (int32_t)m_maxCycles) {
            return -1;  // Timeout
        }
    }
    return count;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDSensor::read() {
    int32_t cycles[80];
    uint8_t error(0);

    m_data[0] = m_data[1] = m_data[2] = m_data[3] = m_data[4] = 0;

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
        m_data[i/8] <<= 1;
        if (highCycles > lowCycles) {
            m_data[i / 8] |= 1;
        }
    }

    uint8_t checksum = (m_data[0] + m_data[1] + m_data[2] + m_data[3]) & 0xFF;
    if (m_data[4] != checksum) {
        Serial.print(F("Sensor: checksum failure - exptected: "));
        Serial.print(checksum, HEX);
        Serial.print(F(", but got: "));
        Serial.println(m_data[4], HEX);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDSensor::readSensor(float& temp, float& hum) {
    bool res(false);
    temp = NAN;
    hum = NAN;
    if (read()) {
        hum  = ((m_data[0] << 8) | m_data[1]) * 0.1;
        temp = (((m_data[2] & 0x7F) << 8 ) | m_data[3]) * 0.1;
        if (m_data[2] & 0x80)
            temp *= -1;
        res =  true;
    }
    digitalWrite(m_pin, HIGH);
    return res;
}
