/*
  Pilotage automatique de l'abri du telescope
  Serge CLAUS
  GPL V3
  Version 3.0
  22/10/2018-16/04/2020
*/

#include "Config.h"

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  // Initialisation des ports série
  Serial.begin(57600);	// Connexion à ESP-Link (ESP8266)
  Ser2.begin(9600);		// Connexion à ESP8266 espDome

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
  mcp.pinMode(TSEL, INPUT); mcp.pullUp(TSEL, HIGH);
  mcp.pinMode(PARK, INPUT); 		//Résistance pulldown de 10k
  //pinMode(BMA, INPUT);				//Bouton Marche/arret (pour l'instant non cablé)
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

  // Démarrage de l'alimentation 12V (pour la lecture des capteurs)
  digitalWrite(ALIM12V, LOW);
  delay(5000);

  if (AbriOuvert && !AbriFerme) StartTel; else StopTel;	// Alimentation 12V du télescope
  // Vérification de la position du dome au démarrage
  if (!AbriOuvert && !AbriFerme) {
    // Position incorrecte on passe en mode manuel
    Manuel = true;
  }
  // Etat actuel de l'abri
  if (PortesOuvert) {
    DomeStart();
  }
  else {
    DomeStop();
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
  // IHM Menus de l'écran LCD
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
          delay(300);
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
        case 7:
          Manuel = !Manuel;
          if (Manuel) Led(LedStatus, LEVEL[4], LEVEL[4], 0, true); else Led(LedStatus, LEVEL[4], 0, 0, true);
          MajLCD();
          delay(800);
          break;
        case 8:
          ouvrePorte2();
          break;
        case 9:
          fermePorte2();
          break;
        case 10:
          DeplaceDomeARU();
          break;
        case 11:
          StartMot;
          break;
        case 12:
          digitalWrite(ALIM12V, LOW);
          delay(1000);
          break;
        case 13:
          digitalWrite(ALIM12V, HIGH);
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
    lcd.backlight();
    POS++;
    if (POS > 5) POS = 0;
    lcd.setCursor(POS * 3, 3);
    delay(200);
  }
  // Changement de valeur (3)
  if (!mcp.digitalRead(BCHOIX)) {
    niveau[POS]++;
    if (niveau[POS] > 4 && POS > 2 && POS < 5) niveau[POS] = 0;
    if (niveau[POS] > 7 && POS == 5 && !Manuel) niveau[POS] = 0;
    if (niveau[POS] > 13 && POS == 5) niveau[POS] = 0;
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
      if (key == '*') { // A
        deplaceAbri(true);
      }
      else if (key == '#') { //B
        deplaceAbri(false);
      }
      else if (key == 'A') { //C
        changePortes(true);
      }
      else if (key == 'B') { //D
        changePortes(false);
      }
      else if (key == 'C') {
        ouvrePorte1();
      }
      else if (key == 'D') {
        fermePorte1();
      }
      else if (key == '1') {
        ECLSTAT[2] = !ECLSTAT[2];
        Eclaire(0, LEVEL[2] * ECLSTAT[2], REDLED[2]);
        delay(300);
      }
      else if (key == '2') {
        ECLSTAT[1] = !ECLSTAT[1];
        Eclaire(1, LEVEL[1] * ECLSTAT[1], REDLED[1]);
        delay(300);
      }
      else if (key == '9') {
        Ser2.println("PA#");
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

    /*   // Aide
       if (SerMsg == "??") {
         Serial.println("Manuel:");
         Serial.println("OK      Retour en mode automatique");
         Serial.println("m+ / m- Alimentation moteur déplacement");
         Serial.println("2+ / 2- Porte 2");
         Serial.println("dd  /!\\ Déplacement inconditionnel");
         Serial.println("Auto/manuel:");
         Serial.println("A? / A+ / A- Alimentation télescope");
         Serial.println("X? / X+ X X- Alimentation ATX 12V");
         Serial.println("P?           Etat des portes");
         Serial.println("D?           Etat du dome");
         Serial.println("T?           Type de télescope");
         Serial.println("p+ / p-      Petite porte");
         Serial.println("AU           ARRET D'URGENCE");
         Serial.println("M?           Mode (manuel/auto)");
         Serial.println("E+ / E-      Eclairage extérieur");
         Serial.println("I+ / I-      Eclairage intérieur");
         Serial.println("PA           Park du télescope");
         Serial.println("Auto/manuel:");
         Serial.println("C?  Informations (abr ferm, abr ouv, po ferm, po ferm, alim tél, park, man/auto");
         Serial.println("    Capteurs portes pf1, pf2, po1, po2");
         Serial.println("Auto:");
         Serial.println("P+ / P-  Portes");
         Serial.println("D+ / D-  Dome");

       }
    */
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
    else if ( SerMsg == "X?" ) {
      Serial.println(Alim12VStatus ? "1" : "0");
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
      Serial.println(AlimTelStatus ? "1" : "0");
    }
    else if (SerMsg == "T?") {
      Serial.println(TType ? "long" : "court");
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
      Ser2.println("PA#");
    }
    else if (SerMsg == "C?") {
      Serial.print(AbriFerme);
      Serial.print(AbriOuvert);
      Serial.print(PortesFerme);
      Serial.print(PortesOuvert);
      Serial.print(Alim12VStatus);
      Serial.print(AlimTelStatus);
      Serial.print(TelPark ? "p" : "n");
      Serial.println(Manuel ? "m" : "a");
      Serial.print(mcp.digitalRead(Pf1));
      Serial.print(mcp.digitalRead(Pf2));
      Serial.print(mcp.digitalRead(Po1));
      Serial.println(mcp.digitalRead(Po2));
    }
    else if (SerMsg == "DI") {
      Ser2.println(!AbriOuvert);
    }
    else if (SerMsg == "PI") {
      Ser2.println(PortesOuvert);
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

  if (Alim12VStatus && !Manuel && !AbriFerme && !AbriOuvert) {
    ARU("Position");
  }

  /* TODO Réactiver s'il y a un bouton cablé
    // Bouton Arret d'urgence
    if (!mcp.digitalRead(BARU)) {
     ARU("Bouton");
    }
  */
  /*  // Bouton Marche/Arret ? on ouvre la petite porte
    if (BoutonMA) {
      ouvrePorte1();
    }
  */
}
