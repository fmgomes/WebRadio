#include <ESP8266WiFi.h>
#include <SPI.h>
#include <webradio.h>

WebRadio radio;
char metaName[128];
char metaURL[128];

void setup()
{

  Serial.begin(115200); // Start Serial 
  Serial.setTimeout(5);
  Serial.println("HW Init...");
  radio.Init(0, 15, 4, 5);
  Serial.println("WiFi Init...");
  radio.WiFiConnect("xpto-net", "");
  Serial.println("Radio connect...");
//  radio.Connect("87.230.103.107", "/top100station.mp3", 80);
  radio.Connect("stream-uk1.radioparadise.com", "/mp3-128", 80);
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
       // radio.Connect("192.152.23.242", "/", 8450);    // AAC 64kbps, dá problemas com o VS1053...
       radio.Connect("206.217.213.236", "/", 8450);   // MP3 128kbps, ok
    } else if(temp == 9) {  
//       radio.Connect("ruc1.cidadedecoimbra.com", "", 8000);
       radio.Connect("87.230.103.107", "/top100station.mp3", 80);
    } else if(temp == 10) {  
       // Shoutcast
       radio.Connect("streaming.radionomy.com", "/-BACO-LIBROS-Y-CAF--RADIO", 80);    
    } else if(temp > 20) {
       radio.SetVolume(temp);
    }
  }
}