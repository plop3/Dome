/* Pilotage automatique de l'abri du telescope
  # Serge CLAUS
  # GPL V3
  # Version 2.0
  # 22/10/2018-28/12/2018
*/

//-------------------------------------FICHIERS EXTERNES---------------------------------------------
#include "Dome.h"
//---------------------------------------PERIPHERIQUES-----------------------------------------------

// MCP23017
#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;

// WiFi
#include <ESP8266WiFi.h>*
#include <ESP8266mDNS.h>

//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties
#define LEDPARK D7	// LED d'indication du park // TODO Remplacer par 1 led APA106 + éclairages intérieurs
// Sorties MCP23017
#define ALIM12V 2   // (R3) Alimentation 12V
#define ALIMTEL 3   // (R4) Alimentation télescope
#define ALIMMOT 1   // (R2) Alimentation 220V moteur abri
#define MOTEUR  0   // (R1) Ouverture/fermeture abri
#define P11     4   // (R5) Relais 1 porte 1
#define P12     5   // (R6) Relais 2 porte 1
#define P21     6   // (R7) Relais 1 porte 2
#define P22     7   // (R8) Relais 2 porte 2

// Entrées
#define PARK  D6	// Etat du telescope 0: non parqué, 1: parqué
/* TODO 
 * entrée ouverture portes
 */
// Entrées MCP23017
#define AO 9        // Capteur abri ouvert
#define AF 8        // Capteur abri fermé
#define PO 11       // Capteur portes ouvertes
#define PF 10       // Capteur portes fermées
/* TODO
 *  Ecran LCD I2c
 */

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes
#define DELAIABRI   25000L  // Durée de déplacement de l'abri
#define MOTOFF HIGH          // Etat pour l'arret du moteur

//---------------------------------------Variables globales------------------------------------

#define PortesOuvert !mcp.digitalRead(PO)
bool PortesFerme = !PortesOuvert;    // A remplacer par les capteurs fin de course
#define AbriOuvert mcp.digitalRead(AO)
#define AbriFerme mcp.digitalRead(AF)
#define TelPark digitalRead(PARK)
//#define TelPark 1
#define AlimStatus  !mcp.digitalRead(ALIM12V)    // Etat de l'alimentation 12V

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial.begin(9600);
  mcp.begin();                      // Utilise l'adresse par défaut qui est 0
  // Initialisation des relais
  for (int i = 0; i < 8; i++) {
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, HIGH);
  }
  mcp.digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Activation des entrées (capteurs...)
  mcp.pinMode(AO, INPUT);
  mcp.pinMode(AF, INPUT);
  mcp.pinMode(PO, INPUT);
  mcp.pinMode(PF, INPUT);
  mcp.pullUp(AO, HIGH);
  mcp.pullUp(AF, HIGH);
  mcp.pullUp(PO, HIGH);
  mcp.pullUp(PF, HIGH);

  //pinMode(PARK, INPUT_PULLUP);
  pinMode(PARK, INPUT);
  pinMode(LEDPARK, OUTPUT);
  digitalWrite(LEDPARK, TelPark);

  // Etat du dome initialisation des interrupteurs
  if (PortesOuvert || !AbriFerme) {	// TODO remplacer par AbriOuvert quand le capteur sera changé
    mcp.digitalWrite(ALIM12V, LOW);
  }
  if ( !AbriFerme) {	// TODO remplacer par AbriOuvert
    mcp.digitalWrite(ALIMTEL, LOW); // Alimentation télescope
  }
  // TODO Tant qu'on n'a pas les contacts portes fermées et abri fermé
  PortesFerme = !PortesOuvert;

  // Connexion WiFi
  int attempts = 0;
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  //WiFi.hostname("dome");
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    attempts++;
  }
  if (attempts == 10) {
    // WiFi non accessible, on passe en mode AP
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.softAP(ssid2,pwd2);
  }
  MDNS.begin("dome");
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg="";		// Message reçu sur le port série

void loop() {
  // Lecture des ordres reçus du port série
  if (Serial.available()) {
    SerMsg=Serial.readStringUntil(35);
	  if (SerMsg == "P+") {
      changePortes(true);
      Serial.println(PortesOuvert ? "1" : "0");
	  }
    else if (SerMsg == "P-") {
      changePortes(false);
      Serial.println(PortesFerme ? "1" : "0");
    }
    else if (SerMsg == "D+") {
      deplaceAbri(true);
	          Serial.println(AbriOuvert ? "1" : "0");
    }
    else if (SerMsg == "D-") {
      deplaceAbri(false);
	          Serial.println(AbriFerme ? "1" : "0");
    }
    else if (SerMsg == "A+") {
	    mcp.digitalWrite(ALIM12V,LOW);
	    Serial.println("1");
    }
    else if ( SerMsg == "A-") {
	    mcp.digitalWrite(ALIM12V,HIGH);
	    Serial.println("1");
    }
    else if (SerMsg == "P?") {
      Serial.println(PortesOuvert ? "1" : "0");
    }
    else if (SerMsg == "D?") {
            Serial.println(AbriFerme ? "0" : "1");
    }
    else if (SerMsg == "A?") {
            Serial.println(AlimStatus ? "1" : "0");
    }
    else if (SerMsg == "AU") {
	    Serial.println("0");
            ARU();
    }
    else if (SerMsg == "p+") {
	ouvrePorte1();
	Serial.println("0");
    }	  
    else if (SerMsg == "C?") {
	Serial.print(AbriFerme);
	Serial.print(AbriOuvert);
	Serial.print(PortesFerme);
	Serial.print(PortesOuvert);
	Serial.println(AlimStatus);
    }
  }
  digitalWrite(LEDPARK, TelPark);
}

