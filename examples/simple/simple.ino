#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <webradio.h>
#include <SparkFun_MMA8452Q.h>

WebRadio radio;
char metaName[128];
char metaURL[128];

char configString[1024];


MMA8452Q accel;


bool readConfig()
{

  // Delete file (for testing only):
//  SPIFFS.remove("/cl_conf.txt");
  
  // open file for reading.
  File configFile = SPIFFS.open("/cl_conf.txt", "r");
  if (!configFile) {
    Serial.println("Failed to open cl_conf.txt.");
    configFile = SPIFFS.open("/cl_conf.txt", "w");
    if (!configFile) {
      Serial.println("Failed to open cl_conf.txt for writing");
      return false;
    }
    // Init config file:
    StaticJsonBuffer<200> jsonBuffer;

/*
    JsonObject& root = jsonBuffer.createObject();
    root["sensor"] = "gps";
    root["time"] = 1351824120;
    JsonArray& data = root.createNestedArray("s");
    data.add("stream-uk1.radioparadise.com");
    data.add("/mp3-128");
    data.add(80);
*/

    JsonObject& root = jsonBuffer.createObject();
    root["nradios"] = 1;
    JsonArray& stations = root.createNestedArray("stations");
    {
      JsonObject& station = stations.createNestedObject();
      station["name"] = "Paradise";
      station["addr"] = "stream-uk1.radioparadise.com";
      station["url"] = "/mp3-128";
      station["port"] = 80;
    }
    {
      JsonObject& station = stations.createNestedObject();
//      station = stations.createNestedObject();
      station["name"] = "Xpto";
      station["addr"] = "87.230.103.107";
      station["url"] = "/top100station.mp3";
      station["port"] = 80;
    }
    
    root.printTo(configString, sizeof(configString));
//    strcpy(configString, "bla bla");
    configFile.write((uint8_t*)configString, strlen(configString));
    configFile.close();
    configFile = SPIFFS.open("/cl_conf.txt", "r");
    if (!configFile) {
      Serial.println("Failed again to open cl_conf.txt");
      return false;
    }
  }
  // Read config file
  configFile.readBytes(configString, configFile.size());
  Serial.println("ConfigFile: ");
  Serial.println(configString);

  {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(configString);
    JsonArray& stations = root["stations"];
    Serial.println((int)root["nstations"]);
    Serial.println((const char*)stations[0]["name"]);
    Serial.println((const char*)stations[1]["name"]);
  }
  return true;
}


void setup()
{

  Serial.begin(115200); // Start Serial 
  Serial.setTimeout(5);
  Serial.println("HW Init...");
  radio.Init(16, 15, 4, 5);


  // Initialize file system.
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  readConfig();

  Serial.println("WiFi Init...");
  radio.WiFiConnect("xpto-net", "");
  Serial.println("Radio connect...");
//  radio.Connect("87.230.103.107", "/top100station.mp3", 80);
  radio.Connect("stream-uk1.radioparadise.com", "/mp3-128", 80);


  Wire.pins(0, 2);
  Wire.begin();
  StartUp_OLED(); // Init Oled and fire up!
  Serial.println("OLED Init...");
  clear_display();
  sendStrXY(" DANBICKS WIFI ", 0, 1); // 16 Character max per line with font set
  sendStrXY("   SCANNER     ", 2, 1);
  sendStrXY("START-UP ....  ", 4, 1);

//  accel.init();
    accel.init(SCALE_8G, ODR_200);
//  accel.setupTap(0x08, 0x08, 0x08);
}

