#ifndef HSDLEDS
#define HSDLEDS

#include "HSDConfig.hpp"
#include <Adafruit_NeoPixel.h>

class HSDLeds {
public:  
    HSDLeds(const HSDConfig& config);
    ~HSDLeds();

    void                begin();
    void                clear();
    uint32_t            getColor(uint32_t ledNum) const;
    HSDConfig::Behavior getBehavior(uint32_t ledNum) const;
    void                set(uint32_t ledNum, HSDConfig::Behavior behavior, uint32_t color);
    void                setAll(HSDConfig::Behavior behavior, uint32_t color);
#ifdef MQTT_TEST_TOPIC    
    void                test(uint32_t type);
#endif    
    void                update();

private:
    struct LedState {
        HSDConfig::Behavior behavior;
        uint32_t            color;
    };

    void handleBlink(unsigned long currentMillis);
    void handleFlash(unsigned long currentMillis);
    void handleFlicker(unsigned long currentMillis);
    void updateStripe();
  
    static const uint32_t blinkOffTime = 500;
    static const uint32_t blinkOnTime = 500;
    static const uint32_t flashOffTime = 200;
    static const uint32_t flashOnTime = 2000;
    static const uint32_t flickerOffTime  = 100;
    static const uint32_t flickerOnTime  = 100;

    bool              m_blinkOn;
    const HSDConfig&  m_config;
    bool              m_flashOn;
    bool              m_flickerOn;
    uint32_t          m_numLeds;
    LedState*         m_pLedState;
    unsigned long     m_previousMillisBlink;
    unsigned long     m_previousMillisFlash;
    unsigned long     m_previousMillisFlicker;
    Adafruit_NeoPixel m_stripe;
};

#endif // HSDLEDS
