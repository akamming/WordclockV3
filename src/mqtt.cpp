//MQTTClass
// by Arnold
#ifdef ESP32
#include <Wifi.h>
#include <WebServer.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif
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
  MQ.setBufferSize(2048); // discovery messages are longer than default max buffersize(!)
  MQ.setCallback(MQTTcallback); // listen to callbacks
  this->lastconnectcheck = millis()-CONNECTTIMEOUT-10; // force try to connect immediately
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
  this->reconnect();
  if ((millis()-this->lastmqttpublication)>PUBLISHTIMEOUT) {
    this->PublishAllMQTTSensors();
  }
  if (MQ.connected()) {
    // Check if values changed, and if yes: communicate
    if (this->mqtt_brightness!=Brightness.brightnessOverride or this->mqtt_nightmode != Config.nightmode) {
      this->mqtt_brightness=Brightness.brightnessOverride;
      this->mqtt_nightmode= Config.nightmode;
      this->UpdateMQTTDimmer(Config.hostname,this->mqtt_nightmode ? false : true,this->mqtt_brightness);
    }
    if (this->mqtt_animspeed!=Config.animspeed) {
      this->mqtt_animspeed=Config.animspeed;
      this->UpdateMQTTNumber(ANIMATIONSPEEDNAME, this->mqtt_animspeed);
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
    if (this->mqttDisplayMode!=Config.defaultMode) {
      this->mqttDisplayMode=Config.defaultMode;
      this->UpdateMQTTModeSelector(MODENAME,this->mqttDisplayMode);
    }
    if (this->debugging != this->mqtt_debugging) {
      this->UpdateMQTTSwitch(DEBUGNAME,debugging);
      mqtt_debugging=debugging;
    }
  }
}

//---------------------------------------------------------------------------------------
// Debug
//
// publishes a debug message
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::Debug(const char* status)
{
  if (MQ.connected() and this->debugging) this->UpdateMQTTText(DEBUGNAME,status);
}


//---------------------------------------------------------------------------------------
// PublishStatus
//
// publishes a status message
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::PublishStatus(const char* status)
{
  if (MQ.connected()) MQ.publish((String(Config.hostname)+"/status").c_str(),status,Config.mqttpersistence);
}


//---------------------------------------------------------------------------------------
// DimmerCommandTopic
//
// Returns a string with the Dimmer commandtopic for a devicename
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
String DimmerCommandTopic(const char* DeviceName)
{
  return String(Config.hostname)+String("/light/")+String(DeviceName)+String("/set");
}

//---------------------------------------------------------------------------------------
// NumberCommandTopic
//
// Returns a string with the Number commandtopic for a devicename
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
String NumberCommandTopic(const char* DeviceName)
{
  return String(Config.hostname)+String("/number/")+String(DeviceName)+String("/set");
}

//---------------------------------------------------------------------------------------
// SwitchCommandTopic
//
// Returns a string with the Switch commandtopic for a devicename
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
String SwitchCommandTopic(const char* DeviceName)
{
  return String(Config.hostname)+String("/switch/")+String(DeviceName)+String("/set");
}

//---------------------------------------------------------------------------------------
// TextCommandTopic
//
// Returns a string with the Text commandtopic for a devicename
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
String TextCommandTopic(const char* DeviceName)
{
  return String(Config.hostname)+String("/text/")+String(DeviceName)+String("/set");
}

//---------------------------------------------------------------------------------------
// SelectorCommandTopic
//
// Returns a string with the Dimmer commandtopic for a devicename
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
String SelectorCommandTopic(const char* DeviceName)
{
  return String(Config.hostname)+String("/select/")+String(DeviceName)+String("/set");
}

//---------------------------------------------------------------------------------------
// addDeviceToJson
//
// Add device to discovery message
//
// -> --
//    *json : Adress of the json object you are modiging
// <- --
//---------------------------------------------------------------------------------------
void addDeviceToJson(JsonDocument *json) {
  JsonObject dev = (*json)["dev"].to<JsonObject>();
  String MAC = WiFi.macAddress();
  MAC.replace(":", "");
  dev["ids"] = MAC;
  dev["name"] = Config.hostname;
  dev["sw"] = String(Config.hostname)+"_"+String(__DATE__)+"_"+String(__TIME__);
  dev["mdl"] = "d1_mini";
  dev["mf"] = "espressif";
}

