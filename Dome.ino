/* Pilotage automatique de l'abri du telescope
  # Serge CLAUS
  # GPL V3
  # Version 2.6
  # 22/10/2018-19/10/2019
*/

//---------------------------------------PERIPHERIQUES-----------------------------------------------
#include <SoftwareSerial.h> // Port série 2 pour le module IHM (LCD,clavier...)
SoftwareSerial Ser2(13, 2); // RX, TX

// MCP23017
#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;		//MCP externe connecté à la carte 8 relais

//#include <avr/wdt.h>

/*
  // TM1638 LEDs & Keys /!\ Librairie sur Ghitub plop3 (https://github.com/plop3/tm1638-library)
  #include <TM1638.h>
  TM1638 module(4, 17, 25);
*/
/*
  // Clavier matriciel I2c
  #include <i2ckeypad.h>
  #define ROWS 4
  #define COLS 4
  #define PCF8574_ADDR 0x26
  i2ckeypad kpd = i2ckeypad(PCF8574_ADDR, ROWS, COLS);
*/

// LCD I2c
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

// LEDs neopixel
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LEDPIN 4
#define NBLEDS 8
Adafruit_NeoPixel pixels(NBLEDS, LEDPIN, NEO_GRB + NEO_KHZ800);

//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties TODO à vérifier !
#define ALIMPC 	10   // (R3) Alimentation 220V sous le télescope (PC Indi /Raspi) contact NF
#define ALIMTEL 9   // (R4) Alimentation télescope
#define ALIMMOT 11   // (R2) Alimentation 220V moteur abri
#define MOTEUR  12   // (R1) Ouverture/fermeture abri
#define P11     8   // (R5) Relais 1 porte 1
#define P12     7   // (R6) Relais 2 porte 1
#define P21     6   // (R7) Relais 1 porte 2
#define P22     5   // (R8) Relais 2 porte 2

#define BARU    3	// MCP  // Bouton arret d'urgence
#define BMA     A6  // Bouton M/A

// Entrées
#define PARK  1		//MCP	// Etat du telescope 0: non parqué, 1: parqué
/* TODO
   entrée ouverture portes
*/
#define AO 7		// MCP  // Capteur abri ouvert
#define AF 5       	// MCP	// Capteur abri fermé
#define Po1 6       // MCP	// Capteur portes ouvertes
#define Po2 2      	// MCP	// Capteur portes fermées
#define Pf1 4	    // MCP	// 
#define Pf2 0      	// MCP	// 

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIPORTESCAPTEUR  30000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIMOTEUR 40000L  // Durée d'initialisation du moteur
#define DELAIABRI   15000L  // Durée de déplacement de l'abri (25000L)
#define MOTOFF HIGH         // Etat pour l'arret du moteur
#define MOTON !MOTOFF

#define BKLIGHT	3			// Backlight LCD

//---------------------------------------Variables globales------------------------------------

#define AlimStatus  (!digitalRead(ALIMTEL))    // Etat de l'alimentation télescope
#define PortesOuvert (!mcp.digitalRead(Po1) && !mcp.digitalRead(Po2))
#define PortesFerme (!mcp.digitalRead(Pf1) && !mcp.digitalRead(Pf2))
#define AbriFerme (!mcp.digitalRead(AF))
#define AbriOuvert (!mcp.digitalRead(AO))
#define MoteurStatus (!digitalRead(ALIMMOT))
#define StartTel digitalWrite(ALIMTEL, LOW)
#define StopTel digitalWrite(ALIMTEL, HIGH)
#define StartMot digitalWrite(ALIMMOT, MOTON)
#define StopMot digitalWrite(ALIMMOT, MOTOFF)
#define StopPC  digitalWrite(ALIMPC, LOW)
#define StartPC digitalWrite(ALIMPC, HIGH)
#define TelPark mcp.digitalRead(PARK)
#define	BoutonMA (analogRead(BMA)<300)

//#define TelPark 1

bool Manuel = false;
bool LastPark = false;
//---------------------------------------SETUP-----------------------------------------------

