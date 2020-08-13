#include "HSDLogger.hpp"
#include "HSDWebserver.hpp"

#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>

HSDLogger::HSDLogger() :
    m_webServer(nullptr)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLogger::log(const char* format, ...) {
    char buf[256];           // place holder for sprintf output
    va_list args;            // args variable to hold the list of parameters
    va_start(args, format);  // mandatory call to initilase args
    vsnprintf(buf, 256, format, args);

    String msg = buf;
    log(msg);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLogger::log(String msg) {
    Serial.println(msg);
    handleWebSocket(msg);
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLogger::log() {
    Serial.println();
    handleWebSocket("");
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLogger::handleWebSocket(String line) {
    m_buffer.push_back(line);
    if (m_buffer.size() > 100)
        m_buffer.erase(m_buffer.begin());
    send();
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLogger::send() {
    if (m_webServer && m_webServer->log(m_buffer))
        m_buffer.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

size_t HSDLogger::write(uint8_t c) {
    if (c == '\n') {
        if (m_msgBuffer.endsWith("\r"))
            m_msgBuffer.remove(m_msgBuffer.length() - 1);
        log(m_msgBuffer);
        m_msgBuffer = "";
    } else {
        m_msgBuffer += static_cast<char>(c);
    }
    return 1;
}

// ---------------------------------------------------------------------------------------------------------------------

void HSDLogger::printf(const char* format, ...) {
    char buf[256];           // place holder for sprintf output
    va_list args;            // args variable to hold the list of parameters
    va_start(args, format);  // mandatory call to initilase args
    int len = vsnprintf(buf, 256, format, args);
    if (len > 0) {
        for (int idx = 0; idx < len; idx++)
            write(buf[idx]);
    }
}

HSDLogger Logger;