//---------------------------------------------------------------------------------------
// PublishMQTTModeSelect
//
// Publish autodiscoverymessage for MQTT Selector
//
// -> --
//    uniquename = name of the switch
//    options = array of const char* with the options in the selector 
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::PublishMQTTModeSelect(const char* uniquename)
{
  Serial.println("PublishMQTTModeSelect");
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = SelectorCommandTopic(uniquename); 
  json["stat_t"] = String(Config.hostname)+"/select/"+String(uniquename)+"/state";
  json["platform"] = "select";
  // json["val_tpl"] = "{{ value_json.mode }}";

  JsonArray options = json["options"].to<JsonArray>();
  options.add("plain");
  options.add("fade");  
  options.add("flyingLettersVerticalUp");
  options.add("flyingLettersVerticalDown");
  options.add("explode");
  options.add("plasma");
  options.add("wakeup");
  options.add("matrix");
  options.add("heart");
  options.add("fire");
  options.add("stars");
  options.add("random");
  options.add("HorizontalStripes");
  options.add("VerticalStripes");
  options.add("RandomDots");
  options.add("RandomStripes");
  options.add("RotatingLine");
  options.add("ChristmasTree");
  options.add("ChristmasStar");
  options.add("red");
  options.add("green");
  options.add("blue");
  options.add("yellowHourglass");
  options.add("greenHourglass");
  options.add("update");
  options.add("updateComplete");
  options.add("updateError");
  options.add("wifiManager");

  addDeviceToJson(&json); // Add Device details to discovery message

  char conf[1024];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+"/select/"+String(Config.hostname)+"/"+String(uniquename)+"/config").c_str(),conf,Config.mqttpersistence);

  // Make sure we receive commands
  MQ.subscribe(SelectorCommandTopic(uniquename).c_str());
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
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = DimmerCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+"/light/"+String(uniquename)+"/state";
  // json["avty_t"] =  String(Config.hostname)+"/status",
  json["schema"] = "json";
  json["brightness"] = true;
  if (SupportRGB) {
    json["supported_color_modes"][0] = "rgb";
  } else {
    json["supported_color_modes"][0] = "brightness";
  }

  addDeviceToJson(&json); // Add Device details to discovery message

  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+"/light/"+String(Config.hostname)+"/"+String(uniquename)+"/config").c_str(),conf,Config.mqttpersistence);

  // Make sure we receive commands
  MQ.subscribe(DimmerCommandTopic(uniquename).c_str());
}

//---------------------------------------------------------------------------------------
// PublishMQTTNumber
//
// Publish autodiscoverymessage for MQTT Number
//
// -> --
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::PublishMQTTNumber(const char* uniquename, int min, int max, float step, bool isSlider)
{
  Serial.println("PublishMQTTNumber");
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = NumberCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+"/number/"+String(uniquename)+"/state";
  json["min"] = min;
  json["max"] = max;
  json["step"] = step;
  if (isSlider) {
    json["mode"] = "slider"; 
  } else {
    json["mode"] = "box";
  }


  addDeviceToJson(&json); // Add Device details to discovery message

  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+"/number/"+String(Config.hostname)+"/"+String(uniquename)+"/config").c_str(),conf,Config.mqttpersistence);

  // Make sure we receive commands
  MQ.subscribe(NumberCommandTopic(uniquename).c_str());
}

//---------------------------------------------------------------------------------------
// PublishMQTTText
//
// Publish autodiscoverymessage for MQTT Text
//
// -> --
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::PublishMQTTText(const char* uniquename)
{
  Serial.println("PublishMQTTNumber");
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = TextCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+"/text/"+String(uniquename)+"/state";


  addDeviceToJson(&json); // Add Device details to discovery message

  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+"/text/"+String(Config.hostname)+"/"+String(uniquename)+"/config").c_str(),conf,Config.mqttpersistence);

  // Make sure we receive commands
  MQ.subscribe(TextCommandTopic(uniquename).c_str());
}




//---------------------------------------------------------------------------------------
// PublishMQTTNumber
//
// Publish autodiscoverymessage for MQTT Number
//
// -> --
// <- --
//---------------------------------------------------------------------------------------