void setup() {

  Serial.begin(57600);
  Ser2.begin(9600);

  mcp.begin();

  // Initialisation des relais
  digitalWrite(ALIMPC, HIGH); pinMode(ALIMPC, OUTPUT);
  digitalWrite(ALIMTEL, HIGH); pinMode(ALIMTEL, OUTPUT);
  digitalWrite(ALIMMOT, HIGH); pinMode(ALIMMOT, OUTPUT);
  digitalWrite(MOTEUR, HIGH); pinMode(MOTEUR, OUTPUT);
  digitalWrite(P11, HIGH); pinMode(P11, OUTPUT);
  digitalWrite(P12, HIGH); pinMode(P12, OUTPUT);
  digitalWrite(P21, HIGH); pinMode(P21, OUTPUT);
  digitalWrite(P22, HIGH); pinMode(P22, OUTPUT);
  digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Activation des entrées (capteurs...)

  mcp.pinMode(AO, INPUT); mcp.pullUp(AO, HIGH);
  mcp.pinMode(AF, INPUT); mcp.pullUp(AF, HIGH);


  mcp.pinMode(Po1, INPUT); mcp.pullUp(Po1, HIGH);
  mcp.pinMode(Pf1, INPUT); mcp.pullUp(Pf1, HIGH);
  mcp.pinMode(Po2, INPUT); mcp.pullUp(Po2, HIGH);
  mcp.pinMode(Pf2, INPUT); mcp.pullUp(Pf2, HIGH);
  mcp.pinMode(BARU, INPUT); mcp.pullUp(BARU, HIGH);
  mcp.pinMode(PARK, INPUT); //mcp.pullUp(PARK, LOW);
  pinMode(BMA, INPUT);
  pinMode(BKLIGHT, OUTPUT);
  analogWrite(BKLIGHT, 50);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Demarrage...");

  // LEDs
  pixels.begin();
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 20, 0));
  pixels.show();

  // Vérification de la position du dome au démarrage
  if (!AbriOuvert && !AbriFerme) {
    // Position incorrecte on passe en mode manuel
    Manuel = true;
  }
  // Etat du dome initialisation des interrupteurs
  if ( AbriOuvert) {
    StartTel; // Alimentation télescope
    StartMot; // Alimentation du moteur de l'abri
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ok");
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg = "";		// Message reçu sur le port série

void loop() {
  SerMsg = LireCmd();
  if (SerMsg != "") {
    if (!Manuel) {
      // Commande en mode Auto seulement
      if (SerMsg == "P+") {
        changePortes(true);
        Serial.println((PortesOuvert) ? "1" : "0");
      }
      else if (SerMsg == "P-") {
        changePortes(false);
        Serial.println((!PortesOuvert) ? "1" : "0");
      }
      else if (SerMsg == "D+") {
        deplaceAbri(true);
        Serial.println((AbriOuvert) ? "1" : "0");
      }
      else if (SerMsg == "D-") {
        deplaceAbri(false);
        Serial.println((AbriFerme) ? "1" : "0");
      }
      else if (SerMsg == "AU") {
        Serial.println("0");
        ARU();
      }
      else if (SerMsg == "MA") {
        Manuel = true;
      }
    }
    else {
      // Commandes manuelles seulement
      if (SerMsg == "OK") {
        // Retour en mode automatique
        Manuel = false;
      }
      else if (SerMsg == "m+") {
        StartMot;
      }
      else if (SerMsg == "m-") {
        StopMot;
      }
      else if (SerMsg == "dd") {
        DeplaceDomeARU();
      }
      else if (SerMsg == "2+") {
        ouvrePorte2();
      }
      else if (SerMsg == "2-") {
        fermePorte2();
      }
    }
    // Commandes Auto/Manuel
    if (SerMsg == "A+") {
      StartTel;
      Serial.println("1");
    }
    else if ( SerMsg == "A-") {
      StopTel;
      Serial.println("1");
    }
    else if (SerMsg == "P?") {
      Serial.println((PortesOuvert) ? "1" : "0");
    }
    else if (SerMsg == "D?") {
      Serial.println((AbriFerme) ? "0" : "1");
    }
    else if (SerMsg == "A?") {
      Serial.println(AlimStatus ? "1" : "0");
    }

    else if (SerMsg == "p-") {
      fermePorte1();
      Serial.println("0");
    }
    else if (SerMsg == "p+") {
      ouvrePorte1();
      Serial.println("0");
    }
    else if (SerMsg == "PC") {
      // Démarrage du PC
      StopPC;
      delay(1000);
      StartPC;
    }
    else if (SerMsg == "C?") {
      Serial.print(AbriFerme);
      Serial.print(AbriOuvert);
      Serial.print(PortesFerme);
      Serial.print(PortesOuvert);
      Serial.print(AlimStatus);
      Serial.println(TelPark ? "p" : "n");
      Serial.print(mcp.digitalRead(Pf1));
      Serial.print(mcp.digitalRead(Pf2));
      Serial.print(mcp.digitalRead(Po1));
      Serial.println(mcp.digitalRead(Po2));
    }
  }

  // LED état park
/*
  if (LastPark != TelPark) {
    pixels.setPixelColor(1, pixels.Color(0, 10*TelPark, 0));
    pixels.show();
    LastPark = TelPark;
  }
 */
  // TEST DEPLACEMENT INOPINE DU DOME
  // TODO à décommenter quand installé
  /*
    if (!Manuel && !AbriFerme && !AbriOuvert) {
    ARU();
    }
  */

  // Bouton Arret d'urgence
  if (!mcp.digitalRead(BARU)) {
    ARU();
  }
  // Bouton Marche/Arret ? on ouvre la petite porte
  if (BoutonMA) {
    ouvrePorte1();
  }

}

//---------------------------------------FONCTIONS--------------------------------------------
String LireCmd(void) {
  if (Serial.available()) {
    SerMsg = Serial.readStringUntil(35);
    return SerMsg;
  }
  if (Ser2.available()) {
    SerMsg = Ser2.readStringUntil(35);
    return SerMsg;
  }
  return "";
}

// Ferme la petite porte
void fermePorte1(void) {
  digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  digitalWrite(P11, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  digitalWrite(P12, HIGH);
}

// Ferme la porte2
void fermePorte2(void) {
  digitalWrite(P21, LOW);
  delay(DELAIPORTES);
  digitalWrite(P21, HIGH);
}

// Ouvre la porte2
void ouvrePorte2(void) {
  digitalWrite(P22, LOW);
  delay(DELAIPORTES);
  digitalWrite(P22, HIGH);
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
    digitalWrite(P12, LOW);
    attendPorte(5000);
    digitalWrite(P22, LOW);
    attendPorte(DELAIPORTESCAPTEUR); // Délai minimum
    // On attend que les portes sont ouvertes
    while (!PortesOuvert) {
      attendPorte(100);
    }
    // Délai pour finir le mouvement
    attendPorte(5000);
    digitalWrite(P12, HIGH);
    digitalWrite(P22, HIGH);
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      return;
    }
    StopMot;
    digitalWrite(P21, LOW);
    attendPorte(5000);
    digitalWrite(P11, LOW);
    attendPorte(DELAIPORTES);
    digitalWrite(P11, HIGH);
    digitalWrite(P21, HIGH);
  }
}