//---------------------------------------FONCTIONS--------------------------------------------

// Ouvre la petite porte
void ouvrePorte1(void) {
  if (!AlimStatus) {
    // Mise en marche de l'alimentation 12V
    mcp.digitalWrite(ALIM12V, LOW);
    delay(3000);
  }	
  mcp.digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  mcp.digitalWrite(P12, HIGH);
}

// Change la position des portes 0: ouverture 1 fermeture
void changePortes(bool etat) {
  // Commande identique à l'état actuel, on sort
  PortesFerme=!PortesOuvert;	// TODO En attendant le capteur portes fermées
  if ((etat && PortesOuvert) || (!etat && PortesFerme)) {
    return;
  }
  if (!AlimStatus) {
    // Mise en marche de l'alimentation 12V
    mcp.digitalWrite(ALIM12V, LOW);
    delay(3000);
  }
  if (etat) {   // Ouverture des portes
    mcp.digitalWrite(P12, LOW);
    delay(5000);
    PortesFerme = false;	// TODO En attendant d'avoir un capteur portes fermées
    mcp.digitalWrite(P22, LOW);
    delay(DELAIPORTES);
    while (!PortesOuvert) {
      delay(10);
    }
    delay(5000);
    mcp.digitalWrite(P12, HIGH);
    mcp.digitalWrite(P22, HIGH);
    PortesFerme = !PortesOuvert; // TODO En attendant d'avoir un capteur portes fermées
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      return;
    }
    mcp.digitalWrite(P21, LOW);
    delay(5000);
    mcp.digitalWrite(P11, LOW);
    attendDep(DELAIPORTES);
    mcp.digitalWrite(P11, HIGH);
    mcp.digitalWrite(P21, HIGH);
    if (AbriFerme) {
      // Coupure de l'alimentation 12V
      mcp.digitalWrite(ALIM12V, HIGH);
    }
    PortesFerme = !PortesOuvert;	// TODO En attendant le capteur
  }
}

// Déplacement de l'abri 1: ouverture 0: fermeture
void deplaceAbri(bool etat) {
  // Commande identique à l'état actuel, on sort
  //if ((etat && AbriOuvert) || (!etat && AbriFerme)) {
  if ((etat && !AbriFerme) || (!etat && AbriFerme)) { // TODO Patch contacteur abri ouvert problématique
    return;
  }
  // Abri dans une position indeterminée, on sort avec message d'erreur
  /* if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    return;
  } */
  mcp.digitalWrite(ALIMMOT, !MOTOFF); // Alimentation du moteur
  // Pas d'alimentation 12V ?
  if (!AlimStatus) {
    mcp.digitalWrite(ALIM12V, false);
    // Boucle d'attente télescope parqué
    for (int i = 0; i < 15; i++) {
      if (TelPark) break;
      attendDep(2000);
    }
  }
  // Test telescope parqué
  if (!TelPark) {
    return;
  }
  if (!PortesOuvert) {
    changePortes(true);    //Ouverture des portes
  }
  else {
    delay(10000); 	// A ajuster
    // Mini impulsion pour activer le moteur ???
  }
  mcp.digitalWrite(ALIMTEL, HIGH); // Coupure alimentation télescope
  // Deplacement de l'abri
  mcp.digitalWrite(MOTEUR, LOW);
  delay(600);
  mcp.digitalWrite(MOTEUR, HIGH);
  attendDep(5000);		// On verifie 	au bout de 5s si l'abri a bougé
  if (etat) {			// Abri en cours d'ouverture
    if (AbriFerme) {
      mcp.digitalWrite(MOTEUR, LOW);
      delay(600);
      mcp.digitalWrite(MOTEUR, HIGH);
    }
    attendDep(DELAIABRI);
    while (!AbriOuvert && !AbriFerme) {
      attendDep(1000);
    }
  }
  else {
    if (AbriOuvert) {
      mcp.digitalWrite(MOTEUR, LOW);
      delay(600);
      mcp.digitalWrite(MOTEUR, HIGH);
    }
    attendDep(DELAIABRI);
    while(!AbriFerme && !AbriOuvert) {	
      attendDep(1000);
    }
  }
  attendDep(5000);		   // Finir le déplacement
  mcp.digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat=!AbriFerme;
  if (etat) {
    // Abri ouvert
    mcp.digitalWrite(ALIMTEL, LOW); // Alimentation télescope
  }
  else {
    // Abri fermé
    mcp.digitalWrite(ALIMTEL, HIGH); // Coupure alimentation télescope
    delay(500);
    changePortes(false);             // Fermeture des portes
    mcp.digitalWrite(ALIM12V, HIGH); // Coupure alimentation 12V
  }
}

// Boucle d'attente lors du déplacement
void attendDep(unsigned long delai) {	// Boucle d'attente pendant le déplacement de l'abri
  int ERRMAX = 2;
  int nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    if (Serial.available()) {
    	SerMsg=Serial.readStringUntil(35);
    	if (SerMsg == "AU") {
	    Serial.println("0");
            ARU();
    	}
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) ARU();
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

// Commande d'arret d'urgence
void ARU() {				// Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  for (int i = 0; i < 8; i++) {
    mcp.digitalWrite(i, HIGH);
  }
  mcp.digitalWrite(ALIMMOT, MOTOFF);
  // Ouverture des portes
  changePortes(true);
  ESP.restart(); // ESP.reset();
}
