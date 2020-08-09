#ifndef HSDLEDS
#define HSDLEDS

#include <Adafruit_NeoPixel.h>

#include "HSDConfig.hpp"

#define LED_COLOR_NONE    0x000000
#define LED_COLOR_BLUE    0x0000FF
#define LED_COLOR_GREEN   0x00FF00
#define LED_COLOR_ORANGE  0xFF4400
#define LED_COLOR_RED     0xFF0000
#define LED_COLOR_YELLOW  0xFFCC00

class HSDLeds {
public:  
    HSDLeds(const HSDConfig* config);
    ~HSDLeds();

    void                begin();
    void                clear();
    uint32_t            getColor(uint32_t ledNum) const;
    HSDConfig::Behavior getBehavior(uint32_t ledNum) const;
    bool                set(uint32_t ledNum, HSDConfig::Behavior behavior, uint32_t color);
    void                setAllOn(uint32_t color);
#ifdef MQTT_TEST_TOPIC    
    void                test(uint32_t type);
#endif    
    void                update();

private:
    struct LedState {
        HSDConfig::Behavior behavior;
        uint32_t            color;
    };

    bool checkCondition(HSDConfig::Behavior behavior, unsigned long curMillis, unsigned long& prev, uint32_t offTime, uint32_t onTime);
    void updateStripe();
  
    bool               m_behaviorOn[5];
    const HSDConfig*   m_config;
    uint32_t           m_numLeds;
    LedState*          m_pLedState;
    Adafruit_NeoPixel* m_strip;
};

#endif // HSDLEDS
