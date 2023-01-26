//MQTTClass
// by Arnold

#ifndef _MQTT_H_
#define _MQTT_H_

#include <ESP8266WiFi.h>
#include <PubSubClient.h>         // MQTT library
#include "config.h"


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

};

extern MqttClass MQTT;

#endif
