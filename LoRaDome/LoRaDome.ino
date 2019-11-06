/*
  Gestion LoRa de l'abri du telescope
  Serge CLAUS
  GPL V3
  Version 1.1
  22/10/2018-06/11/2019
*/

//---------------------------------------PERIPHERIQUES-----------------------------------------------

// WiFi OTA
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// TM1638 LEDs & Keys /!\ Librairie sur Ghitub plop3 (https://github.com/plop3/tm1638-library)
#include <TM1638.h>
TM1638 module(4, 17, 25); // (4, 17, 25)

// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enables and select radio type (if attached)
//#define MY_RADIO_RF24
//#define MY_RADIO_RFM69
#define MY_RADIO_RFM95
#define MY_RFM95_IRQ_PIN 26
#define MY_RFM95_RST_PIN 14
#define MY_REPEATER_FEATURE
//#define MY_GATEWAY_ESP32

#define MY_WIFI_SSID "astro"
#define MY_WIFI_PASSWORD "B546546AF0"

// Set the hostname for the WiFi Client. This is the hostname
// passed to the DHCP server if not static.
#define MY_HOSTNAME "domelora"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,1,100

// If using static ip you can define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,1,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0

// The port to keep open on node server mode
#define MY_PORT 5003

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2// Set blinking period


// Advanced Gateway Options
//#define MY_DEFAULT_LED_BLINK_PERIOD 50

// Flash leds on rx/tx/err
// Led pins used if blinking feature is enabled above
//#define MY_DEFAULT_ERR_LED_PIN 32  // Transfer data error led pin
//#define MY_DEFAULT_RX_LED_PIN  25  // Receive Data led pin
//#define MY_DEFAULT_TX_LED_PIN  27  // Transmit Data led pin

//#define MY_WITH_LEDS_BLINKING_INVERSE  // At the time of Error, Receive, Transmit the pin is at a high level

// MySensors
#include <MySensors.h>
MyMessage msgD(2, V_TRIPPED);  // Dome
MyMessage msgP(3, V_TRIPPED); // Portes
MyMessage msgT(4, V_STATUS);  // Télescope parqué

// NTP
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Timer
#include <SimpleTimer.h>
SimpleTimer timer;

//---------------------------------------Macros---------------------------------------------------

//---------------------------------------CONSTANTES-----------------------------------------------

//---------------------------------------VARIABLES GLOBALES---------------------------------------

// TM1638
byte NiveauAff = 1;		// Intensité de l'affichage
String formattedDate;	// Client NTP
int TypeAff = 1; 		// 0: Eteint, 1: Heure GMT, 2: T° /H%
bool StateAff = true;	// Etat de l'affichage (M/A par bouton 2)
int compte10 = 0;			// Compteur pour exécuter des commandes toutes les 10s
int compte5 = 0;			// Compteur pour exécuter des commandes toutes les 5s

//---------------------------------------SETUP-----------------------------------------------

void setup()
{
  // Afficheur TM1638
  module.setupDisplay(1, 3);
  module.setDisplayToString("Start");
  module.setLED(TM1638_COLOR_RED, 0);

  // WiFi
  Serial.begin(9600);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);
  while ((WiFi.waitForConnectResult() != WL_CONNECTED) ) {
    Serial.println("Connection Failed! Restart...");
    WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);
    delay(5000);
    //ESP.restart();
  }
  ArduinoOTA.setHostname("domelora");

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

  // Timer
  timer.setInterval(1000, FuncSec);

  // TODO Eteint l'afficheur si les portes sont fermées

  // Setup locally attached sensors
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

