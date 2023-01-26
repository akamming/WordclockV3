//MQTTClass
// by Arnold
#include "mqtt.h"

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
MqttClass MQTT=MqttClass();
WiFiClient espClient;  // Needed for MQTT
PubSubClient MQ(espClient); // MQTT client

//---------------------------------------------------------------------------------------
// <MqttClass>
//
// Constructor, currently empty
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
MqttClass::MqttClass()
{
}

//---------------------------------------------------------------------------------------
// ~MqttClass
//
// Destructor, currently empty
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
MqttClass::~MqttClass()
{
}

//---------------------------------------------------------------------------------------
// begin
//
// Sets up internal handlers
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::begin()
{
  MQ.setServer(Config.mqttserver, Config.mqttport); // server details
  MQ.setBufferSize(512); // discovery messages are longer than default max buffersize(!)
  MQ.setCallback(MQTTcallback); // listen to callbacks
  this->reconnect();
}

//---------------------------------------------------------------------------------------
// process
//
// make sure MQTT is handled
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::process()
{
   MQ.loop();
}

//---------------------------------------------------------------------------------------
// connected
//
// return if connected
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
bool MqttClass::connected()
{
   return MQ.connected();
}

//---------------------------------------------------------------------------------------
// (re)connect
//
// Try to connect
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::reconnect()
{
  if (Config.usemqtt) {
    if (!MQ.connected()) {
      Serial.print("Attempting MQTT connection...");
      bool mqttconnected;
      if (Config.usemqttauthentication) {
        mqttconnected = MQ.connect(Config.hostname, Config.mqttuser, Config.mqttpass);
      } else {
        mqttconnected = MQ.connect(Config.hostname);
      }
      if (mqttconnected) {
        Serial.println("Connect succeeded");
        // PublishAllMQTTSensors();      
      } else {
        Serial.print("failed, rc=");
        Serial.print(MQ.state());
      }
    }
  } 
}


//---------------------------------------------------------------------------------------
// process
//
// handle callbacks (currently empty)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::MQTTcallback(char* topic, byte* payload, unsigned int length) 
{
}