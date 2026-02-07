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
  this->mqtt_bluecorrection = 0;
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
    if (this->mqtt_bluecorrection!=Config.blueCorrection) {
      this->mqtt_bluecorrection=Config.blueCorrection;
      this->UpdateMQTTNumber(BLUECORRECTIONNAME, this->mqtt_bluecorrection);
    }
    if (this->mqtt_fg_enabled!=Config.fg_enabled) {
      this->mqtt_fg_enabled=Config.fg_enabled;
      this->UpdateMQTTColorDimmer(FOREGROUNDNAME, this->mqtt_fg_enabled);
    }
    // Always check and publish color RGB values if they changed (from GUI, webinterface, or external sources)
    if (!isSameColor(Config.fg, this->mqtt_fg_color)) {
      this->mqtt_fg_color=Config.fg;
      this->UpdateMQTTColorDimmer(FOREGROUNDNAME, this->mqtt_fg_enabled);
    }
    if (this->mqtt_bg_enabled!=Config.bg_enabled) {
      this->mqtt_bg_enabled=Config.bg_enabled;
      this->UpdateMQTTColorDimmer(BACKGROUNDNAME, this->mqtt_bg_enabled);
    }
    // Always check and publish color RGB values if they changed (from GUI, webinterface, or external sources)
    if (!isSameColor(Config.bg, this->mqtt_bg_color)) {
      this->mqtt_bg_color=Config.bg;
      this->UpdateMQTTColorDimmer(BACKGROUNDNAME, this->mqtt_bg_enabled);
    }
    if (this->mqtt_s_enabled!=Config.s_enabled) {
      this->mqtt_s_enabled=Config.s_enabled;
      this->UpdateMQTTColorDimmer(SECONDSNAME, this->mqtt_s_enabled);
    }
    // Always check and publish color RGB values if they changed (from GUI, webinterface, or external sources)
    if (!isSameColor(Config.s, this->mqtt_s_color)) {
      this->mqtt_s_color=Config.s;
      this->UpdateMQTTColorDimmer(SECONDSNAME, this->mqtt_s_enabled);
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
  if (MQ.connected()) MQ.publish((String(Config.hostname)+FPSTR(MQTT_STATUS)).c_str(),status,Config.mqttpersistence);
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
  return String(Config.hostname)+FPSTR(MQTT_LIGHT)+String(DeviceName)+FPSTR(MQTT_SET);
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
  return String(Config.hostname)+FPSTR(MQTT_NUMBER)+String(DeviceName)+FPSTR(MQTT_SET);
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
  return String(Config.hostname)+FPSTR(MQTT_SWITCH)+String(DeviceName)+FPSTR(MQTT_SET);
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
  return String(Config.hostname)+FPSTR(MQTT_TEXT)+String(DeviceName)+FPSTR(MQTT_SET);
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
  return String(Config.hostname)+FPSTR(MQTT_SELECT)+String(DeviceName)+FPSTR(MQTT_SET);
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
  Serial.println(F("PublishMQTTModeSelect"));
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = SelectorCommandTopic(uniquename); 
  json["stat_t"] = String(Config.hostname)+FPSTR(MQTT_SELECT)+String(uniquename)+FPSTR(MQTT_STATE);
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
  options.add("JingleBells");
  options.add("MerryChristmas");
  options.add("HappyNewYear");
  options.add("pong");
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
  size_t jsonSize = measureJson(json);
  if(jsonSize >= sizeof(conf)) {
    Serial.printf("JSON too large: %u bytes, buffer is %u\n", jsonSize, sizeof(conf));
    return; // Skip publishing to prevent crash
  }
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+FPSTR(MQTT_SELECT)+String(Config.hostname)+"/"+String(uniquename)+FPSTR(MQTT_CONFIG)).c_str(),conf,Config.mqttpersistence);

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
  Serial.println(F("PublishMQTTDimmer"));
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = DimmerCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+FPSTR(MQTT_LIGHT)+String(uniquename)+FPSTR(MQTT_STATE);
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
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+FPSTR(MQTT_LIGHT)+String(Config.hostname)+"/"+String(uniquename)+FPSTR(MQTT_CONFIG)).c_str(),conf,Config.mqttpersistence);

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
  Serial.println(F("PublishMQTTNumber"));
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = NumberCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+FPSTR(MQTT_NUMBER)+String(uniquename)+FPSTR(MQTT_STATE);
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
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+FPSTR(MQTT_NUMBER)+String(Config.hostname)+"/"+String(uniquename)+FPSTR(MQTT_CONFIG)).c_str(),conf,Config.mqttpersistence);

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
  Serial.println(F("PublishMQTTNumber"));
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = TextCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+FPSTR(MQTT_TEXT)+String(uniquename)+FPSTR(MQTT_STATE);


  addDeviceToJson(&json); // Add Device details to discovery message

  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+FPSTR(MQTT_TEXT)+String(Config.hostname)+"/"+String(uniquename)+FPSTR(MQTT_CONFIG)).c_str(),conf,Config.mqttpersistence);

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
  Serial.println(F("PublishMQTTSwitch"));
  JsonDocument json;

  // Construct JSON config message
  json["name"] = uniquename;
  json["unique_id"] = String(Config.hostname)+"_"+uniquename;
  json["cmd_t"] = SwitchCommandTopic(uniquename);
  json["stat_t"] = String(Config.hostname)+FPSTR(MQTT_SWITCH)+String(uniquename)+FPSTR(MQTT_STATE);

  addDeviceToJson(&json);

  char conf[512];
  serializeJson(json, conf);  // conf now contains the json

  // Publish config message
  MQ.publish((String(MQTTAUTODISCOVERYTOPIC)+FPSTR(MQTT_SWITCH)+String(Config.hostname)+"/"+String(uniquename)+FPSTR(MQTT_CONFIG)).c_str(),conf,Config.mqttpersistence);

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
  Serial.println(F("UpdateMQTTModeSelector"));

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
  case DisplayMode::jinglebells:
    displaymode="jinglebells"; 
    break;
  case DisplayMode::merryChristmas:
    displaymode="merryChristmas"; 
    break;
  case DisplayMode::happyNewYear:
    displaymode="happyNewYear"; 
    break;
  case DisplayMode::pong:
    displaymode="pong"; 
    break;
  default:
    displaymode="unknown"; 
    break;
  }

  // publish state message
  MQ.publish((String(Config.hostname)+FPSTR(MQTT_SELECT)+String(uniquename)+FPSTR(MQTT_STATE)).c_str(),displaymode.c_str(),Config.mqttpersistence);
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
  Serial.println(F("UpdateMQTTDimmer"));
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
  Serial.println(F("UpdateMQTTNumber"));

  // publish state message
  MQ.publish((String(Config.hostname)+FPSTR(MQTT_NUMBER)+String(uniquename)+FPSTR(MQTT_STATE)).c_str(),String(Mod).c_str(),Config.mqttpersistence);
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
  Serial.println(F("UpdateMQTTSwitch"));

  // publish state message
  MQ.publish((String(Config.hostname)+FPSTR(MQTT_SWITCH)+String(uniquename)+FPSTR(MQTT_STATE)).c_str(),Value?"ON":"OFF",Config.mqttpersistence);
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
  Serial.println(F("UpdateMQTTText"));
 
  // publish state message
  MQ.publish((String(Config.hostname)+FPSTR(MQTT_TEXT)+String(uniquename)+FPSTR(MQTT_STATE)).c_str(),text,Config.mqttpersistence);
}