void MqttClass::PublishMQTTSwitch(const char* uniquename)
{
  Serial.println("PublishMQTTSwitch");
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = SwitchCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+"/switch/"+String(uniquename)+"/state";

  addDeviceToJson(&json);

  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+"/switch/"+String(Config.hostname)+"/"+String(uniquename)+"/config").c_str(),conf,Config.mqttpersistence);

  // subscribe if need to listen to commands
  MQ.subscribe(SwitchCommandTopic(uniquename).c_str());
}


//---------------------------------------------------------------------------------------
// UpdateMQTTModeSelector
//
// Update an MQTT Mode Selector
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTModeSelector(const char* uniquename, DisplayMode mode)
{
  Serial.println("UpdateMQTTModeSelector");

  String displaymode;
  
  switch(mode)
  {
  case DisplayMode::plain:
    displaymode="plain";
    break;
  case DisplayMode::fade:
    displaymode="fade"; 
    break;
  case DisplayMode::flyingLettersVerticalUp:
    displaymode="flyingLettersVerticalUp"; 
    break;
  case DisplayMode::flyingLettersVerticalDown:
    displaymode="flyingLettersVerticalDown"; 
    break;
  case DisplayMode::explode:
    displaymode="explode"; 
    break;
  case DisplayMode::wakeup:
    displaymode="wakeup"; 
    break;
  case DisplayMode::matrix:
    displaymode="matrix"; 
    break;
  case DisplayMode::heart:
    displaymode="heart"; 
    break;
  case DisplayMode::fire:
    displaymode="fire"; 
    break;
  case DisplayMode::stars:
    displaymode="stars"; 
    break;
  case DisplayMode::random:
    displaymode="random"; 
    break;
  case DisplayMode::HorizontalStripes:
    displaymode="HorizontalStripes"; 
    break;
  case DisplayMode::VerticalStripes:
    displaymode="VerticalStripes"; 
    break;
  case DisplayMode::RandomDots:
    displaymode="RandomDots"; 
    break;
  case DisplayMode::RandomStripes:
    displaymode="RandomStripes"; 
    break;
  case DisplayMode::RotatingLine:
    displaymode="RotatingLine"; 
    break;
  case DisplayMode::plasma:
    displaymode="plasma"; 
    break;
  case DisplayMode::red:
    displaymode="red"; 
    break;
  case DisplayMode::green:
    displaymode="green"; 
    break;
  case DisplayMode::blue:
    displaymode="blue"; 
    break;
  case DisplayMode::yellowHourglass:
    displaymode="yellowHourglass"; 
    break;
  case DisplayMode::greenHourglass:
    displaymode="greenHourglass"; 
    break;
  case DisplayMode::update:
    displaymode="update"; 
    break;
  case DisplayMode::updateComplete:
    displaymode="updateComplete"; 
    break;
  case DisplayMode::updateError:
    displaymode="updateError"; 
    break;
  case DisplayMode::wifiManager:
    displaymode="wifiManager"; 
    break;
  case DisplayMode::christmastree:
    displaymode="christmastree"; 
    break;
  case DisplayMode::christmasstar:
    displaymode="christmasstar"; 
    break;
  default:
    displaymode="unknown"; 
    break;
  }

  // publish state message
  MQ.publish((String(Config.hostname)+"/select/"+String(uniquename)+"/state").c_str(),displaymode.c_str(),Config.mqttpersistence);
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
  JsonDocument json;

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
// UpdateMQTTNumber
//
// Update an MQTT Number
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTNumber(const char* uniquename, uint8_t Mod)
{
  Serial.println("UpdateMQTTNumber");

  // publish state message
  MQ.publish((String(Config.hostname)+"/number/"+String(uniquename)+"/state").c_str(),String(Mod).c_str(),Config.mqttpersistence);
}

//---------------------------------------------------------------------------------------
// UpdateMQTTSwitch
//
// Update an MQTT Switch
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTSwitch(const char* uniquename, bool Value)
{
  Serial.println("UpdateMQTTSwitch");

  // publish state message
  MQ.publish((String(Config.hostname)+"/switch/"+String(uniquename)+"/state").c_str(),Value?"ON":"OFF",Config.mqttpersistence);
}


//---------------------------------------------------------------------------------------
// UpdateMQTTText
//
// Update an MQTT Text
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTText(const char* uniquename, const char* text)
{
  Serial.println("UpdateMQTTText");
 
  // publish state message
  MQ.publish((String(Config.hostname)+"/text/"+String(uniquename)+"/state").c_str(),text,Config.mqttpersistence);
}


//---------------------------------------------------------------------------------------
// UpdateMQTTColorDimmer
//
// Update an MQTT Dimer switch
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTColorDimmer(const char* uniquename, palette_entry Color)
{
  Serial.println("UpdateMQTTColorDimmer");
  JsonDocument json;
  uint8_t brightness = MaxColor(Color);

  // Construct JSON config message
  json["color_mode"] = "rgb";
  json["brightness"]=brightness;
  JsonObject color = json["color"].to<JsonObject>();
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
  if (MQ.connected()) {
    // make sure don't publish to often
    this->lastmqttpublication=millis();

    // let the environment know we're online
    this->PublishStatus("online");

    // publish the autodiscovery messages
    this->PublishMQTTDimmer(Config.hostname,false);
    this->PublishMQTTDimmer(FOREGROUNDNAME,true);
    this->PublishMQTTDimmer(BACKGROUNDNAME,true);
    this->PublishMQTTDimmer(SECONDSNAME,true);
    this->PublishMQTTNumber(ANIMATIONSPEEDNAME,1,100,1,true);
    this->PublishMQTTModeSelect(MODENAME);
    this->PublishMQTTText(DEBUGNAME);
    this->PublishMQTTSwitch(DEBUGNAME);

    // Trick the program to communicate in the next run by making sure the mqtt cached values are set to the "wrong" values
    this->mqtt_brightness = Brightness.brightnessOverride==50 ? 51 : 50;
    this->mqtt_animspeed = Config.animspeed==50 ? 51 : 50;  
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
    this->mqttDisplayMode = Config.defaultMode == DisplayMode::fade ? DisplayMode::explode : DisplayMode::fade; 
  }
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
        this->Debug("Connect succeeded");
        this->PublishAllMQTTSensors();

      } else {
        Serial.print("failed, rc=");
        Serial.print(MQ.state());
      }
    }
  } 
}

