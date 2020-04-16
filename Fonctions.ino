//---------------------------------------FONCTIONS------------------------------
// Allumage d'une LED
void Led(byte led, byte R, byte V, byte B, bool refresh) {
  pixels.setPixelColor(led, pixels.Color(R, V, B));
  if (refresh) pixels.show();
}
// Allumage d'un barreau de LEDs
void Eclaire(byte barre, byte valeur, bool rouge) {
  for (byte i = 4 + 8 * barre; i < (12 + 8 * barre); i++) {
    pixels.setPixelColor(i, pixels.Color(valeur, valeur * rouge, valeur * rouge));
    pixels.show();
  }
}

// Fonction bip
void bip(byte pin, int freq, int delai) {
  tone(pin, freq, delai);
  delay(200);
  analogWrite(BKLIGHT, LEVEL[3]);
}

// Lecture des commandes depuis le port série
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

// Affichage d'un message d'info sur l'écran LCD
void msgInfo(String texte, byte type) {
  // Type: 0 mouvement en cours, 1 info, 2 erreur, 3 alerte
  switch (type) {
    case 0:	// Mouvement: LED bleue
      Led(LedStatus, 0 , 0 , LEVEL[4], true);
      break;
    case 1:	//	Ok: LED verte
      Led(LedStatus, LEVEL[4], 0, 0, true);
      break;
    case 2:	// Alerte: LED mauve
      Led(LedStatus, 0, LEVEL[4], LEVEL[4], true);
      break;
    case 3:	// Alarme (ARU) LED rouge
      Led(LedStatus, 0, LEVEL[4], 0, true);
      break;
  }
  texte += "                    ";
  texte = texte.substring(0, 15);
  lcd.setCursor(0, 1);
  lcd.print(texte);
  lcd.setCursor(POS * 3, 3);
}

// Ferme la petite porte
void fermePorte1(void) {
  msgInfo("P1 F...", 0);
  digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  msgInfo("P1 Close", 1);
  digitalWrite(P11, HIGH);
  // Arret de l'alimentation
  digitalWrite(ALIM12V, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  digitalWrite(ALIM12V, LOW); // Mise en marche de l'alimentation ATX
  delay(2000);
  msgInfo("P1 O...", 0);
  digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  msgInfo("P1 OPEN", 1);
  digitalWrite(P12, HIGH);
}

// Ferme la porte2
void fermePorte2(void) {
  digitalWrite(P21, LOW);
  delay(DELAIPORTES);
  digitalWrite(P21, HIGH);
  DomeStop();
}

// Ouvre la porte2
void ouvrePorte2(void) {
  digitalWrite(P22, LOW);
  delay(DELAIPORTES);
  digitalWrite(P22, HIGH);
}

// Change la position des portes 0: ouverture 1 fermeture
bool changePortes(bool etat) {
  bool alimOn = false;
  if (!Alim12VStatus) {
    digitalWrite(ALIM12V, LOW);
    delay(2000);
    alimOn = true;
  }
  // Commande identique à l'état actuel, on sort
  if ((etat && PortesOuvert) || (!etat && PortesFerme)) {
    msgInfo("Erreur position", 2);
    if (alimOn)   digitalWrite(ALIM12V, HIGH);
    return false;
  }
  if (etat) {   // Ouverture des portes
    DomeStart();
    // Ouverture des portes
    lcd.backlight();
    msgInfo("P1 O...", 0);
    digitalWrite(P12, LOW);
    delay(5000); //if (!attendARU(5000, true, true, false)) return false;
    digitalWrite(P22, LOW);
    msgInfo("P12 O...", 0);
    delay(DELAIPORTESCAPTEUR); //if (!attendARU(DELAIPORTESCAPTEUR, true, true, false)) return false; // Délai minimum
    // On attend que les portes sont ouvertes
    while (!PortesOuvert) {
      delay(100); //if (!attendARU(100, true, true, false)) return false;
    }
    // Délai pour finir le mouvement
    delay(5000); //if (!attendARU(5000, true, true, false)) return false;
    digitalWrite(P12, HIGH);
    digitalWrite(P22, HIGH);
    msgInfo("P12 Open", 1);
    Ser2.println("PO#");
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      msgInfo("Erreur position", 2);
      return false;
    }
    Led(LedStatus, LEVEL[4], LEVEL[4], 0, true);
    StopMot;
    msgInfo("P2 F...", 1);
    digitalWrite(P21, LOW);
    if (!attendARU(INTERVALLEPORTES, true, true, false)) return false;
    msgInfo("P12 F...", 0);
    digitalWrite(P11, LOW);
    if (!attendARU(DELAIPORTES, true, true, false)) return false;
    digitalWrite(P11, HIGH);
    digitalWrite(P21, HIGH);
    // Arret des éclairages
    Ser2.println("PF#");
    DomeStop();
  }
  return true;
}
// Déplacement du dome sans vérifications
void DeplaceDomeARU(void) {
  /* Déplace le dome sans vérification /!\ */
  digitalWrite(MOTEUR, LOW);
  delay(600);
  digitalWrite(MOTEUR, HIGH);
}

