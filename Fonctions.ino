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
void msgInfo(String texte) {
  texte += "                    ";
  texte = texte.substring(0, 15);
  lcd.setCursor(0, 1);
  lcd.print(texte);
  lcd.setCursor(POS * 3, 3);
}
// Ferme la petite porte
void fermePorte1(void) {
  msgInfo("P1 F...");
  digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  msgInfo("P1 Close");
  digitalWrite(P11, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  msgInfo("P1 O...");
  digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  msgInfo("P1 OPEN");
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
bool changePortes(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && PortesOuvert) || (!etat && PortesFerme)) {
    msgInfo("Erreur position");
    return false;
  }
  if (etat) {   // Ouverture des portes
    // Alimentation du moteur
    StartMot; // On allume assez tôt pour laisser le temps de s'initialiser
    // Ouverture des portes
    Led(LedStatus, LEVEL[4], LEVEL[4], 0, true);
    lcd.backlight();
    msgInfo("P1 O...");
    digitalWrite(P12, LOW);
    if (!attendPorte(5000)) return false;
    digitalWrite(P22, LOW);
    msgInfo("P12 O...");
    if (!attendPorte(DELAIPORTESCAPTEUR)) return false; // Délai minimum
    // On attend que les portes sont ouvertes
    while (!PortesOuvert) {
      if (!attendPorte(100)) return false;
    }
    // Délai pour finir le mouvement
    if (!attendPorte(5000)) return false;
    digitalWrite(P12, HIGH);
    digitalWrite(P22, HIGH);
    msgInfo("P12 Open");
    DomeStart();
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      msgInfo("Err POs");
      return false;
    }
    Led(LedStatus, LEVEL[4], LEVEL[4], 0, true);
    StopMot;
    msgInfo("P2 F...");
    digitalWrite(P21, LOW);
    if (!attendPorte(5000)) return false;
    msgInfo("P12 F...");
    digitalWrite(P11, LOW);
    if (!attendPorte(DELAIPORTES)) return false;
    digitalWrite(P11, HIGH);
    digitalWrite(P21, HIGH);
    // Arret des éclairages
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
  // Commande identique à l'état actuel, on sort
  if ((etat && AbriOuvert) || (!etat && AbriFerme)) {
    msgInfo("Err POS");
    return false;
  }
  // Test telescope parqué
  if (!TelPark) {
    msgInfo("Err PArk");
    // Tentative de parquer le télescope
    Ser2.write("PA#");
    Serial.println(Ser2.readStringUntil('\r'));
    // Attente de 3mn maxi
    byte n = 18;
    while (!TelPark && n > 0) {
      delay(10000L);
    }
    if (!TelPark) {
      return false;
    }
  }
  StopTel; // Coupure alimentation télescope
  Led(LedStatus, LEVEL[4], LEVEL[4], 0, true);
  if (!PortesOuvert) {
    if (!MoteurStatus) StartMot; // Alimentation du moteur
    changePortes(true);    //Ouverture des portes
  }
  else if (!MoteurStatus) {
    // Attente d'initialisation du moteur de l'abri
    StartMot;
    //Attente pour l'initialisation du moteur
    if (!attendPorte(DELAIMOTEUR)) return false; // Protection contre les déplacements intempestifs
  }
  // Deplacement de l'abri
  msgInfo("Deplacement abri...");
  while (AbriFerme || AbriOuvert) {
    digitalWrite(MOTEUR, LOW);
    delay(600);
    digitalWrite(MOTEUR, HIGH);
    attendDep(4000);	// Attente pour voir si la commande est bien passée
  }
  if (!attendDep(DELAIABRI)) return false;
  while (!AbriFerme && !AbriOuvert) {
    if (!attendDep(1000)) return false;
  }
  if (!attendDep(2000)) return false;      // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat = AbriOuvert;
  if (etat) {
    // Abri ouvert
    DomeStart();
  }
  else {
    // Abri fermé
    StopTel; // Coupure alimentation télescope
    msgInfo("Abri fermé");
    delay(500);
    changePortes(false);             // Fermeture des portes
    // Pas nécessaire (déjà fait à la fermeture des portes)
    StopMot; // Coupure alimentation moteur abri
    StopTel; // Coupure alimentation dome
  }
  bip(BUZZER, 440, 1000);
  return true;
}
// Boucle d'attente lors du déplacement
bool attendDep(unsigned long delai) { // Boucle d'attente pendant le déplacement de l'abri
  byte ERRMAX = 2;
  byte nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    SerMsg = LireCmd();
    if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
      return false;
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) {
      ARU();
      return false;
    }
    // Bouton Arret d'urgence
    if (!mcp.digitalRead(BARU)) {
      ARU();
      return false;
    }
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
  return true;
}
// Boucle d'attente pendant l'ouverture/fermeture des portes
bool attendPorte(unsigned long delai) {
  byte ERRMAX = 2;
  byte nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    SerMsg = LireCmd();
    if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
      return false;
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) {
      ARU();
      return false;
    }
    // Si le dome se déplace pendant le mouvement des portes: ARU
    if (!AbriFerme && !AbriOuvert) {
      ARU();
      return false;
    }
    // Bouton Arret d'urgence
    if (!mcp.digitalRead(BARU)) {
      ARU();
      return false;
    }
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
  return true;
}
// Commande d'arret d'urgence
void ARU() {        // Arret d'urgence
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
  // Passage en mode manuel
  Manuel = true;
  msgInfo("ARU !");
  Led(LedStatus, 0, 50, 0, true);
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
  // LEDs
  Led(LedStatus, LEVEL[4], 0, 0, false);
  Led(LedPark, LEVEL[4], 0, 0, false);
  Led(LedClavier, LEDLVLCLAV, LEDLVLCLAV, LEDLVLCLAV, true);
  // LCD
  // TODO Rétro-éclairage
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 2);
  lcd.print("TA IN EX LC LD CMD");
  lcd.setCursor(0, 3);
  lcd.print("2R 3B 5B 2  2  PARK");
  lcd.setCursor(POS * 3, 3);
  lcd.blink();
  msgInfo("Ok");
  StartTel; // Alimentation télescope
  StartMot; // Alimentation du moteur de l'abri
  LastPark = !TelPark;  // Réactive la LED park
  Lock = false; // Clavier activé
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
  StopTel;
  LastPark = true;	// Désactive l'affichage de l'état du park
  Lock = true;	// Clavier locké
}
// Fonction executée toutes les secondes
void FuncSec() {
  // Repositionne le curseur de l'afficheur
  //lcd.setCursor(POS * 3, 3);
}
// Eclairage du clavier
void EclaireClavier() {
  // Allume la LED
  Led(LedClavier, LEDLVLCLAV, LEDLVLCLAV, LEDLVLCLAV, true);
  // Demarre le temporisateur
  if (!Veille) {
    timer.setTimeout(TPSVEILLE * 1000, EteintClavier);
    Veille = true;
  }
}
// Eteint l'éclairage du clavier
void EteintClavier() {
  Led(LedClavier, 0, 0, 0, true);
  Veille = false;
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
        Led(LedClavier, 0, LEDLVLCLAV, 0, true);
        delay(2000);
        EclaireClavier();
        return true;
      }
    }
  }
}