void loop()
{
  String SerMsg;
  String ret;
  // Send locally attached sensors data here
  timer.run();
  ArduinoOTA.handle();
  //Serial.println("C?#");
  //delay(1000);

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
    }
    else if (SerMsg == "DO") {
      send(msgD.set(1));
    }
    else if (SerMsg == "DF") {
      send(msgD.set(0));
    }
    else if (SerMsg == "PO") {
      send(msgP.set(1));
    }
    else if (SerMsg == "PF") {
      send(msgP.set(0));
    }
    else if (SerMsg == "TP") {
      send(msgT.set(1));
    }
    else if (SerMsg == "TN") {
      send(msgT.set(0));
    }
  }
  // Lecture des boutons TM1638
  byte keys = module.getButtons();
  switch (keys) {
    case 1:	//T1 Affiche l'heure
      TypeAff = 1;
      break;
    case 2:	// T2 T° H%
      TypeAff = 2;
      break;
    case 4:	// T3 T° miroir, point de rosée
      TypeAff = 3;
      break;
    case 8:	// T4
      break;
    case 16:	// T5
      break;
    case 32:	// T6 Augmente l'intensité de l'éclairage
      if (NiveauAff < 7) NiveauAff++;
      module.setupDisplay(StateAff, NiveauAff);
      delay(300);
      break;
    case 64:	// T7 Diminue l'intensité de l'éclairage
      if (NiveauAff > 0) NiveauAff--;
      module.setupDisplay(StateAff, NiveauAff);
      delay(300);
      break;
    case 128:	// T8	Allume/Eteint l'affichage
      StateAff = !StateAff;
      module.setupDisplay(StateAff, NiveauAff);
      delay(300);
  }
}

//---------------------------------------FONCTIONS MYSENSORS----------------------------------

void presentation()
{
  // Present locally attached sensors here
  sendSketchInfo("Passerelle dome", "1.1");
  present(2, S_DOOR);
  present(3, S_DOOR);
  present(4, S_BINARY);
}

void before() {
}

void receive(const MyMessage &message) {
  // TODO Lecture des demandes MySensors (D'abord problème RX/Nano à régler)
  /*
    Ouvrir dome
    Fermer dome
    Ouvrir portes
    Fermer portes
    Bouton ARU
  */
  if (message.type == V_STATUS) {
    switch (message.sensor) {
      case 2:
        Serial.println(message.getBool() ? "D-" : "D+");
        break;
      case 3:
        Serial.println(message.getBool() ? "P-" : "P+");
        break;
      case 4:
        GetScopeInfo(":hP#");
        break;
    }
    Serial.println(message.getBool() ? "P-" : "P+");
  }
}

//---------------------------------------FONCTIONS--------------------------------------------

// Fonction executée toutes les secondes
void FuncSec() {
  compte5++;
  compte10++;
  // Toutes les 5s
  if (compte5 == 5) {
    compte5 = 0;
    // TODO Lecture de l'état de chauffe du miroir
  }
  // Toutes les 10s
  if (compte10 == 10) {
    // Fonctions exécutées toutes les 10s
    compte10 = 0;
    switch (TypeAff) {
      case 2:
        break;
      case 3:
        break;
    }
    // TODO Lecture de l'état du télescope: Park, Tracking...
  }
  // Toutes les 1s
  switch (TypeAff) {
    case 0: // Pas d'affichage
      module.setDisplayToString("        ");
      break;
    case 1: // Affichage de l'heure
      if (WiFi.status() != WL_CONNECTED) {

        module.setDisplayToString("--------");
      }
      else {
        while (!timeClient.update()) {
          timeClient.forceUpdate();
        }
        formattedDate = timeClient.getFormattedTime();
        module.setDisplayToString(formattedDate);
      }
      break;
  }
}
// Récupération/envoi des infos du télescope depuis OnStepESPServer
String GetScopeInfo(String msg) {
  String ret;
  WiFiClient client;
  if (!client.connect("192.168.0.15", 9999)) {
    delay(1000);
    return "Error";
  }
  client.print(msg);
  while (client.available()) {
    ret = client.readStringUntil('\r');
    //ret=client.readString();
  }
  client.stop();
  return ret;
}
// Affiche un message sur le TM1638
void AffTM(String ch) {
  ch = ch + "        ";
  ch = ch.substring(0, 8);
  module.setDisplayToString(ch);
}
// Affiche 2 champs formattés sur le TM1638
void AffTM2(String ch1, String ch2) {
  ch1 = ch1 + "    ";
  ch2 = ch2 + "    ";
  ch1 = ch1.substring(0, 4);
  ch2 = ch2.substring(0, 4);
  module.setDisplayToString(ch1 + ch2);
}
