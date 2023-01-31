//MQTTClass
// by Arnold
#include <ESP8266WiFi.h>
#include <PubSubClient.h>         // MQTT library
#include "mqtt.h"
#include "brightness.h"
#include <ArduinoJson.h>

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
MqttClass MQTT=MqttClass();
WiFiClient espClient;  // Needed for MQTT
PubSubClient MQ(espClient); // MQTT client

//---------------------------------------------------------------------------------------
// helper functions
//---------------------------------------------------------------------------------------


bool isSameColor(palette_entry A, palette_entry B)
{
  bool SameColor=true;
  if (A.r!=B.r) SameColor=false;
  if (A.g!=B.g) SameColor=false;
  if (A.b!=B.b) SameColor=false;
  return SameColor;
}

uint8_t MaxColor(palette_entry A) 
{
  uint8_t max = A.r;
  if (A.g>max) max=A.g;
  if (A.b>max) max=A.b;
  return max;
}


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
  this->lastconnectcheck = millis()-CONNECTTIMEOUT-10; // force try to connect immediately
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
  this->reconnect();
  if (MQ.connected()) {
    // Check if values changed, and if yes: communicate
    if (this->mqtt_brightness!=Brightness.brightnessOverride or this->mqtt_nightmode != Config.nightmode) {
      this->mqtt_brightness=Brightness.brightnessOverride;
      this->mqtt_nightmode= Config.nightmode;
      this->UpdateMQTTDimmer(Config.hostname,this->mqtt_nightmode ? false : true,this->mqtt_brightness);
    }
    if (!isSameColor(Config.fg,this->fg)) {
      this->fg=Config.fg;
      this->UpdateMQTTColorDimmer(FOREGROUNDNAME, this->fg);
    }
    if (!isSameColor(Config.bg,this->bg)) {
      this->bg=Config.bg;
      this->UpdateMQTTColorDimmer(BACKGROUNDNAME, this->bg);
    }
    if (!isSameColor(Config.s,this->s)) {
      this->s=Config.s;
      this->UpdateMQTTColorDimmer(SECONDSNAME, this->s);
    }
  }
}

//---------------------------------------------------------------------------------------
// CommandTopic
//
// Returns a string with the commandtopic for a devicename
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
String DimmerCommandTopic(const char* DeviceName)
{
  return String(Config.hostname)+String("/light/")+String(DeviceName)+String("/set");
}

//---------------------------------------------------------------------------------------
// PublishMQTTDimmer
//
// Publish autodiscoverymessage for MQTT Dimmer switch
//
// -> --
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::PublishMQTTDimmer(const char* uniquename, bool SupportRGB)
{
  Serial.println("PublishMQTTDimmer");
  StaticJsonDocument<512> json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = DimmerCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+"/light/"+String(uniquename)+"/state";
  json["schema"] = "json";
  json["brightness"] = true;
  if (SupportRGB) {
    json["supported_color_modes"][0] = "rgb";
  }
  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+"/light/"+String(Config.hostname)+"/"+String(uniquename)+"/config").c_str(),conf,Config.mqttpersistence);

  // Make sure we receive commands
  MQ.subscribe(DimmerCommandTopic(uniquename).c_str());
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
// UpdateMQTTDimmer
//
// Update an MQTT Dimer switch
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTColorDimmer(const char* uniquename, palette_entry Color)
{
  Serial.println("UpdateMQTTDimmer");
  StaticJsonDocument<512> json;
  uint8_t brightness = MaxColor(Color);

  // Construct JSON config message
  json["color_mode"] = "rgb";
  json["brightness"]=brightness;
  JsonObject color = json.createNestedObject("color");
  if (brightness==0) {
    json["state"] = "OFF";  
    color["r"] = 0;
    color["g"] = 0;
    color["b"] = 0;
  } else {
    json["state"]="ON";
    color["r"] = Color.r * 255 / brightness;
    color["g"] = Color.g * 255 / brightness;
    color["b"] = Color.b * 255 / brightness;
  }

  char state[512];
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
  this->PublishMQTTDimmer(Config.hostname,false);
  this->PublishMQTTDimmer(FOREGROUNDNAME,true);
  this->PublishMQTTDimmer(BACKGROUNDNAME,true);
  this->PublishMQTTDimmer(SECONDSNAME,true);
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
  if (Config.usemqtt && (this->lastconnectcheck<millis()-CONNECTTIMEOUT)) 
  {
    this->lastconnectcheck=millis();
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

        // Make sure MQTT cache has different values in cache, so communication is forced in next loop.
        this->mqtt_brightness = Brightness.brightnessOverride==50 ? 51 : 50;  
        this->mqtt_nightmode = Config.nightmode ? false : true ;
        if (isSameColor(Config.fg,{0,0,0})) {
          this->fg={1,1,1};
        } else {
          this->fg={0,0,0};
        }       
        if (isSameColor(Config.bg,{0,0,0})) {
          this->bg={1,1,1};
        } else {
          this->bg={0,0,0};
        } 
        if (isSameColor(Config.s,{0,0,0})) {
          this->s={1,1,1};
        } else {
          this->s={0,0,0};
        }       

      } else {
        Serial.print("failed, rc=");
        Serial.print(MQ.state());
      }
    }
  } 
}



//---------------------------------------------------------------------------------------
// MQTTCallback
//
// handle callbacks (currently empty)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::MQTTcallback(char* topic, byte* payload, unsigned int length) 
{
  // get vars from callback
  String topicstr=String(topic);
  char payloadstr[256];
  strncpy(payloadstr,(char *)payload,length);
  payloadstr[length]='\0';

  // decode payload
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payloadstr);

  if (error) {
    MQ.publish("log/topic",topicstr.c_str());
    MQ.publish("log/payload",payloadstr);
    MQ.publish("log/length",String(length).c_str());
    MQ.publish("log/error","Deserialisation failed");
  } else {
    // main switch: The name of the light = config.hostname 
    if (topicstr.equals(DimmerCommandTopic(Config.hostname) ) ) {
      // we have a match: let's decode
      if (doc.containsKey("state")) Config.nightmode = String(doc["state"]).equals("ON") ? false: true;
      if (doc.containsKey("brightness")) Brightness.brightnessOverride = doc["brightness"];
    } else {
      MQ.publish("log/topic",topicstr.c_str());
      MQ.publish("log/payload",payloadstr);
      MQ.publish("log/length",String(length).c_str());
      MQ.publish("log/command","unknown topic");
    }
  }
  Config.saveDelayed();
}
