/* Pilotage automatique de l'abri du telescope
  # Serge CLAUS
  # GPL V3
  # Version 3.0
  # 22/10/2018-24/06/2019
  # Version pour TTGO ESP32 LoRa + MCP23017
*/

//-------------------------------------FICHIERS EXTERNES---------------------------------------------
#include "Dome.h"

//---------------------------------------PERIPHERIQUES-----------------------------------------------
// MCP23017
#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;

// WiFi + OTA
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Serveur TCP
WiFiServer Server(23);

//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties
#define LEDPARK 2	// LED d'indication du park // TODO Remplacer par 1 led APA106 + éclairages intérieurs
#define LUMIERE 4  // Eclairage de l'abri TODO à modifier

// Sorties MCP23017
#define ALIM12V 2   // (R3) Alimentation 12V
#define ALIM24V 3   // (R4) Alimentation télescope
#define ALIMMOT 1   // (R2) Alimentation 220V moteur abri
#define MOTEUR  0   // (R1) Ouverture/fermeture abri
#define P11     4   // (R5) Relais 1 porte 1
#define P12     5   // (R6) Relais 2 porte 1
#define P21     6   // (R7) Relais 1 porte 2
#define P22     7   // (R8) Relais 2 porte 2

// Entrées
#define PARK  13	// Etat du telescope 0: non parqué, 1: parqué
/* TODO 
 * entrée ouverture portes
 */
#define AO 9        // Capteur abri ouvert
#define AF 8        // Capteur abri fermé
#define Po1 11       // Capteur porte 1 ouverte
#define Po2 13       // Capteur porte 2 ouverte
#define Pf1 10	    //  Capteur porte 1 fermée (BARU)
#define Pf2 12       // Capteur porte 2 fermée (BMA)

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIPORTESCAPTEUR  30000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIMOTEUR 40000L  // Durée d'initialisation du moteur
#define DELAIABRI   15000L  // Durée de déplacement de l'abri (25000L)
#define MOTOFF HIGH         // Etat pour l'arret du moteur
#define MOTON !MOTOFF

//---------------------------------------Variables globales------------------------------------

#define AlimStatus  (!mcp.digitalRead(ALIM24V))    // Etat de l'alimentation télescope
#define PortesOuvert (!mcp.digitalRead(Po1) && !mcp.digitalRead(Po2))
#define PortesFerme (!mcp.digitalRead(Pf1) && !mcp.digitalRead(Pf2))
#define AbriFerme (!mcp.digitalRead(AF)) 
#define AbriOuvert (!mcp.digitalRead(AO))
#define MoteurStatus (!mcp.digitalRead(ALIMMOT))
#define StartTel mcp.digitalWrite(ALIM24V, LOW)
#define StopTel mcp.digitalWrite(ALIM24V, HIGH)
#define StartMot mcp.digitalWrite(ALIMMOT, MOTON)
#define StopMot mcp.digitalWrite(ALIMMOT, MOTOFF)
//#define TelPark digitalRead(PARK)
#define TelPark 1

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial.begin(9600);
  mcp.begin();	// Utilise l'adresse par défaut qui est 0
  // Initialisation des relais
  pinMode(LEDPARK, OUTPUT);
  pinMode(LUMIERE, OUTPUT);
  mcp.digitalWrite(ALIM12V,HIGH);
  mcp.pinMode(ALIM12V, OUTPUT);
  mcp.digitalWrite(ALIM24V,HIGH);mcp.pinMode(ALIM24V, OUTPUT);
  mcp.digitalWrite(ALIMMOT,HIGH);mcp.pinMode(ALIMMOT, OUTPUT);
  mcp.digitalWrite(MOTEUR,HIGH);mcp.pinMode(MOTEUR, OUTPUT);
  mcp.digitalWrite(P11,HIGH);mcp.pinMode(P11, OUTPUT);
  mcp.digitalWrite(P12,HIGH);mcp.pinMode(P12, OUTPUT);
  mcp.digitalWrite(P21,HIGH);mcp.pinMode(P21, OUTPUT);
  mcp.digitalWrite(P22,HIGH);mcp.pinMode(P22, OUTPUT);
  
  mcp.digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Activation des entrées (capteurs...)
  mcp.pinMode(AO, INPUT_PULLUP);
  mcp.pinMode(AF, INPUT_PULLUP);
  mcp.pinMode(Po1, INPUT_PULLUP);
  mcp.pinMode(Pf1, INPUT_PULLUP);
  mcp.pinMode(Po2, INPUT_PULLUP);
  mcp.pinMode(Pf2, INPUT_PULLUP);
  //pinMode(BARU, INPUT_PULLUP);
  //pinMode(BMA, INPUT_PULLUP);

  //pinMode(PARK, INPUT_PULLUP);
  pinMode(PARK, INPUT);
  pinMode(LEDPARK, OUTPUT);
  digitalWrite(LEDPARK, TelPark);

  // Etat du dome initialisation des interrupteurs
  if ( AbriOuvert) {
    StartTel; // Alimentation télescope
  }
  
  
  // Connexion WiFi
  WiFi.mode(WIFI_AP_STA);
  //WiFi.hostname("dome");
  //access point part
  Serial.println("Creating Accesspoint");
  WiFi.softAP(assid,asecret,7,0,5);
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());
  //station part
  IPAddress local_IP(192,168,0,17);
  IPAddress gateway(192,168,0,1);
  IPAddress subnet(255,255,255,0);
  IPAddress primaryDNS(212,27,40,240);
  IPAddress secondaryDNS(212,27,40,241);
  WiFi.config(local_IP,gateway,subnet,primaryDNS,secondaryDNS);
  Serial.print("connecting to...");
  Serial.println(ssid);
  WiFi.begin(ssid,password);
  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED && attempts < 10){
    delay(500);
	attempts++;
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());   
  //MDNS.begin("dome");