// Déplacement de l'abri 1: ouverture 0: fermeture
bool deplaceAbri(bool etat) {
  bool alimOn = false;
  if (!Alim12VStatus) {
    digitalWrite(ALIM12V, LOW);
    delay(2000);
    alimOn = true;
  }
  // Commande identique à l'état actuel, on sort
  if ((etat && AbriOuvert) || (!etat && AbriFerme)) {
    msgInfo("Erreur position", 2);
    if (alimOn)   digitalWrite(ALIM12V, HIGH);
    return false;
  }
  // Test telescope parqué
  if (!TelPark) {
    msgInfo("Erreur Park", 2);
    // Tentative de parquer le télescope
    Ser2.write("PA#");
    // Attente de 3mn maxi
    byte n = 18;
    while (!TelPark && n > 0) {
      n--;
      delay(10000L);
    }
    if (!TelPark && TType) {
      if (alimOn)   digitalWrite(ALIM12V, HIGH);
      return false;
    }
  }
  if (!TType) StopTel; // Coupure alimentation télescope

  Led(LedStatus, LEVEL[4], LEVEL[4], 0, true);
  if (!PortesOuvert) {
    if (!MoteurStatus) StartMot; // Alimentation du moteur
    changePortes(true);    //Ouverture des portes
  }
  else if (!MoteurStatus) {
    // Attente d'initialisation du moteur de l'abri
    StartMot;
    //Attente pour l'initialisation du moteur
    if (!attendARU(DELAIMOTEUR, true, true, false)) return false; // Protection contre les déplacements intempestifs
  }
  // Deplacement de l'abri
  msgInfo("Deplacement abri...", 0);
  Eclaire(2, 100, 1);
  while (AbriFerme || AbriOuvert) {
    digitalWrite(MOTEUR, LOW);
    delay(600);
    digitalWrite(MOTEUR, HIGH);
    attendARU(4000, true, false, true);	// Attente pour voir si la commande est bien passée
  }
  if (!attendARU(DELAIABRI, true, false, true)) return false;
  while (!AbriFerme && !AbriOuvert) {
    if (!attendARU(1000, true, false, true)) return false;
  }
  if (!attendARU(2000, true, false, true)) return false;   // Finir le déplacement
  Eclaire(2, 0, 1);
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat = AbriOuvert;
  if (!etat) {
    // Abri fermé
    if (!TType) StopTel; // Coupure alimentation télescope
    msgInfo("Abri fermé", 1);
    delay(500);
    changePortes(false);             // Fermeture des portes
    Ser2.println("DF#");
    // Pas nécessaire (déjà fait à la fermeture des portes)
    //StopMot; // Coupure alimentation moteur abri
    //StopTel; // Coupure alimentation dome
  }
  else {
    Ser2.println("DO#");
    msgInfo("Abri ouvert", 1);
    StartTel;
  }
  bip(BUZZER, 440, 1000);
  return true;
}

