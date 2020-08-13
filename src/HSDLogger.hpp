#ifndef HSDLOGGER_H
#define HSDLOGGER_H

#include <Arduino.h>
#include <vector>

using namespace std;

class HSDWebserver;

class HSDLogger : public Print {
public:
    HSDLogger();

    void log();
    void log(const char* format, ...);
    void log(String msg);
    void send();
    inline void setWebServer(HSDWebserver* webServer) { m_webServer = webServer; }

    void   printf(const char* format, ...);
    size_t write(uint8_t);

private:
    void handleWebSocket(String line);

    vector<String> m_buffer;
    String         m_msgBuffer;
    HSDWebserver*  m_webServer;
};

extern HSDLogger Logger;

#endif // HSDLOGGER_H