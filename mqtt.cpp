//MQTTClass
// by Arnold
#include <ESP8266WiFi.h>
#include <PubSubClient.h>         // MQTT library
#include "mqtt.h"
#include "config.h"
#include "brightness.h"
#include <ArduinoJson.h>

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
   if (MQ.connected()) {
     // Check if values changed, and if yes: communicate

     // main switch
     if (this->mqtt_brightness!=Brightness.brightnessOverride or this->mqtt_nightmode != Config.nightmode) {
      this->mqtt_brightness=Brightness.brightnessOverride;
      this->mqtt_nightmode= Config.nightmode;
      this->UpdateMQTTDimmer(Config.hostname,this->mqtt_nightmode ? false : true,this->mqtt_brightness);  
    }

  }
}

//---------------------------------------------------------------------------------------
// PublishMQTTDimmer
//
// Publish autodiscoverymessage for MQTT Dimmer switch
//
// -> --
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::PublishMQTTDimmer(const char* uniquename)
{
  Serial.println("PublishMQTTDimmer");
  StaticJsonDocument<512> json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = String(Config.hostname)+"/light/"+String(uniquename)+"/set";
  json["stat_t"] = String(Config.hostname)+"/light/"+String(uniquename)+"/state";
  json["schema"] = "json";
  json["brightness"] = true;
  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+"/light/"+String(Config.hostname)+"/"+String(uniquename)+"/config").c_str(),conf,Config.mqttpersistence);

}

//---------------------------------------------------------------------------------------
// UpdateMQTTDimmer
//
// Update an MQTT Dimer switch
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTDimmer(const char* uniquename, bool Value, uint8_t  Mod)
{
  Serial.println("UpdateMQTTDimmer");
  StaticJsonDocument<512> json;

  // Construct JSON config message
  json["state"]=Value ? "ON" : "OFF";
  if (Value and Mod==0) { // Workaround for homekit not being able to show dimmer with value on and brightness 0
    json["brightness"]=3;     
  } else {
    json["brightness"]=Mod;
  }
  char state[128];
  serializeJson(json, state);  // state now contains the json

  // publish state message
  MQ.publish((String(Config.hostname)+"/light/"+String(uniquename)+"/state").c_str(),state,Config.mqttpersistence);
}

//---------------------------------------------------------------------------------------
// PublishAllMQTTsensors
//
// Send autodiscovery messages for all sensors
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::PublishAllMQTTSensors()
{
  this->PublishMQTTDimmer(Config.hostname);
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
        this->PublishAllMQTTSensors();   
        this->mqtt_brightness = Brightness.brightnessOverride==50 ? 51 : 50; // remember the wrong value (forcing communication in next loop)  
        this->mqtt_nightmode = Config.nightmode ? false : true ; // remember the wrong value (forcing communication in next loop)
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