//---------------------------------------------------------------------------------------
// ProcessColorCommand
//
// Calculate new value based on old value and command
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
palette_entry ProcessColorCommand(palette_entry OldColor, char* payloadstr)
{
  palette_entry NewColor;

  // decode payload
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payloadstr);

  if (error) {
    MQ.publish("log/payload",payloadstr);
    MQ.publish("log/error","Deserialisation failed");
  } else {
    if (doc["state"].is<const char*>() && String(doc["state"]).equals("OFF")) {
      NewColor = {0,0,0}; // switch off if we have an off command     
    } else {
      // see if we have to process the color, at least remember old brightness and maxcolor
      uint8_t brightness = MaxColor(OldColor);
      uint8_t maxcolor = brightness;

      // overrule brightness if specified
      if (doc["brightness"].is<unsigned int>()) brightness=doc["brightness"];

      // check if we have to update the color and maxcolor
      if (doc["color"].is<JsonObject>()) {
        JsonObject color = doc["color"];
        NewColor={color["r"],color["g"],color["b"]};
        maxcolor=MaxColor(NewColor); // reset maxcolor to new value
      } else {
        NewColor=OldColor;
      }

      // do the final colorcorrection based on brightness
      if (maxcolor==0) { 
        // prevent division by zero
        NewColor={brightness,brightness,brightness};
      } else {
        //update the brightness if we received a new one
        // apply the new brightness
        NewColor={(uint8_t)(NewColor.r*brightness/maxcolor),(uint8_t)(NewColor.g*brightness/maxcolor),(uint8_t)(NewColor.b*brightness/maxcolor)};
      }
    }
  }
  return NewColor;
} 

