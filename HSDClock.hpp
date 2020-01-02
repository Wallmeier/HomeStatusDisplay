#ifndef HSDCLOCK_H
#define HSDCLOCK_H

#define EZTIME_EZT_NAMESPACE

#include "HSDConfig.hpp"
#include <TM1637.h>
#include <ezTime.h>

class HSDClock {
public:
    HSDClock(const HSDConfig* config);

    void begin();
    void handle();

private:
    const HSDConfig* m_config;
    Timezone         m_local;
    uint8_t          m_oldTime;
    TM1637*          m_tm1637;
};

#endif // HSDCLOCK_H
