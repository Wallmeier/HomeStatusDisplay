#include "HSDLeds.hpp"

#define NUMBER_OF_ELEMENTS(array)  (sizeof(array) / sizeof(array[0]))

HSDLeds::HSDLeds(const HSDConfig* config) :
    m_config(config),
    m_numLeds(0),
    m_pLedState(nullptr),
    m_strip(nullptr)
{
    for (uint8_t idx = 0; idx < 5; idx++)
        m_behaviorOn[idx] = true;
    m_behaviorOn[static_cast<uint8_t>(HSDConfig::Behavior::Off)] = false;
}

// ---------------------------------------------------------------------------------------------------------------------

HSDLeds::~HSDLeds() {
    if (m_pLedState)
        delete[] m_pLedState;
    if (m_strip)
        delete m_strip;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLeds::begin() {
    m_numLeds = m_config->getNumberOfLeds();
    m_pLedState = new LedState[m_numLeds];
    Serial.printf("Starting LEDs on pin %d (length %d)\n", m_config->getLedDataPin(), m_numLeds);
    m_strip = new Adafruit_NeoPixel(m_numLeds, m_config->getLedDataPin(), NEO_GRB + NEO_KHZ800);
    m_strip->begin();
    m_strip->setBrightness(m_config->getLedBrightness());
  
    clear();
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDLeds::set(uint32_t ledNum, HSDConfig::Behavior behavior, uint32_t color) { 
    bool update(false);
    if (ledNum < m_numLeds) {
        update |= m_pLedState[ledNum].behavior != behavior;
        update |= m_pLedState[ledNum].color != color;
        
        m_pLedState[ledNum].behavior = behavior;
        m_pLedState[ledNum].color = color;
        
        if (update)
            updateStripe();
    }
    return update;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLeds::setAllOn(uint32_t color) {
    bool update(false);
    for (uint32_t idx = 0; idx < m_numLeds; idx++) {
        update |= m_pLedState[idx].behavior != HSDConfig::Behavior::On;
        update |= m_pLedState[idx].color != color;

        m_pLedState[idx].behavior = HSDConfig::Behavior::On;
        m_pLedState[idx].color = color;
    }
    if (update)
        updateStripe();
}

// ---------------------------------------------------------------------------------------------------------------------

uint32_t HSDLeds::getColor(uint32_t ledNum) const {
    if (ledNum < m_numLeds)
        return m_pLedState[ledNum].color;
    return LED_COLOR_NONE;
}

// ---------------------------------------------------------------------------------------------------------------------

HSDConfig::Behavior HSDLeds::getBehavior(uint32_t ledNum) const {
    HSDConfig::Behavior behavior = HSDConfig::Behavior::Off;
    if (ledNum < m_numLeds)
        behavior = m_pLedState[ledNum].behavior;
    return behavior;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLeds::updateStripe() {
    for (uint8_t idx = 0; idx < m_numLeds; idx++) {
        if (m_behaviorOn[static_cast<uint8_t>(m_pLedState[idx].behavior)])
            m_strip->setPixelColor(idx, m_pLedState[idx].color);
        else
            m_strip->setPixelColor(idx, LED_COLOR_NONE);
    }
    m_strip->show();
    Serial.println("Stripe updated");
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLeds::clear() {
    for (uint8_t idx = 0; idx < m_numLeds; idx++) {
        m_pLedState[idx].behavior = HSDConfig::Behavior::Off;
        m_pLedState[idx].color = LED_COLOR_NONE;
    }
    updateStripe();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLeds::update() {
    static unsigned long prevBlink(0), prevFlash(0), prevFlicker(0);
    
    unsigned long curMillis = millis();
    bool update(false);
    update |= checkCondition(HSDConfig::Behavior::Blinking, curMillis, prevBlink, 500, 500);
    update |= checkCondition(HSDConfig::Behavior::Flashing, curMillis, prevFlash, 2000, 200);
    update |= checkCondition(HSDConfig::Behavior::Flickering, curMillis, prevFlicker, 100, 100);
    if (update)
        updateStripe();
}

// ---------------------------------------------------------------------------------------------------------------------

bool HSDLeds::checkCondition(HSDConfig::Behavior behavior, unsigned long curMillis, unsigned long& prev, uint32_t offTime, 
                             uint32_t onTime) {
    bool update(false);
    uint8_t behIdx = static_cast<uint8_t>(behavior);
    if (!m_behaviorOn[behIdx] && (curMillis - prev >= onTime)) {
        m_behaviorOn[behIdx] = true;
        prev = curMillis;
        update = true;
    } else if (m_behaviorOn[behIdx] && (curMillis - prev >= offTime)) {
        m_behaviorOn[behIdx] = false;
        prev = curMillis;
        update = true;
    }
    if (update)
        for (uint8_t idx = 0; idx < m_numLeds; idx++)
            if (m_pLedState[idx].behavior == behavior && m_pLedState[idx].color != LED_COLOR_NONE)
                return true;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------
#ifdef MQTT_TEST_TOPIC
void HSDLeds::test(uint32_t type) {
    clear();
    if (type == 1) { // left row on
        for (uint32_t led = 0; led < m_numLeds / 3; led++) {
            m_pLedState[led].behavior = HSDConfig::Behavior::On;
            m_pLedState[led].color = LED_COLOR_GREEN;
        }
        updateStripe();
    } else if (type == 2) { // middle row on
        for (uint32_t led = m_numLeds / 3; led < m_numLeds / 3 * 2; led++) {
            m_pLedState[led].behavior = HSDConfig::Behavior::On;
            m_pLedState[led].color = LED_COLOR_GREEN;
        }
        updateStripe();
    } else if(type == 3) {  // right row on
        for (uint32_t led = m_numLeds / 3 * 2; led < m_numLeds; led++) {
            m_pLedState[led].behavior = HSDConfig::Behavior::On;
            m_pLedState[led].color = LED_COLOR_GREEN;
        }
        updateStripe();
    } else if (type == 4) { // all rows on
        for (uint32_t led = 0; led < m_numLeds; led++) {
            m_pLedState[led].behavior = HSDConfig::Behavior::On;
            m_pLedState[led].color = LED_COLOR_GREEN;
        }
        updateStripe();
    } else if (type == 5) {
        uint32_t colors[] = {LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_BLUE};
        for (uint32_t led = 0; led < m_numLeds / 3; led++) {
            for (uint32_t colorIndex = 0; colorIndex < NUMBER_OF_ELEMENTS(colors); colorIndex++) {
                m_pLedState[led].behavior = HSDConfig::Behavior::On;
                m_pLedState[led].color = colors[colorIndex];
  
                m_pLedState[led + m_numLeds / 3].behavior = HSDConfig::Behavior::On;
                m_pLedState[led + m_numLeds / 3].color = colors[colorIndex];
  
                m_pLedState[led + m_numLeds / 3 * 2].behavior = HSDConfig::Behavior::On;
                m_pLedState[led + m_numLeds / 3 * 2].color = colors[colorIndex];

                updateStripe();
                delay(50);
            }

            m_pLedState[led].behavior = HSDConfig::Behavior::Off;
            m_pLedState[led].color = LED_COLOR_NONE;

            m_pLedState[led + m_numLeds / 3].behavior = HSDConfig::Behavior::Off;
            m_pLedState[led + m_numLeds / 3].color = LED_COLOR_NONE;

            m_pLedState[led + m_numLeds / 3 * 2].behavior = HSDConfig::Behavior::Off;
            m_pLedState[led + m_numLeds / 3 * 2].color = LED_COLOR_NONE;
  
            updateStripe();
            delay(5);
        }
    }
}
#endif // MQTT_TEST_TOPIC