// Boucle d'attente pendant le déplacement de l'abri
bool attendARU(unsigned long delai, bool park, bool depl, bool portes) {
  byte ERRMAX = 3;
  byte nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    SerMsg = LireCmd();
    if (SerMsg == "AU") {
      Serial.println("0");
      ARU("Série");
      return false;
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (park) {
      if (!TelPark && TType) nbpark++;
      if (nbpark >= ERRMAX) {
        ARU("Park");
        return false;
      }
    }
    if (depl) {
      // Si le dome se déplace pendant le mouvement des portes: ARU
      if (!AbriFerme && !AbriOuvert) {
        ARU("Position");
        return false;
      }
    }
    if (portes) {
      // Vérifie si les portes ne bougent pas
      if (!PortesOuvert) {
        ARU("Portes");
        return false;
      }
    }
    // Bouton Arret d'urgence
    if (!mcp.digitalRead(BARU)) {
      ARU("Bouton");
      return false;
    }
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
  return true;
}

// Commande d'arret d'urgence
void ARU(String msg) {        // Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  Serial.println("ARU "+msg);
  digitalWrite(ALIMTEL, HIGH);
  digitalWrite(ALIMMOT, HIGH);
  digitalWrite(MOTEUR, HIGH);
  digitalWrite(P11, HIGH);
  digitalWrite(P12, HIGH);
  digitalWrite(P21, HIGH);
  digitalWrite(P22, HIGH);
  digitalWrite(ALIMMOT, MOTOFF);
  // Passage en mode manuel
  Manuel = true;
  msgInfo("ARU ! " + msg, 3);
  Led(LedStatus, 0, 50, 0, true);
  lcd.backlight();
  // Ouverture des portes
  //changePortes(true);
  // ouvrePorte1();
  // Attente tant que le bouton Arret d'urgence est appuyé
  while (!mcp.digitalRead(BARU)) {
    delay(100);
  }
}
// Initialisation du dome
void DomeStart() {
  if (timeroff) {
    timeroff = false;
    timer.deleteTimer(idTimer);
  }

  // Alimentation 12V
  digitalWrite(ALIM12V, LOW);
  delay(2000);
  // LEDs
  Led(LedStatus, LEVEL[4], 0, 0, false);
  Led(LedPark, LEVEL[4], 0, 0, false);
  Led(LedClavier, LEDLVLCLAV, LEDLVLCLAV, LEDLVLCLAV, true);
  // LCD
  analogWrite(BKLIGHT, LEVEL[3]);
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 2);
  lcd.print("TA IN EX LC LD CMD");
  lcd.setCursor(0, 3);
  lcd.print("2R 3B 5B 2  2  PARK ");
  lcd.setCursor(POS * 3, 3);
  lcd.blink();
  msgInfo("Ok", 1);
  //StartTel; // Alimentation 12V du télescope
  StartMot; // Alimentation du moteur de l'abri
  LastPark = !TelPark;  // Réactive la LED park
  Lock = false; // Clavier activé
  Veille = false;	// Le clavier reste éclairé tant que le dome est ouvert

}
// Arret du dome
void DomeStop() {
  //LCD
  //lcd.clear();
  lcd.noBacklight();
  //LEDs
  pixels.clear();
  pixels.show();
  StopMot;
 
  if (!TType) {
    idTimer = timer.setTimeout(TPSPARK * 1000L, StopAlim12Vtimer);
    timeroff = true;
  }
  else {
    // Alimentation 12V
    digitalWrite(ALIM12V, HIGH);
  }
  delay(1000);
  LastPark = true;	// Désactive l'affichage de l'état du park
  Lock = true;	// Clavier locké
  Veille = false;
}

