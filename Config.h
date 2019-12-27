//---------------------------------------PERIPHERIQUES--------------------------
#include <SoftwareSerial.h> // Port série 2 pour le module LoRa (TM1638...)
SoftwareSerial Ser2(16, 2); // RX, TX (13, 2)

// MCP23017
#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;    //MCP Entrées capteurs

// Clavier matriciel I2c
#include <i2ckeypad.h>
#define ROWS 4
#define COLS 4
#define PCF8574_ADDR 0x26
i2ckeypad kpd = i2ckeypad(PCF8574_ADDR, ROWS, COLS);

// LCD I2c
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 20 chars and 4 line display

// LEDs neopixel
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LEDPIN 4
#define NBLEDS 28	// Nombre total de LEDs
Adafruit_NeoPixel pixels(NBLEDS, LEDPIN, NEO_GRB + NEO_KHZ400);
/* 0:		Eclairage clavier
   1:		Status dome
   2:		Status park
   3:		Option (N/A)
   4-11:	Eclairage table
   12-19:	Eclairage intérieur
   20-28:	Eclairage extérieur
*/

/*
// Client REST: Lecture de l'heure/date, Demande de park vers OnStepESP
#include <ELClient.h>
#include <ELClientSocket.h>
ELClient esp(&Serial, &Serial);
ELClientSocket tcp(&esp);
char * const tcpServer PROGMEM = "192.168.0.103";
uint16_t const tcpPort PROGMEM = 9999;
*/

// Timer
#include <SimpleTimer.h>
SimpleTimer timer;
//---------------------------------------CONSTANTES-----------------------------

// Sorties
#define ALIM12V   10   // (R3) Mise en marche de l'alimentation ATX
#define ALIMTEL 9   // (R4) Alimentation télescope
#define ALIMMOT 11   // (R2) Alimentation 220V moteur abri
#define MOTEUR  12   // (R1) Ouverture/fermeture abri
#define P11     8   // (R5) Relais 1 porte 1
#define P12     7   // (R6) Relais 2 porte 1
#define P21     6   // (R7) Relais 1 porte 2
#define P22     5   // (R8) Relais 2 porte 2

#define BARU    3 // MCP  // Bouton arret d'urgence
#define BMA     A6  // Bouton M/A

// Entrées
#define PARK  1   	//MCP // Etat du telescope 0: non parqué, 1: parqué

#define AO 7        // MCP  // Capteur abri ouvert
#define AF 5        // MCP  // Capteur abri fermé
#define Po1 6       // MCP  // Capteur portes ouvertes
#define Po2 2       // MCP  // 
#define Pf1 4       // MCP  // Capteur portes fermées
#define Pf2 0       // MCP  // 

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIPORTESCAPTEUR  30000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIMOTEUR 10000L  // Durée d'initialisation du moteur (40000L)
#define DELAIABRI   11000L  // Durée de déplacement de l'abri (15000L)
#define INTERVALLEPORTES 8000
#define MOTOFF HIGH         // Etat pour l'arret du moteur
#define MOTON !MOTOFF

#define BKLIGHT 3     	// PIN Backlight LCD
#define BUZZER 17 		//A3

// Boutons poussoirs
#define BEXT 11  //MCP	Eclairage extérieur
#define BINT 13  //MCP	Eclairage intérieur
#define BTAB 14  //MCP	Eclairage table
#define BSEL 10  //MCP	Bouton de sélection -->
#define BCHOIX	9	//MCP	Bouton de choix
#define BVALID	8	//MCP Bouton de validation

// PCF8574
#define TPSVEILLE 300000L  //Delai avant la mise en veille du clavier (ms)

// LEDs
#define LEDLVLCLAV 15	// Intensité de la led du clavier

const byte LEDLEV[] = {2,5,10,20,40}; 		// Intensités possibles des LEDs
const byte APALEV[] = {10,25,50,100,200};		// Intensités des barreaux de LEDs
const byte LCDLEV[] = {5,10,15,20,40};		// Intensité du rétro-éclairage LCD

//---------------------------------------Macros---------------------------------
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
#define BoutonMA (analogRead(BMA)<300)

#define LedClavier  0
#define LedStatus   1
#define LedPark     2
#define LedOpt      3
//#define TelPark 1

//---------------------------------------Variables globales---------------------
bool Manuel = false;  // Mode manuel
bool LastPark = false;  // Dernier état de Park
byte LEVEL[] = {25, 50, 200, 20, 2}; // Intensités Table, Intérieur, Extérieur, LCD, LEDs
bool REDLED[] = {false, true, true}; // Eclairage rouge (false), blanc (true) Table, Intérieur, Extérieur
bool ECLSTAT[] = { false, false, false}; //Etat des éclairages 

bool ECLINT = false;
bool ECLEXT = false;
bool ECLTAB = false;

bool Lock = true; // Dome locké
bool Veille = false; // Clavier matriciel en veille
String SECRET = "1234"; // Code de déverrouillage

// IHM
byte POS = 5;
byte niveau[] = {1, 7, 9, 2, 2, 0};	// Choix de niveaux d'éclairage au démarrage