//---------------------------------------------------------------------------------------
// UpdateMQTTColorDimmer
//
// Update an MQTT Color Dimmer with on/off state and brightness (for slider control)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MqttClass::UpdateMQTTColorDimmer(const char* uniquename, bool enabled)
{
  Serial.println(F("UpdateMQTTColorDimmer"));
  JsonDocument json;

  // Get the current color value to determine brightness for the slider
  palette_entry color;
  if (String(uniquename).equals(FOREGROUNDNAME)) {
    color = Config.fg;
  } else if (String(uniquename).equals(BACKGROUNDNAME)) {
    color = Config.bg;
  } else if (String(uniquename).equals(SECONDSNAME)) {
    color = Config.s;
  } else {
    color = {0, 0, 0};
  }
  
  uint8_t brightness = MaxColor(color);

  // Construct JSON config message with brightness for slider
  json["state"] = enabled ? "ON" : "OFF";
  json["brightness"] = brightness;
  
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
    this->PublishMQTTNumber(BLUECORRECTIONNAME,0,100,1,true);
    this->PublishMQTTModeSelect(MODENAME);
    this->PublishMQTTText(DEBUGNAME);
    this->PublishMQTTSwitch(DEBUGNAME);

    // Immediately publish current state values after autodiscovery
    this->UpdateMQTTDimmer(Config.hostname, !Config.nightmode, Brightness.brightnessOverride);
    this->UpdateMQTTColorDimmer(FOREGROUNDNAME, Config.fg_enabled);
    this->UpdateMQTTColorDimmer(BACKGROUNDNAME, Config.bg_enabled);
    this->UpdateMQTTColorDimmer(SECONDSNAME, Config.s_enabled);
    this->UpdateMQTTNumber(ANIMATIONSPEEDNAME, Config.animspeed);
    this->UpdateMQTTNumber(BLUECORRECTIONNAME, Config.blueCorrection);
    this->UpdateMQTTModeSelector(MODENAME, Config.defaultMode);
    this->UpdateMQTTSwitch(DEBUGNAME, debugging);

    // Trick the program to communicate in the next run by making sure the mqtt cached values are set to the "wrong" values
    this->mqtt_brightness = Brightness.brightnessOverride==50 ? 51 : 50;
    this->mqtt_animspeed = Config.animspeed==50 ? 51 : 50;  
    this->mqtt_bluecorrection = (Config.blueCorrection==50 ? 51 : 50);
    this->mqtt_nightmode = Config.nightmode ? false : true ;
    this->mqtt_fg_enabled = !Config.fg_enabled;
    this->mqtt_bg_enabled = !Config.bg_enabled;
    this->mqtt_s_enabled = !Config.s_enabled;
    this->mqtt_fg_color = {1, 1, 1};
    this->mqtt_bg_color = {1, 1, 1};
    this->mqtt_s_color = {1, 1, 1};
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
      Serial.print(F("Attempting MQTT connection..."));
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
        Serial.print(F("failed, rc="));
        Serial.print(MQ.state());
      }
    }
  } 
}

