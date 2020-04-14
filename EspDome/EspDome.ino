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

// NTP
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//---------------------------------------Macros---------------------------------------------------

//---------------------------------------CONSTANTES-----------------------------------------------

//---------------------------------------VARIABLES GLOBALES---------------------------------------
int compte60 = 0;    // Compteur pour exécuter des commandes toutes les 10s
int compte10 = 0;   // Compteur pour exécuter des commandes toutes les 5s


//---------------------------------------SETUP-----------------------------------------------

void setup()
{
  // WiFi
  Serial.begin(9600);
  Serial.println("Booting");

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(STASSID,  STAPSK);
  //wifiMulti.addAP("onstep", STAPSK);
  wifiMulti.addAP("dehors", STAPSK);
  while ((wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Connection Failed! Restart...");
    delay(5000);
    //ESP.restart();
  }
  ArduinoOTA.setHostname("espdome");

  // Setup locally attached sensors
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd updating");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // NTP
  timeClient.begin();
  //timeClient.setTimeOffset(3600); On reste en GMT
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

void loop()
{
  String SerMsg;
  String ret;
  ArduinoOTA.handle();

  // Lecture des infos provenant de l'Arduino Nano
  if (Serial.available()) {
    SerMsg = Serial.readStringUntil(35);
    if (SerMsg == "PA") {
      ret = GetScopeInfo(":hP#");
      Serial.println(ret);
    }
    else if (SerMsg == "HO") {
      GetScopeInfo(":hC#");
      Serial.println("0");
    }
    else if (SerMsg == "FN") {
      ret = GetScopeInfo(":GVN#");
      Serial.println(ret);
    }
    /*
        else if (SerMsg == "DO") {
          send(msgD.set(0));
        }
        else if (SerMsg == "DF") {
          send(msgD.set(1));
        }
    */
  }
}

//---------------------------------------FONCTIONS--------------------------------------------

// Fonction executée toutes les secondes
void FuncSec() {
  compte10++;
  compte60++;
  // Toutes les 10s
  if (compte10 == 10) {
    compte10 = 0;
    // TODO Lecture de l'état de chauffe du miroir
    String Chauffe = GetScopeInfo(":GXG6#");
    /*    if (Chauffe != "0") {
          module.setLED(TM1638_COLOR_RED, 7);
        }
        else {
          module.setLED(TM1638_COLOR_NONE, 7);
        }
    */
  }
  // Toutes les 60s
  if (compte60 == 60) {
    // Fonctions exécutées toutes les 60s
    compte60 = 0;
    String Temp = GetScopeInfo(":GX9A#");	// T°
    String Hum = GetScopeInfo(":GX9C#");	// H%
  }
  // Toutes les 1s
}

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