// OTA
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("dome");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();
  // Serveur TCP
  Server.begin();
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg="";		// Message reçu sur le port série

void loop() {
  ArduinoOTA.handle();	
  // Lecture des ordres reçus du port série2 (ESP8266)
  WiFiClient client = Server.available();
  if (client) {
    SerMsg=client.readStringUntil(35);
	  if (SerMsg == "P+") {
      changePortes(true);
      client.println((PortesOuvert) ? "1" : "0");
	  }
    else if (SerMsg == "P-") {
      changePortes(false);
      client.println((!PortesOuvert) ? "1" : "0");
    }
    else if (SerMsg == "D+") {
      deplaceAbri(true);
	          client.println((AbriOuvert) ? "1" : "0");
    }
    else if (SerMsg == "D-") {
      deplaceAbri(false);
	          client.println((AbriFerme)? "1" : "0");
    }
    else if (SerMsg == "A+") {
	    StartTel;
	    client.println("1");
    }
    else if ( SerMsg == "A-") {
	    StopTel;
	    client.println("1");
    }
	  else if (SerMsg == "P?") {
      client.println((PortesOuvert) ? "1" : "0");
    }
    else if (SerMsg == "D?") {
            client.println((AbriFerme) ? "0" : "1");
    }
    else if (SerMsg == "A?") {
            client.println(AlimStatus ? "1" : "0");
    }
    else if (SerMsg == "AU") {
	    client.println("0");
            ARU();
    }
    else if (SerMsg == "p-") {
	  fermePorte1();
	  client.println("0");
    }	  
    else if (SerMsg == "p+") {
	  ouvrePorte1();
	  client.println("0");
    }	  
    else if (SerMsg == "C?") {
      client.print(AbriFerme);
      client.print(!AbriFerme);
      client.print(PortesFerme);
      client.print(PortesOuvert);
      client.print(AlimStatus);
      client.println(TelPark ? "p" : "n");
	  client.print(digitalRead(Pf1));
	  client.print(digitalRead(Pf2));
	  client.print(digitalRead(Po1));
	  client.println(digitalRead(Po2));
    }
  client.stop();
  }
  digitalWrite(LEDPARK, TelPark);
    
  // TEST DEPLACEMENT INOPINE DU DOME
  if (!AbriFerme && !AbriOuvert) {ARU();};
  
  // Bouton Arret d'urgence
  //if digitalRead(BARU) {ARU();}
  
}

//---------------------------------------FONCTIONS--------------------------------------------