// Eclairage du clavier
void EclaireClavier() {
  // Allume la LED pendant 5mn
  Led(LedClavier, LEDLVLCLAV, LEDLVLCLAV, LEDLVLCLAV, true);
  // Demarre le temporisateur
  if (!Veille) {
    timer.setTimeout(TPSVEILLE, EteintClavier);
    Veille = true;
  }
}
// Eteint l'éclairage du clavier
void EteintClavier() {
  if (Veille) {
    Led(LedClavier, 0, 0, 0, true);
    Lock = true;
    Veille = false;
  }
}
// Déverrouillage au clavier
bool ClavierCode(char key) {
  String code = String(key);
  unsigned long previousMillis = millis();
  int delai = 10; // Delai pour rentrer le code en secondes
  // TODO Allumage de la LED en orange pour éclairer le clavier
  EclaireClavier();
  while (true) {
    unsigned long currentMillis = millis();
    // Attente d'une touche ou de la fin du délai
    if (currentMillis - previousMillis >= (delai * 1000)) {
      Serial.println("Mauvais code");
      bip(BUZZER, 220, 300);
      //Led(LedClavier,0,0,0,true);
      return false;
    }
    key = kpd.get_key();
    if (key != '\0') {
      bip(BUZZER, 440, 100);
      Led(LedClavier, 0, LEDLVLCLAV, LEDLVLCLAV, true);
      delay(200);
      Led(LedClavier, LEDLVLCLAV, LEDLVLCLAV, LEDLVLCLAV, true);
      // Touche pressée
      code = code + key;
      byte lg = 0;
      if (code.length() > 4) {
        lg = code.length() - 4;
      }
      code = code.substring(lg);
      Serial.println(code);
      if (code == SECRET) {
        Serial.println("Code OK");
        // TODO Allume la LED en vert pendant 2s
        bip(BUZZER, 440, 300);
        Led(LedClavier, LEDLVLCLAV, 0, 0, true);
        delay(500);
        return true;
      }
    }
  }
}
// Mise à jour de l'affichage LCD, des LEDs
void MajLCD() {
  lcd.setCursor(POS * 3, 3);
  if (POS < 3) {
    if (niveau[POS] > 4) {
      lcd.print(niveau[POS] - 4);
      lcd.print("B");
      REDLED[POS] = true;
      LEVEL[POS] = APALEV[niveau[POS] - 5];
      Eclaire(2 - POS, LEVEL[POS] * ECLSTAT[POS], REDLED[POS]);
      // TODO
    }
    else {
      lcd.print(niveau[POS] + 1);
      lcd.print("R");
      REDLED[POS] = false;
      LEVEL[POS] = APALEV[niveau[POS]];
      Eclaire(2 - POS, LEVEL[POS] * ECLSTAT[POS], REDLED[POS]);
      // TODO changement d'intensité des éclairages
    }
  }
  else if (POS == 4) {
    lcd.print(niveau[POS]);
    LEVEL[4] = LEDLEV[niveau[POS]];
    Led(LedStatus, LEVEL[4], 0, 0, true);
    Led(LedPark, LEVEL[4] * TelPark, 0, 0, true);
  }
  else if (POS == 3) {
    lcd.print(niveau[POS]);
    LEVEL[3] = LCDLEV[niveau[POS]];
    analogWrite(BKLIGHT, LEVEL[3]);
  }
  else {
    // Commande
    switch (niveau[POS]) {
      case 0:
        lcd.print("PARK ");
        break;
      case 1:
        lcd.print("OU P1");
        break;
      case 2:
        lcd.print("FE P1");
        break;
      case 3:
        lcd.print("OU AB");
        break;
      case 4:
        lcd.print("FE AB");
        break;
      case 5:
        lcd.print("OU PO");
        break;
      case 6:
        lcd.print("FE PO");
        break;
      case 7:
        lcd.print((Manuel) ? "AUTO " : "MANU ");
        break;
      case 8:
        lcd.print("OU P2");
        break;
      case 9:
        lcd.print("FE P2");
        break;
      case 10:
        lcd.print("DEPLA");
        break;
      case 11:
        lcd.print("MOTON");
        break;
      case 12:
        lcd.print("ATX M");
        break;
      case 13:
        lcd.print("ATX A");
        break;
      case 14:
        lcd.print("AUTO ");
        break;
    }
  }
  lcd.setCursor(POS * 3, 3);
  delay(200);
}

void StopAlim12Vtimer() {
  timeroff = false;
  StopTel;
  delay(100);
  digitalWrite(ALIM12V, HIGH);
}
