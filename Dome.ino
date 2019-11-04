/*
  Pilotage automatique de l'abri du telescope
  Serge CLAUS
  GPL V3
  Version 2.6
  22/10/2018-19/10/2019
*/

#include "Config.h"

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial.begin(57600);
  Ser2.begin(9600);
  // MCP23017
  mcp.begin();

  // Clavier matriciel
  kpd.init();

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
  analogWrite(BKLIGHT, BKLEVEL);

  // Boutons poussoirs
  mcp.pinMode(BINT, INPUT); mcp.pullUp(BINT, HIGH);
  mcp.pinMode(BEXT, INPUT); mcp.pullUp(BEXT, HIGH);
  mcp.pinMode(BTAB, INPUT); mcp.pullUp(BTAB, HIGH);
  // LCD
  lcd.init();
  if (PortesOuvert) {
    lcd.backlight();
  }
  msgInfo("Demarrage...   ");

  // LEDs
  pixels.begin();
  pixels.clear();
  if (PortesOuvert) {
    Led(LedStatus, LEDLEVEL, 0, 0, false);
    Led(LedPark, LEDLEVEL, 0, 0, false);
  }
  else {
    LastPark = true; // Empêche la LED de park de s'allumer
  }
  Led(LedClavier, LEDLEVEL, LEDLEVEL, LEDLEVEL, true);
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
  msgInfo("Ok");
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg = "";		// Message reçu sur le port série

void loop() {
  // Lecture des boutons poussoirs
  if (!mcp.digitalRead(BINT)) {
    ECLINT = !ECLINT;
    bip(BUZZER, 440, 100);
    Eclaire(1, 100 * ECLINT, 100 * ECLINT, 100 * ECLINT);
    delay(300);
  }
  if (!mcp.digitalRead(BTAB)) {
    ECLTAB = !ECLTAB;
    bip(BUZZER, 440, 100);
    Eclaire(2, LEDLEVEL * 2 * ECLTAB, 0 , 0);
    delay(300);
  }

  if (!mcp.digitalRead(BEXT)) {
    ECLEXT = !ECLEXT;
    bip(BUZZER, 440, 100);
    Eclaire(0, 100 * ECLEXT, 100 * ECLEXT, 100 * ECLEXT);
    delay(300);
  }

  // Lecture des boutons du clavier
  //byte keys = module.getButtons();
  char key = kpd.get_key();

  if (key != '\0') {
    bip(BUZZER, 440, 100);
    // Tone désactive le pwm sur la sortie 3

    //Serial.print(key);
    if (key == 'A') {
      deplaceAbri(true);
    }
    else if (key == 'B') {
      deplaceAbri(false);
    }
    else if (key == 'C') {
      changePortes(true);
    }
    else if (key == 'D') {
      changePortes(false);
    }
    else if (key == '*') {
      ouvrePorte1();
    }
    else if (key == '#') {
      fermePorte1();
    }
  }
  // Lecture des données des ports série
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
    else if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
    }
    else if (SerMsg == "M?") {
      Serial.println(Manuel ? "m" : "a");
    }
    else if (SerMsg == "E+") {
      Eclaire(0, 100, 100, 100);
      Serial.println("1");
    }
    else if (SerMsg == "E-") {
      Eclaire(0, 0, 0, 0);
      Serial.println("0");
    }

    else if (SerMsg == "I+") {
      Eclaire(1, 100, 100, 100);
      Serial.println("1");
    }
    else if (SerMsg == "I-") {
      Eclaire(1, 0, 0, 0);
      Serial.println("0");
    }
    else if (SerMsg == "C?") {
      Serial.print(AbriFerme);
      Serial.print(AbriOuvert);
      Serial.print(PortesFerme);
      Serial.print(PortesOuvert);
      Serial.print(AlimStatus);
      Serial.print(TelPark ? "p" : "n");
      Serial.println(Manuel ? "m" : "a");
      Serial.print(mcp.digitalRead(Pf1));
      Serial.print(mcp.digitalRead(Pf2));
      Serial.print(mcp.digitalRead(Po1));
      Serial.println(mcp.digitalRead(Po2));
    }
  }

  // LED état park

  if (LastPark != TelPark) {
    Led(LedPark, LEDLEVEL * TelPark, 0, 0, true);
    LastPark = TelPark;
  }

  // TEST DEPLACEMENT INOPINE DU DOME
  // TODO à décommenter quand installé

  if (!Manuel && !AbriFerme && !AbriOuvert) {
    ARU();
  }

  // Bouton Arret d'urgence
  if (!mcp.digitalRead(BARU)) {
    ARU();
  }
  // Bouton Marche/Arret ? on ouvre la petite porte
  if (BoutonMA) {
    ouvrePorte1();
  }

}