//---------------------------------------------------------------------------------------
// ProcessColorCommand
//
// Calculate new value based on old value and command, also returns enabled state
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
palette_entry ProcessColorCommand(palette_entry OldColor, char* payloadstr, bool& enabled_out)
{
  palette_entry NewColor = OldColor;  // Default: keep old color
  enabled_out = true;  // default to enabled

  // decode payload
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payloadstr);

  if (error) {
    MQ.publish("log/payload",payloadstr);
    MQ.publish("log/error","Deserialisation failed");
  } else {
    if (doc["state"].is<const char*>() && String(doc["state"]).equals("OFF")) {
      // Just disable, don't change the color
      enabled_out = false;
    } else {
      // State is ON or not specified - keep enabled
      enabled_out = true;
      
      // Only modify color if brightness or RGB values are provided
      uint8_t brightness = MaxColor(OldColor);
      uint8_t maxcolor = brightness;
      
      // Check if new brightness is specified
      if (doc["brightness"].is<unsigned int>()) {
        brightness = doc["brightness"];
        
        // If brightness is 0, just disable but keep the color
        if (brightness == 0) {
          enabled_out = false;
          return OldColor;  // Return old color unchanged
        }
      }

      // Check if new RGB values are provided
      if (doc["color"].is<JsonObject>()) {
        JsonObject color = doc["color"];
        NewColor = {color["r"], color["g"], color["b"]};
        maxcolor = MaxColor(NewColor);
        
        // If new color is 0,0,0 disable it
        if (maxcolor == 0) {
          enabled_out = false;
          return NewColor;
        }
      }

      // Apply brightness scaling if we have a new color or brightness change
      if (doc["brightness"].is<unsigned int>() || doc["color"].is<JsonObject>()) {
        if (maxcolor == 0) {
          // New color is 0,0,0 - disable
          enabled_out = false;
          NewColor = {0, 0, 0};
        } else if (doc["brightness"].is<unsigned int>()) {
          // Apply new brightness to existing color
          NewColor = {
            (uint8_t)(NewColor.r * brightness / maxcolor),
            (uint8_t)(NewColor.g * brightness / maxcolor),
            (uint8_t)(NewColor.b * brightness / maxcolor)
          };
        }
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
  } else if (payload=="JingleBells") {
    return DisplayMode::jinglebells;
  } else if (payload=="MerryChristmas") {
    return DisplayMode::merryChristmas;
  } else if (payload=="HappyNewYear") {
    return DisplayMode::happyNewYear;
  } else if (payload=="pong") {
    return DisplayMode::pong;
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
    Serial.println(F("Unknown display mode received by mqtt"));
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
  
  // Prevent buffer overflow: limit length to buffer size - 1
  if(length >= sizeof(payloadstr)) {
    Serial.printf("MQTT payload too large: %u bytes, truncating to %u\n", length, sizeof(payloadstr)-1);
    length = sizeof(payloadstr) - 1;
  }
  
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
  } else if (topicstr.equals(NumberCommandTopic(BLUECORRECTIONNAME))) {
    int value = String(payloadstr).toInt();
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    Config.blueCorrection = value;
  } else if (topicstr.equals(DimmerCommandTopic(FOREGROUNDNAME) ) ) {
    // Handle foreground color and enabled state via ProcessColorCommand
    bool new_enabled;
    Config.fg = ProcessColorCommand(Config.fg, payloadstr, new_enabled);
    Config.fg_enabled = new_enabled;
  } else if (topicstr.equals(DimmerCommandTopic(BACKGROUNDNAME) ) ) {
    // Handle background color and enabled state via ProcessColorCommand
    bool new_enabled;
    Config.bg = ProcessColorCommand(Config.bg, payloadstr, new_enabled);
    Config.bg_enabled = new_enabled;
  } else if (topicstr.equals(DimmerCommandTopic(SECONDSNAME) ) ) {
    // Handle seconds color and enabled state via ProcessColorCommand
    bool new_enabled;
    Config.s = ProcessColorCommand(Config.s, payloadstr, new_enabled);
    Config.s_enabled = new_enabled;
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