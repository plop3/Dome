/*
  Gestion de l'abri du telescope, connexion à OnSTep pour le Park
  Serge CLAUS
  GPL V3
  Version 1.0
  16/03/2020
*/

//---------------------------------------PERIPHERIQUES-----------------------------------------------

// WiFi OTA
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "WiFiP.h"
ESP8266WiFiMulti wifiMulti;

/*
// NTP
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
*/

//---------------------------------------Macros---------------------------------------------------
#define DEBUG_OFF

//---------------------------------------CONSTANTES-----------------------------------------------

//---------------------------------------VARIABLES GLOBALES---------------------------------------

//---------------------------------------SETUP-----------------------------------------------

void setup()
{
  // WiFi
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(STASSID,  STAPSK);
  //wifiMulti.addAP("onstep", STAPSK);
  wifiMulti.addAP("dehors", STAPSK);
  while ((wifiMulti.run() != WL_CONNECTED)) {
    delay(5000);
    //ESP.restart();
  }
  ArduinoOTA.setHostname("espdome");

  // Setup locally attached sensors
  ArduinoOTA.onStart([]() {
  });
  ArduinoOTA.onEnd([]() {
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
  });
  ArduinoOTA.begin();

  // NTP
  //timeClient.begin();
  //timeClient.setTimeOffset(3600); On reste en GMT
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

void loop() {
  String SerMsg;
  String ret;
  ArduinoOTA.handle();

  // Lecture des infos provenant de l'Arduino Nano
  if (Serial.available()) {
    SerMsg = Serial.readStringUntil(35);
    if (SerMsg == "PA") {
      ret = GetScopeInfo(":hP#");
      //Serial.println(ret);
    }
  }
}

//---------------------------------------FONCTIONS--------------------------------------------

// Récupération des infos de la carte Dome
String GetDomeInfo(String msg) {
  Serial.println(msg);
  unsigned long currentMillis = millis();
  unsigned long previousMillis = millis();
  while ((currentMillis - previousMillis < 1000) && !Serial.available())
  {
    currentMillis = millis();
  }
  return Serial.readStringUntil(35);
}

// Récupération/envoi des infos du télescope depuis OnStepESPServer
String GetScopeInfo(String msg) {
  String ret;
  WiFiClient client;
  if (!client.connect("192.168.0.15", 9999)) {
    delay(1000);
    return "Err ";
  }
  client.print(msg);
  unsigned long currentMillis = millis();
  unsigned long previousMillis = millis();
  while ((currentMillis - previousMillis < 1000) && !client.available())
  {
    currentMillis = millis();
  }
  ret = client.readStringUntil(35);
  //ret = client.readString();
  client.stop();
  return ret;
}