//---------------------------------------------------------------------------------------
// 
//
// Calculate new value based on old value and command
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
DisplayMode GetDisplayModeFromPayload(String payload)
{
  if (payload=="plain") {
    return DisplayMode::plain; 
  } else if (payload=="fade") {
    return DisplayMode::fade;
  } else if (payload=="flyingLettersVerticalUp") {
    return DisplayMode::flyingLettersVerticalUp;
  } else if (payload=="flyingLettersVerticalDown") {
    return DisplayMode::flyingLettersVerticalDown; 
  } else if (payload=="explode") {
    return DisplayMode::explode;
  } else if (payload=="plasma") {
    return DisplayMode::plasma;
  } else if (payload=="wakeup") {
    return DisplayMode::wakeup;
  } else if (payload=="matrix") {
    return DisplayMode::matrix;
  } else if (payload=="heart") {
    return DisplayMode::heart;
  } else if (payload=="fire") {
    return DisplayMode::fire;
  } else if (payload=="stars") {
    return DisplayMode::stars;
  } else if (payload=="random") {
    return DisplayMode::random;
  } else if (payload=="HorizontalStripes") {
    return DisplayMode::HorizontalStripes;
  } else if (payload=="VerticalStripes") {
    return DisplayMode::VerticalStripes;
  } else if (payload=="RandomDots") {
    return DisplayMode::RandomDots;
  } else if (payload=="RandomStripes") {
    return DisplayMode::RandomStripes;
  } else if (payload=="RotatingLine") {
    return DisplayMode::RotatingLine;
  } else if (payload=="ChristmasTree") {
    return DisplayMode::christmastree;
  } else if (payload=="ChristmasStar") {
    return DisplayMode::christmasstar;
  } else if (payload=="red") {
    return DisplayMode::red;
  } else if (payload=="green") {
    return DisplayMode::green;
  } else if (payload=="blue") {
    return DisplayMode::blue;
  } else if (payload=="yellowHourglass") {
    return DisplayMode::yellowHourglass;
  } else if (payload=="greenHourglass") {
    return DisplayMode::greenHourglass;
  } else if (payload=="update") {
    return DisplayMode::update;
  } else if (payload=="updateComplete") {
    return DisplayMode::updateComplete;
  } else if (payload=="updateError") {
    return DisplayMode::updateError;
  } else if (payload=="wifiManager") {
    return DisplayMode::wifiManager;
  } else  {
    Serial.println("Unknown display mode received by mqtt");
    return DisplayMode::plain;
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

  // main switch: The name of the light = config.hostname 
  if (topicstr.equals(DimmerCommandTopic(Config.hostname) ) ) {
    // decode payload
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payloadstr);

    if (error) {
      MQ.publish("log/topic",topicstr.c_str());
      MQ.publish("log/payload",payloadstr);
      MQ.publish("log/length",String(length).c_str());
      MQ.publish("log/error","Deserialisation failed");
    } else {
      // we have a match: let's decode
      if (doc["brightness"].is<unsigned int>()) {
        Brightness.brightnessOverride = doc["brightness"];
        if (Brightness.brightnessOverride>0) Config.nightmode = false; // undo nightmode when a brightness level >0 is set
      }
      if (doc["state"].is<const char*>()) Config.nightmode = String(doc["state"]).equals("ON") ? false: true;
    } 
  } else if (topicstr.equals(NumberCommandTopic(ANIMATIONSPEEDNAME))) {
    Config.animspeed = String(payloadstr).toInt();
  } else if (topicstr.equals(DimmerCommandTopic(FOREGROUNDNAME) ) ) {
    Config.fg=ProcessColorCommand(Config.fg, payloadstr); 
  } else if (topicstr.equals(DimmerCommandTopic(BACKGROUNDNAME) ) ) {
    Config.bg=ProcessColorCommand(Config.bg, payloadstr); 
  } else if (topicstr.equals(DimmerCommandTopic(SECONDSNAME) ) ) {
    Config.s=ProcessColorCommand(Config.s, payloadstr); 
  } else if (topicstr.equals(SelectorCommandTopic(MODENAME) ) ) {
    Config.defaultMode = GetDisplayModeFromPayload(payloadstr);
  } else if (topicstr.equals(SwitchCommandTopic(DEBUGNAME) ) ) {
    if (String(payloadstr).equals("ON")) {
      MQTT.debugging=true;
     } else {
      MQTT.debugging=false;
    } 
  } else {
      MQ.publish("log/topic",topicstr.c_str());
      MQ.publish("log/payload",payloadstr);
      MQ.publish("log/length",String(length).c_str());
      MQ.publish("log/command","unknown topic");
  } Config.saveDelayed();
}