void loop()
{
int ret;

  ret = radio.Loop(metaName, 0);
  if (ret == 1) {
    Serial.println(metaName);
//    Serial.println(metaURL);
  } else if (ret == -1) {
    Serial.println("RET -1");
    radio.Reconnect();
  }

  if(Serial.available()){
    int temp = Serial.parseInt();
    Serial.print("Input: "); Serial.println(temp);
    if(temp == 1) {
       radio.Connect("stream-uk1.radioparadise.com", "/mp3-128", 80);
    } else if(temp == 2) {  
       radio.Connect("icecast.omroep.nl", "/3fm-bb-mp3", 80);
    } else if(temp == 3) {  
       radio.Connect("stream-uk1.radioparadise.com", "/mp3-192", 80);
    } else if(temp == 4) {
       radio.SetVolume(0);
    } else if(temp == 5) {
       radio.Connect("stream-uk1.radioparadise.com", "/aac-128", 80);
    } else if(temp == 6) {
       radio.PrintDetails();
    } else if(temp == 7) {
       radio.PrintDebug();
    } else if(temp == 8) {  
       // Shoutcast GotRadio - Jazz So True
       radio.Connect("192.152.23.242", "/", 8450);    // AAC 64kbps, d? problemas com o VS1053...
       // radio.Connect("206.217.213.236", "/", 8450);   // MP3 128kbps, ok
    } else if(temp == 9) {  
//       radio.Connect("ruc1.cidadedecoimbra.com", "", 8000);
       radio.Connect("87.230.103.107", "/top100station.mp3", 80);
    } else if(temp == 10) {  
       // Shoutcast
       radio.Connect("streaming.radionomy.com", "/-BACO-LIBROS-Y-CAF--RADIO", 80);    
    } else if(temp == 17) {  
       radio.AdjustRate(-187000);
    } else if(temp == 18) {  
       radio.AdjustRate(511999);
//       radio.SetClock(100);
    } else if(temp == 19) {  
       radio.AdjustRate(0);
//       radio.SetClock(0);
    } else if(temp > 20) {
       radio.SetVolume(temp);
    }
  }
  int x = accel.readTap();

  if (x&0x10) {
    if (x&0x8)
      Serial.print("    Double Tap (2) on X");  // tabbing here for visibility 
    else 
      Serial.print("Single (1) tap on X"); 
    if (x & 0x01)  { // If PoIX is set 
      Serial.println(" -"); 
    } else { 
      Serial.println(" +"); 
    } 
  }
  if (x&0x20) {
    if (x&0x8)
      Serial.print("    Double Tap (2) on Y");  // tabbing here for visibility 
    else 
      Serial.print("Single (1) tap on Y"); 
    if (x & 0x02)  { // If PoIX is set 
      Serial.println(" -"); 
    } else { 
      Serial.println(" +"); 
    } 
  }
  if (x&0x40) {
    if (x&0x8)
      Serial.print("    Double Tap (2) on Z");  // tabbing here for visibility 
    else 
      Serial.print("Single (1) tap on Z"); 
    if (x & 0x04)  { // If PoIX is set 
      Serial.println(" -"); 
    } else { 
      Serial.println(" +"); 
    } 
  }
   
/*     
   if ((x & 0x10)==0x10)  // If AxX bit is set 
   { 
     if ((x & 0x08)==0x08)  // If DPE (double puls) bit is set 
       Serial.print("    Double Tap (2) on X");  // tabbing here for visibility 
     else 
       Serial.print("Single (1) tap on X"); 

     if ((x & 0x01)==0x01)  { // If PoIX is set 
       Serial.println(" -"); 
     } else { 
       Serial.println(" +"); 
     } 
   } 
   if ((x & 0x20)==0x20)  // If AxY bit is set 
   { 
     if ((x & 0x08)==0x08)  // If DPE (double pulse) bit is set 
       Serial.print("    Double Tap (2) on Y"); 
     else 
       Serial.print("Single (1) tap on Y"); 
 
     if ((x & 0x02)==0x02) { // If PoIY is set 
       Serial.println(" -"); 
     } else { 
       Serial.println(" +"); 
     } 
   } 
   if ((x & 0x40)==0x40)  // If AxZ bit is set 
   { 
     if ((x & 0x08)==0x08)  // If DPE (double puls) bit is set 
       Serial.print("    Double Tap (2) on Z"); 
     else 
       Serial.print("Single (1) tap on Z"); 
     if ((x & 0x04)==0x04) { // If PoIZ is set 
       Serial.println(" -");  
     } else { 
       Serial.println(" +"); 
   } 
*/
}
