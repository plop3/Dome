//---------------------------------------PERIPHERIQUES-----------------------------------------------
#include <SoftwareSerial.h> // Port série 2 pour le module IHM (LCD,clavier...)
SoftwareSerial Ser2(13, 2); // RX, TX

// MCP23017
#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;    //MCP externe connecté à la carte 8 relais

// Clavier matriciel I2c
#include <i2ckeypad.h>
#define ROWS 4
#define COLS 4
#define PCF8574_ADDR 0x26
i2ckeypad kpd = i2ckeypad(PCF8574_ADDR, ROWS, COLS);

// LCD I2c
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

// LEDs neopixel
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LEDPIN 4
#define NBLEDS 28
Adafruit_NeoPixel pixels(NBLEDS, LEDPIN, NEO_GRB + NEO_KHZ400);

// Client REST
#include <ELClient.h>
#include <ELClientRest.h>
ELClient esp(&Serial, &Serial);
ELClientRest rest(&esp);

//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties
#define ALIMPC   10   // (R3) Alimentation 220V sous le télescope (PC Indi /Raspi) contact NF
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
#define PARK  1   //MCP // Etat du telescope 0: non parqué, 1: parqué

#define AO 7        // MCP  // Capteur abri ouvert
#define AF 5        // MCP  // Capteur abri fermé
#define Po1 6       // MCP  // Capteur portes ouvertes
#define Po2 2       // MCP  // 
#define Pf1 4       // MCP  // Capteur portes fermées
#define Pf2 0       // MCP  // 

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIPORTESCAPTEUR  30000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIMOTEUR 40000L  // Durée d'initialisation du moteur
#define DELAIABRI   15000L  // Durée de déplacement de l'abri (25000L)
#define MOTOFF HIGH         // Etat pour l'arret du moteur
#define MOTON !MOTOFF

#define BKLIGHT 3     // Backlight LCD
#define BUZZER 17 //A3
#define NiveauAff 0      //TM1638

// Boutons poussoirs
#define BEXT 11 //11
#define BINT 13  //12
#define BTAB 14  //13

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
#define BoutonMA (analogRead(BMA)<300)

#define LedClavier  0
#define LedStatus   1
#define LedPark     2
#define LedOpt      3

//#define TelPark 1

bool Manuel = false;  // Mode manuel
bool LastPark = false;  // Dernier état de Park
bool StateAff = true; // Etat de l'affichage du TM1638 (ON/OFF)
int BKLEVEL = 20;  // PWM LCD
int LEDLEVEL = 15;  // Intensité des LEDs du coffret

bool ECLINT = false;
bool ECLEXT = false;
bool ECLTAB = false;
