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

// Client Web
#include <ESP8266HTTPClient.h>
HTTPClient http;
WiFiClient client;

// Serveur Web
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// Serveur Web
ESP8266WebServer server ( 80 );

/*
  // NTP
  #include <NTPClient.h>
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);
*/

//---------------------------------------Macros---------------------------------------------------
#define DEBUG_OFF
#define DAbriID   3576  // ID domoticz du boudon dome

//---------------------------------------CONSTANTES-----------------------------------------------

//---------------------------------------VARIABLES GLOBALES---------------------------------------
String portes = "0"; // Portes fermées
String dome = "1"; // Abri fermé
//---------------------------------------SETUP-----------------------------------------------

void setup()
{
  // WiFi
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  IPAddress ip(192, 168, 0, 17);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 0, 1);
  IPAddress gateway(192, 168, 0, 1);
  WiFi.config(ip, gateway, subnet, dns);
   WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
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

  // Attente du démarrage de l'arduino Dome
  delay(2000);
  // Lecture des infos du dome
  portes = GetDomeInfo("PI#");delay(500);
  dome = GetDomeInfo("DI#");
  // Serveur Web
  server.begin();
  server.on ("/state", sendDomeState);
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

void loop() {
  String SerMsg;
  String ret;
  ArduinoOTA.handle();
  // Serveur Web
  server.handleClient();

  // Lecture des infos provenant de l'Arduino Nano
  if (Serial.available()) {
    SerMsg = Serial.readStringUntil(35);
    if (SerMsg == "PA") {
      ret = GetScopeInfo(":hP#");
      //Serial.println(ret);
    }
    else if (SerMsg == "DO") {
      sendDomoticz(DAbriID, "On");
      dome = "0";
    }
    else if (SerMsg == "DF") {
      sendDomoticz(DAbriID, "Off");
      dome = "1";
    }
    else if (SerMsg == "PO") {
      portes = "1";
    }
    else if (SerMsg == "PF") {
      portes = "0";
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
  String ret=Serial.readString();
  ret.trim();
  return ret;
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

// Envoi de l'état du dome à Domoticz
void sendDomoticz(int ID, String Etat) {
  http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=switchlight&idx=" + String(ID) + "&switchcmd=" + Etat);
  http.GET();
  http.end();
}

// Envoi les infos du dome (pour dome scripting gateway)
void sendDomeState() {
  server.send(200, "text/plain", dome + " " + portes + " 0");
}