void DeplaceDomeARU(void) {
  /* Déplace le dome sans vérification /!\ */
  digitalWrite(MOTEUR, LOW);
  delay(600);
  digitalWrite(MOTEUR, HIGH);
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
  digitalWrite(MOTEUR, LOW);
  delay(600);
  digitalWrite(MOTEUR, HIGH);
  attendDep(DELAIABRI);
  while (!AbriFerme && !AbriOuvert) {
    attendDep(1000);
  }
  attendDep(2000);		   // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat = AbriOuvert;
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
    SerMsg = LireCmd();
    if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
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
    SerMsg = LireCmd();
    if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) ARU();
    // Si le dome se déplace pendant le mouvement des portes: ARU
    if (!AbriFerme && !AbriOuvert) {
      ARU();
    }
    // Bouton Arret d'urgence
    //if digitalRead(BARU) {ARU();}
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

// Commande d'arret d'urgence
void ARU() {				// Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  Serial.println("ARU");
  digitalWrite(ALIMTEL, HIGH);
  digitalWrite(ALIMMOT, HIGH);
  digitalWrite(MOTEUR, HIGH);
  digitalWrite(P11, HIGH);
  digitalWrite(P12, HIGH);
  digitalWrite(P21, HIGH);
  digitalWrite(P22, HIGH);
  digitalWrite(ALIMMOT, MOTOFF);
  // Ouverture des portes
  changePortes(true);
}
