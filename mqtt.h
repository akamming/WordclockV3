//MQTTClass
// by Arnold

#ifndef _MQTT_H_
#define _MQTT_H_



#define MQTTAUTODISCOVERYTOPIC "homeassistant"

class MqttClass
{
public:
	MqttClass();
	virtual ~MqttClass();
	void begin();
	void process();
  void reconnect();
  bool connected();


private:
  static void MQTTcallback(char* topic, byte* payload, unsigned int length);
  void PublishAllMQTTSensors();
  void PublishMQTTDimmer(const char* uniquename);
  void UpdateMQTTDimmer(const char* uniquename, bool Value, uint8_t Mod);

  // vars to remember the last status
  bool mqtt_nightmode = false;
  uint32_t mqtt_brightness = 0;


};

extern MqttClass MQTT;

#endif
