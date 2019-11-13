#pragma once

#include "HSDConfig.hpp"
#include "HSDWifi.hpp"
#include "HSDWebserver.hpp"
#include "HSDLeds.hpp"
#include "HSDMqtt.hpp"

#ifdef HSD_CLOCK_ENABLED
#include "HSDClock.hpp"
#endif

#ifdef HSD_SENSOR_ENABLED
#include "HSDSensor.hpp"
#endif

class HomeStatusDisplay
{
public:

  HomeStatusDisplay();
  
  void begin(const char* version, const char* identifier);
  void work();
  
private:

  unsigned long uptime = 0;
  unsigned long calcUptime();

  static const int MQTT_MSG_MAX_LEN = 50;
  
  void mqttCallback(char* topic, byte* payload, unsigned int length);

  bool isStatusTopic(String& topic);
  String getDevice(String& statusTopic);

  void handleStatus(String device, String msg);
  void handleTest(String msg);

  void checkConnections();

  char mqttMsgBuffer[MQTT_MSG_MAX_LEN + 1];
  
  HSDConfig m_config;
  HSDWifi m_wifi;
  HSDWebserver m_webServer;
  HSDMqtt m_mqttHandler;
  HSDLeds m_leds;
  #ifdef HSD_CLOCK_ENABLED
  HSDClock m_clock;
  #endif
  #ifdef HSD_SENSOR_ENABLED
  HSDSensor m_sensor;
  #endif

  bool m_lastWifiConnectionState;
  bool m_lastMqttConnectionState;
  unsigned long m_oneMinuteTimerLast;
  unsigned long m_uptime;
};