// Ferme la petite porte
void fermePorte1(void) {
  mcp.digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  mcp.digitalWrite(P11, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  mcp.digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  mcp.digitalWrite(P12, HIGH);
}

// Change la position des portes 0: ouverture 1 fermeture
void changePortes(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && PortesOuvert) || (!etat && PortesFerme)) {
	  return;
  }
  if (etat) {   // Ouverture des portes
	// Alimentation du moteur
	StartMot; // On allume assez tôt pour laisser le temps de s'initialiser
	// Ouverture des portes
    mcp.digitalWrite(P12, LOW);
    attendPorte(5000);
    mcp.digitalWrite(P22, LOW);
    attendPorte(DELAIPORTESCAPTEUR); // Délai minimum
    // On attend que les portes sont ouvertes
    while (!PortesOuvert) {
      attendPorte(100);
    }
    // Délai pour finir le mouvement
    attendPorte(5000);
    mcp.digitalWrite(P12, HIGH);
    mcp.digitalWrite(P22, HIGH);
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      return;
    }
    StopMot;
    mcp.digitalWrite(P21, LOW);
    attendPorte(5000);
    mcp.digitalWrite(P11, LOW);
    attendPorte(DELAIPORTES);
    mcp.digitalWrite(P11, HIGH);
    mcp.digitalWrite(P21, HIGH);
  }
}

// Déplacement de l'abri 1: ouverture 0: fermeture
void deplaceAbri(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && AbriOuvert) || (!etat && AbriFerme)) { 
    return;
  }
  // Test telescope parqué
   if (!TelPark) {
    return;
  }
  StopTel; // Coupure alimentation télescope
  if (!PortesOuvert) {
    if (!MoteurStatus) StartMot; // Alimentation du moteur
    changePortes(true);    //Ouverture des portes
  }
  else if (!MoteurStatus) {
    // Attente d'initialisation du moteur de l'abri
    StartMot;
    //Attente pour l'initialisation du moteur
    attendPorte(DELAIMOTEUR); // Protection contre les déplacements intempestifs
  }
  // Deplacement de l'abri
  mcp.digitalWrite(MOTEUR, LOW);
  delay(600);
  mcp.digitalWrite(MOTEUR, HIGH);
  attendDep(DELAIABRI);
  while(!AbriFerme && !AbriOuvert) {	
    attendDep(1000);
  }
  attendDep(2000);		   // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat=AbriOuvert;
  if (etat) {
    // Abri ouvert
    StartTel; // Alimentation télescope
  }
  else {
    // Abri fermé
    StopTel; // Coupure alimentation télescope
    delay(500);
    changePortes(false);             // Fermeture des portes
    // Pas nécessaire (déjà fait à la fermeture des portes)
	  StopMot; // Coupure alimentation moteur abri
    StopTel; // Coupure alimentation dome
  }
}

// Boucle d'attente lors du déplacement
void attendDep(unsigned long delai) {	// Boucle d'attente pendant le déplacement de l'abri
  int ERRMAX = 2;
  int nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
	WiFiClient client = Server.available();
	if (client) {
    	SerMsg=client.readStringUntil(35);
    	if (SerMsg == "AU") {
	    client.println("0");
            ARU();
    	}
	client.stop();
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) ARU();
    // Bouton Arret d'urgence
    //if digitalRead(BARU) {ARU();}
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

void attendPorte(unsigned long delai) {	// Boucle d'attente pendant l'ouverture/fermeture des portes
  int ERRMAX = 2;
  int nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
	WiFiClient client = Server.available();
	if (client) {
    	SerMsg=client.readStringUntil(35);
    	if (SerMsg == "AU") {
	    client.println("0");
            ARU();
    	}
	client.stop();
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) ARU();
	// Si le dome se déplace pendant le mouvement des portes: ARU
	if (!AbriFerme && !AbriOuvert) {ARU();}
	// Bouton Arret d'urgence
    //if digitalRead(BARU) {ARU();}
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

// Commande d'arret d'urgence
void ARU() {				// Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  mcp.digitalWrite(ALIM12V,HIGH);
  mcp.digitalWrite(ALIM24V,HIGH);
  mcp.digitalWrite(ALIMMOT,HIGH);
  mcp.digitalWrite(MOTEUR,HIGH);
  mcp.digitalWrite(P11,HIGH);
  mcp.digitalWrite(P12,HIGH);
  mcp.digitalWrite(P21,HIGH);
  mcp.digitalWrite(P22,HIGH);
  mcp.digitalWrite(ALIMMOT, MOTOFF);
  // Ouverture des portes
  changePortes(true);
  ESP.restart();
}
