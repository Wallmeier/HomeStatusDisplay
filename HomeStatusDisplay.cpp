#include "HomeStatusDisplay.hpp"

// function declarations
void handleMqttMessage(String topic, String msg);

#define ONE_MINUTE_MILLIS (60000)

HomeStatusDisplay::HomeStatusDisplay()
:
m_webServer(m_config, m_leds, m_mqttHandler),
m_wifi(m_config),
m_mqttHandler(m_config, std::bind(&HomeStatusDisplay::mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
m_leds(m_config),
#ifdef CLOCK_ENABLED
m_clock(m_config),
#endif
m_lastWifiConnectionState(false),
m_lastMqttConnectionState(false),
m_oneMinuteTimerLast(0),
m_uptime(0)
{
}

void HomeStatusDisplay::begin(const char* version, const char* identifier)
{
  // initialize serial
  Serial.begin(115200);
  Serial.println(F(""));

  m_config.begin(version, identifier);
  m_webServer.begin();
  m_leds.begin();
  m_wifi.begin();
  m_mqttHandler.begin();
  #ifdef CLOCK_ENABLED
  m_clock.begin();
  #endif

  Serial.print(F("Free RAM: ")); Serial.println(ESP.getFreeHeap());
}

void HomeStatusDisplay::work()
{
  uptime = calcUptime();
    
  checkConnections();

  m_wifi.handleConnection();
  m_webServer.handleClient(uptime);

  if(m_wifi.connected())
  {
    m_mqttHandler.handle();
  }
  
  m_leds.update();

  #ifdef CLOCK_ENABLED
  m_clock.handle();
  #endif

  delay(100);
}

unsigned long HomeStatusDisplay::calcUptime()
{
  if(millis() - m_oneMinuteTimerLast >= ONE_MINUTE_MILLIS)
  {
    m_uptime++;
    m_oneMinuteTimerLast = millis();

    Serial.println("Uptime: " + String(m_uptime) + " min");

    if(m_mqttHandler.connected())
    {
      uint32_t free;
      uint16_t max;
      uint8_t frag;
      ESP.getHeapStats(&free, &max, &frag);

      m_mqttHandler.publish("HomeStatusDisplayStat", String(m_uptime, DEC) + " m," + String(free, DEC) + "," + String(max, DEC) + "," + String(frag, DEC));
    }
  }

  return m_uptime;
}

void HomeStatusDisplay::mqttCallback(char* topic, byte* payload, unsigned int length)
{
  int i = 0;

  for(i = 0; (i < length) && (i < MQTT_MSG_MAX_LEN); i++) 
  {
    mqttMsgBuffer[i] = payload[i];
  }
  mqttMsgBuffer[i] = '\0';

  String mqttTopicString(topic);
  String mqttMsgString = String(mqttMsgBuffer);
  
  Serial.print(F("Received an MQTT message for topic ")); Serial.println(mqttTopicString + ": " + mqttMsgString);

  if(mqttTopicString.equals(m_config.getMqttTestTopic()))
  {
    handleTest(mqttMsgString);
  }
  else if(isStatusTopic(mqttTopicString))
  {
    String device = getDevice(mqttTopicString);

    handleStatus(device, mqttMsgString);
  }
}

bool HomeStatusDisplay::isStatusTopic(String& topic)
{
  String mqttStatusTopic = String(m_config.getMqttStatusTopic());
  int posOfLastSlashInStatusTopic = mqttStatusTopic.lastIndexOf("/");

  return topic.startsWith(mqttStatusTopic.substring(0, posOfLastSlashInStatusTopic)) ? true : false;
}

String HomeStatusDisplay::getDevice(String& statusTopic)
{
  int posOfLastSlashInStatusTopic = statusTopic.lastIndexOf("/");

  return statusTopic.substring(posOfLastSlashInStatusTopic + 1);
}

void HomeStatusDisplay::handleTest(String msg)
{
  int type = msg.toInt();
  if(type > 0)
  {
    Serial.print(F("Showing testpattern ")); Serial.println(type);
    m_leds.test(type);
  }
  else if(type == 0)
  {
    m_leds.clear();
    m_mqttHandler.reconnect();  // back to normal
  }
}

void HomeStatusDisplay::handleStatus(String device, String msg)
{ 
  int ledNumber = m_config.getLedNumber(device);

  if(ledNumber != -1)
  {
    int colorMapIndex = m_config.getColorMapIndex(msg);
    
    if(colorMapIndex != -1)
    {
      HSDConfig::Behavior behavior = m_config.getLedBehavior(colorMapIndex);
      uint32_t color = m_config.getLedColor(colorMapIndex);

      Serial.println("Set led number " + String(ledNumber) + " to behavior " + String(behavior) + " with color " + m_config.hex2string(color));
      m_leds.set(ledNumber, behavior, color);
    }
    else if(msg.length() > 3 && msg[0] == '#')  // allow MQTT broker to directly set LED color with HEX strings
    {
      uint32_t color = m_config.string2hex(msg);

      Serial.println("Received HEX " + String(msg) + " and set led number " + String(ledNumber) + " with this color ON");
      m_leds.set(ledNumber, HSDConfig::ON, color);
    }
    else
    {
      Serial.println("Unknown message " + msg + " for led number " + String(ledNumber) + ", set to OFF");
      m_leds.set(ledNumber, HSDConfig::OFF, m_config.getDefaultColor("NONE"));
    }
  }
  else
  {
    Serial.println("No LED defined for device " + device + ", ignoring it");
  }
}

void HomeStatusDisplay::checkConnections()
{
  if(!m_lastMqttConnectionState && m_mqttHandler.connected())
  {
    m_leds.clear();
    m_lastMqttConnectionState = true;
  }
  else if(m_lastMqttConnectionState && !m_mqttHandler.connected())
  {
    m_leds.clear();
    m_lastMqttConnectionState = false;
  }

  if(!m_mqttHandler.connected() && m_wifi.connected())
  {
    m_leds.setAll(HSDConfig::ON, m_config.getDefaultColor("YELLOW"));
  }
  
  if(!m_lastWifiConnectionState && m_wifi.connected())
  {
    m_leds.clear();

    if(!m_mqttHandler.connected())
    {
      m_leds.setAll(HSDConfig::ON, m_config.getDefaultColor("ORANGE"));
    }
    
    m_lastWifiConnectionState = true;
  }
  else if(m_lastWifiConnectionState && !m_wifi.connected())
  {
    m_leds.clear();
    m_lastWifiConnectionState = false;
  }

  if(!m_wifi.connected())
  {
    m_leds.setAll(HSDConfig::ON, m_config.getDefaultColor("RED"));
  }
}


