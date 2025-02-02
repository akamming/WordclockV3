//MQTTClass
// by Arnold

#ifndef _MQTT_H_
#define _MQTT_H_
#include "config.h"



#define MQTTAUTODISCOVERYTOPIC "homeassistant"
#define FOREGROUNDNAME "Foreground"
#define BACKGROUNDNAME "Background"
#define SECONDSNAME "Seconds"
#define MODENAME "Mode"
#define ANIMATIONSPEEDNAME "AnimationSpeed"
#define DEBUGNAME "Debug"
#define CONNECTTIMEOUT 60000 // only try to connect once a minute
#define PUBLISHTIMEOUT 3600000 // publish the sensors at least every hour 

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
  void Debug(const char* status);

private:
  static void MQTTcallback(char* topic, byte* payload, unsigned int length);
  void PublishAllMQTTSensors();
  void PublishMQTTDimmer(const char* uniquename, bool SupportRGB);
  void PublishMQTTModeSelect(const char* uniquename);
  void PublishMQTTNumber(const char* uniquename, int min, int max, float step, bool isSlider);
  void PublishMQTTSwitch(const char* uniquename);
  void PublishMQTTText(const char* uniquename);
  void UpdateMQTTDimmer(const char* uniquename, bool Value, uint8_t brightness);
  void UpdateMQTTColorDimmer(const char* uniquename, palette_entry Color);
  void UpdateMQTTModeSelector(const char* uniquename, DisplayMode mode);
  void UpdateMQTTNumber(const char* uniquename, uint8_t Mod);
  void UpdateMQTTText(const char* uniquename, const char* text);
  void UpdateMQTTSwitch(const char* uniquename, bool Value);


  // vars to remember the last status
  bool debugging=true;
  bool mqtt_debugging=false;
  bool mqtt_nightmode = false;
  uint32_t mqtt_brightness = 0;
  palette_entry bg;
	palette_entry fg;
	palette_entry s;
  DisplayMode mqttDisplayMode;
  uint8_t mqtt_animspeed;

  unsigned long lastconnectcheck;
  unsigned long lastmqttpublication;

};

extern MqttClass MQTT;

#endif
