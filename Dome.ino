/*
  Pilotage automatique de l'abri du telescope
  Serge CLAUS
  GPL V3
  Version 2.8
  22/10/2018-06/11/2019
*/

#include "Config.h"

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  // Initialisation des ports série
  Serial.begin(57600);	// Connexion à ESP-Link (ESP8266)
  Ser2.begin(9600);		// Connexion à ESP32 LoRa

  // MCP23017 Gestion des entrées capteurs, park...
  mcp.begin();

  // Clavier matriciel 4x4
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
  mcp.pinMode(PARK, INPUT); 		//Résistance pulldown de 10k
  pinMode(BMA, INPUT);				//Bouton Marche/arret (pour l'instant non cablé)
  pinMode(BKLIGHT, OUTPUT);			//Sortie rétro-éclairage LCD
  analogWrite(BKLIGHT, LEVEL[3]);

  // Boutons poussoirs
  mcp.pinMode(BINT, INPUT); mcp.pullUp(BINT, HIGH);
  mcp.pinMode(BEXT, INPUT); mcp.pullUp(BEXT, HIGH);
  mcp.pinMode(BTAB, INPUT); mcp.pullUp(BTAB, HIGH);
  mcp.pinMode(BVALID, INPUT); mcp.pullUp(BVALID, HIGH);
  mcp.pinMode(BCHOIX, INPUT); mcp.pullUp(BCHOIX, HIGH);
  mcp.pinMode(BSEL, INPUT); mcp.pullUp(BSEL, HIGH);
  // LEDs
  pixels.begin();
  pixels.clear();
  pixels.show();

  // LCD
  lcd.init();
  lcd.setCursor(0, 2);
  lcd.print("TA IN EX LC LD CMD");
  lcd.setCursor(0, 3);
  lcd.print("2R 3B 5B 2  2  PARK");
  lcd.setCursor(POS * 3, 3);
  lcd.blink();

  // Timer
  timer.setInterval(1000, FuncSec);

  if (PortesOuvert) {
    DomeStart();
  }
  else {
    DomeStop();
  }
  // Vérification de la position du dome au démarrage
  if (!AbriOuvert && !AbriFerme) {
    // Position incorrecte on passe en mode manuel
    Manuel = true;
  }
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg = "";		// Message reçu sur le port série

void loop() {
  timer.run(); // Timer pour la LED clavier
  // Lecture des boutons poussoirs
  if (!mcp.digitalRead(BINT)) {
    ECLINT = !ECLINT;
    bip(BUZZER, 440, 100);
    Eclaire(1, LEVEL[1] * ECLINT, REDLED[1]);
    delay(300);
  }
  if (!mcp.digitalRead(BTAB)) {
    ECLTAB = !ECLTAB;
    bip(BUZZER, 440, 100);
    Eclaire(2, LEVEL[0] * ECLTAB, REDLED[0]);
    delay(300);
  }

  if (!mcp.digitalRead(BEXT)) {
    ECLEXT = !ECLEXT;
    bip(BUZZER, 440, 100);
    Eclaire(0, LEVEL[2] * ECLEXT, REDLED[2]);
    delay(300);
  }
  // IHM
  if (!mcp.digitalRead(BVALID)) {
    if (Lock) {
      // Active l'afficheur LCD
      lcd.backlight();
      Lock = false;
      delay(200);
    }
    else if (POS == 5) {
      // Lance la commande demandée
      switch (niveau[5]) {
        case 0:
          Serial.println("Park");
          Ser2.write("PA#");
          break;
        case 1:
          ouvrePorte1();
          break;
        case 2:
          fermePorte1();
          break;
        case 3:
          deplaceAbri(true);
          break;
        case 4:
          deplaceAbri(false);
          break;
        case 5:
          changePortes(true);
          break;
        case 6:
          changePortes(false);
          break;
      }
      delay(200);
    }
	else {
		// Commande -
		if (niveau[POS]>0) {
			niveau[POS]--;
			MajLCD();
		}
	}
  }
  // Touche de déplacement (2)
  if (!mcp.digitalRead(BSEL)) {
    POS++;
    if (POS > 5) POS = 0;
    lcd.setCursor(POS * 3, 3);
    delay(200);
  }
  // Changement de valeur (3)
  if (!mcp.digitalRead(BCHOIX)) {
    niveau[POS]++;
    if (niveau[POS] > 5 && POS > 2 && POS < 5) niveau[POS] = 1;
    if (niveau[POS] > 6 && POS == 5) niveau[POS] = 0;
    if (niveau[POS] > 10 ) niveau[POS] = 1;
	MajLCD();
  }

  // Lecture des boutons du clavier
  //byte keys = module.getButtons();
  char key = kpd.get_key();
  if (key != '\0') {
    bip(BUZZER, 440, 100);
    // Tone désactive le pwm sur la sortie 3
    if (Lock) {
      if (ClavierCode(key)) {
        Lock = false;
      }
    }
    else {

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
	  else if (key == "1") {
	      ECLEXT = !ECLEXT;
		Eclaire(0, LEVEL[2] * ECLEXT, REDLED[2]);
		delay(300);
	  }
	  else if (key =="2") {
		ECLINT = !ECLINT;
		Eclaire(1, LEVEL[1] * ECLINT, REDLED[1]);
		delay(300);
	  }
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
      ARU("Série");
    }
    else if (SerMsg == "M?") {
      Serial.println(Manuel ? "m" : "a");
    }
    else if (SerMsg == "E+") {
      Eclaire(0, LEVEL[2], REDLED[2]);
      Serial.println("1");
    }
    else if (SerMsg == "E-") {
      Eclaire(0, 0, false);
      Serial.println("0");
    }

    else if (SerMsg == "I+") {
      Eclaire(1, LEVEL[1], REDLED[1]);
      Serial.println("1");
    }
    else if (SerMsg == "I-") {
      Eclaire(1, 0, false);
      Serial.println("0");
    }
    else if (SerMsg == "PA") {
      Ser2.print("PA#");
      Serial.println(Ser2.readStringUntil('\r'));

    }
    else if (SerMsg == "HO") {
      Ser2.write("HO#");
      Serial.println(Ser2.readStringUntil('\r'));
    }
    else if (SerMsg == "FN") {
      Ser2.write("FN#");
      Serial.println(Ser2.readStringUntil('\r'));
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
    Led(LedPark, LEVEL[4] * TelPark, 0, 0, true);
    LastPark = TelPark;
	Ser2.println(TelPark?"TN":"TP");
  }

  // TEST DEPLACEMENT INOPINE DU DOME
  // TODO à décommenter quand installé

  if (!Manuel && !AbriFerme && !AbriOuvert) {
    ARU("Position");
  }

  // Bouton Arret d'urgence
  if (!mcp.digitalRead(BARU)) {
    ARU("Bouton");
  }
  // Bouton Marche/Arret ? on ouvre la petite porte
  if (BoutonMA) {
    ouvrePorte1();
  }

}
