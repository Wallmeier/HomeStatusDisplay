#ifndef HSDCONFIGFILE_H
#define HSDCONFIGFILE_H

#include <Arduino.h>
#include <ArduinoJson.h>

class HSDConfigFile {
public:
    HSDConfigFile(const String& fileName);

    bool read(char* buffer, int bufSize);
    bool write(JsonObject* data);

private:
    String m_fileName;
};

#endif // HSDCONFIGFILE_H
