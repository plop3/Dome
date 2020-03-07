/*
  Pilotage automatique de l'abri du telescope
  Serge CLAUS
  GPL V3
  Version 2.9
  22/10/2018-14/01/2020
*/

#include "Config.h"

//#define REST

#ifdef REST
#include <ELClient.h>
#include <ELClientRest.h>
ELClient esp(&Serial, &Serial);
ELClientRest rest(&esp);
#endif
#ifdef MQTT
//#include <ELClientCmd.h>
#include <ELClientMqtt.h>
ELClient esp(&Serial, &Serial);
//ELClientCmd cmd(&esp);
ELClientMqtt mqtt(&esp);
#endif

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  // Initialisation des ports série
  Serial.begin(57600);	// Connexion à ESP-Link (ESP8266)
  //Ser2.begin(9600);		// Connexion à ESP32 LoRa

  // MCP23017 Gestion des entrées capteurs, park...
  mcp.begin();

  // Clavier matriciel 4x4
  kpd.init();

  // Initialisation des relais
  digitalWrite(ALIM12V, HIGH); pinMode(ALIM12V, OUTPUT);
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

#ifdef REST || MQTT
  // MQTT/Rest
  byte ok = 30;
  do {
    ok--;
    if (esp.Sync()) ok = 0;    // sync up with esp-link, blocks for up to 2 seconds
    if (ok) Serial.println("EL-Client sync failed!");
  } while (ok);
  Serial.println("EL-Client synced!");
#endif
#ifdef REST
  rest.begin("192.168.0.27:1789");
#endif
#ifdef MQTT
  // TODO
#endif

  if (PortesOuvert) {
    DomeStart();
  }
  else {
    DomeStop();
  }
  if (AbriOuvert && !AbriFerme) StartTel; else StopTel;	// Alimentation 12V du télescope
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
    ECLSTAT[1] = !ECLSTAT[1];
    bip(BUZZER, 440, 100);
    Eclaire(1, LEVEL[1] * ECLSTAT[1], REDLED[1]);
    delay(300);
  }
  if (!mcp.digitalRead(BTAB)) {
    ECLSTAT[0] = !ECLSTAT[0];
    bip(BUZZER, 440, 100);
    Eclaire(2, LEVEL[0] * ECLSTAT[0], REDLED[0]);
    delay(300);
  }

  if (!mcp.digitalRead(BEXT)) {
    ECLSTAT[2] = !ECLSTAT[2];
    bip(BUZZER, 440, 100);
    Eclaire(0, LEVEL[2] * ECLSTAT[2], REDLED[2]);
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
        //        case 0:
        //          Serial.println("Park");
        //          Ser2.write("PA#");
        //          break;
        //mqtt.publish("/Dome/0", "ParkMount");
        case 0:
          ouvrePorte1();
          break;
        case 1:
          fermePorte1();
          break;
        case 2:
          deplaceAbri(true);
          break;
        case 3:
          deplaceAbri(false);
          break;
        case 4:
          changePortes(true);
          break;
        case 5:
          changePortes(false);
          break;
        case 6:
          Manuel = !Manuel;
          if (Manuel) Led(LedStatus, LEVEL[4], LEVEL[4], 0, true); else Led(LedStatus, LEVEL[4], 0, 0, true);
          MajLCD();
          delay(800);
          break;
        case 7:
          if (Manuel) ouvrePorte2();
          break;
        case 8:
          if (Manuel) fermePorte2();
          break;
        case 9:
          if (Manuel) DeplaceDomeARU();
          break;
        case 10:
          if (Manuel) StartMot;
          break;
        case 11:
          if (Manuel) digitalWrite(ALIM12V, LOW);
          delay(1000);
          break;
        case 12:
          if (Manuel) digitalWrite(ALIM12V, HIGH);
          break;
      }
    }
    else {
      // Commande -
      if (niveau[POS] > 0) {
        niveau[POS]--;
        MajLCD();
        delay(200);

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
    if (niveau[POS] > 4 && POS > 2 && POS < 5) niveau[POS] = 0;
    if (niveau[POS] > 6 && POS == 5 && !Manuel) niveau[POS] = 0;
    if (niveau[POS] > 12 && POS == 5) niveau[POS] = 0;
    if (niveau[POS] > 9 && POS < 5) niveau[POS] = 0;
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
      // TODO Changement temporaire des touches (problème avec le clavier)
      if (key == '5') { // A
        deplaceAbri(true);
      }
      else if (key == '6') { //B
        deplaceAbri(false);
      }
      else if (key == '8') { //C
        changePortes(true);
      }
      else if (key == '9') { //D
        changePortes(false);
      }
      else if (key == '*') {
        ouvrePorte1();
      }
      else if (key == '#') {
        fermePorte1();
      }
      else if (key == "1") {
        ECLSTAT[2] = !ECLSTAT[2];
        Eclaire(0, LEVEL[2] * ECLSTAT[2], REDLED[2]);
        delay(300);
      }
      else if (key == "2") {
        ECLSTAT[1] = !ECLSTAT[1];
        Eclaire(1, LEVEL[1] * ECLSTAT[1], REDLED[1]);
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
    else if ( SerMsg == "X+" ) {
      digitalWrite(ALIM12V, LOW);
    }
    else if ( SerMsg == "X-" ) {
      digitalWrite(ALIM12V, HIGH);
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
      Serial.println("+ParkMount");
#ifdef REST
      rest.get("/");
#endif
    }
    /*
         else if (SerMsg == "HO") {
         Ser2.write("HO#");
         Serial.println(SerESP());
         }
         else if (SerMsg == "FN") {
         Ser2.write("FN#");
         Serial.println(SerESP());
         }
         else if (SerMsg == "EC") {
         Ser2.write("EC#");
         Serial.println(SerESP());
         }
    */
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
    Ser2.println(TelPark ? "TN" : "TP");
  }

  // TEST DEPLACEMENT INOPINE DU DOME
  // TODO à décommenter quand installé

  if (!Manuel && !AbriFerme && !AbriOuvert) {
    ARU("Position");
  }

  /* TODO Réactiver s'il y a un bouton cablé
    // Bouton Arret d'urgence
    if (!mcp.digitalRead(BARU)) {
     ARU("Bouton");
    }
  */
  // Bouton Marche/Arret ? on ouvre la petite porte
  if (BoutonMA) {
    ouvrePorte1();
  }

}
