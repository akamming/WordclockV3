//MQTTClass
// by Arnold

#ifndef _MQTT_H_
#define _MQTT_H_
#include "config.h"



#define MQTTAUTODISCOVERYTOPIC "homeassistant"
#define FOREGROUNDNAME "Foreground"
#define BACKGROUNDNAME "Background"
#define SECONDSNAME "Seconds"
#define CONNECTTIMEOUT 60000 // only try to connect once a minute
#define PUBLISHTIMEOUT 600000 // publish the sensors at least every 10 minutes 

class MqttClass
{
public:
	MqttClass();
	virtual ~MqttClass();
	void begin();
	void process();
  void reconnect();
  bool connected();
  void PublishStatus(const char* status);


private:
  static void MQTTcallback(char* topic, byte* payload, unsigned int length);
  void PublishAllMQTTSensors();
  void PublishMQTTDimmer(const char* uniquename, bool SupportRGB);
  void UpdateMQTTDimmer(const char* uniquename, bool Value, uint8_t brightness);
  void UpdateMQTTColorDimmer(const char* uniquename, palette_entry Color);

  // vars to remember the last status
  bool mqtt_nightmode = false;
  uint32_t mqtt_brightness = 0;
  palette_entry bg;
	palette_entry fg;
	palette_entry s;

  unsigned long lastconnectcheck;
  unsigned long lastmqttpublication;

};

extern MqttClass MQTT;

#endif
