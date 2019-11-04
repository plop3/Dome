//---------------------------------------FONCTIONS--------------------------------------------
void Led(int led, int R, int V, int B, bool refresh) {
  pixels.setPixelColor(led, pixels.Color(R, V, B));
  if (refresh) pixels.show();
}
void Eclaire(int barre, int R, int V, int B) {
  for (int i = 4 + 8 * barre; i < (12 + 8 * barre); i++) {
    pixels.setPixelColor(i, pixels.Color(R, V, B));
    pixels.show();
  }
}

void bip(int pin, int freq, int delai) {
  tone(pin, freq, delai);
  delay(200);
  analogWrite(BKLIGHT, BKLEVEL);
}
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

void msgInfo(String texte) {
  texte += "                    ";
  texte = texte.substring(0, 15);
  lcd.setCursor(0, 1);
  lcd.print(texte);
}
// Ferme la petite porte
void fermePorte1(void) {
  msgInfo("P1 F...");
  digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  msgInfo("P1 Close");
  //  module.setupDisplay(0, NiveauAff);
  digitalWrite(P11, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {


  //module.setupDisplay(1, NiveauAff);
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
    //   module.setupDisplay(1, NiveauAff);
    //  module.setDisplayToString("P1 O... ");
    Led(LedStatus, LEDLEVEL, LEDLEVEL, 0, true);
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
    LastPark = !TelPark;  // Réactive la LED park
    Led(LedStatus, LEDLEVEL, 0, 0, true);
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {

      msgInfo("Err POs");
      return false;
    }
    Led(LedStatus, LEDLEVEL, LEDLEVEL, 0, true);
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
    lcd.noBacklight();
    pixels.clear();
    pixels.show();
  }
  return true;
}

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
    return false;
  }
  StopTel; // Coupure alimentation télescope
  Led(LedStatus, LEDLEVEL, LEDLEVEL, 0, true);
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
  digitalWrite(MOTEUR, LOW);
  delay(600);
  digitalWrite(MOTEUR, HIGH);
  if (!attendDep(DELAIABRI)) return false;
  while (!AbriFerme && !AbriOuvert) {
    if (!attendDep(1000)) return false;
  }
  if (!attendDep(2000)) return false;      // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat = AbriOuvert;
  if (etat) {
    // Abri ouvert
    msgInfo("Abri ouvert");
    StartTel; // Alimentation télescope
    Led(LedStatus, LEDLEVEL, 0, 0, true);
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
  return true;
}

// Boucle d'attente lors du déplacement
bool attendDep(unsigned long delai) { // Boucle d'attente pendant le déplacement de l'abri
  int ERRMAX = 2;
  int nbpark = 0;
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

bool attendPorte(unsigned long delai) { // Boucle d'attente pendant l'ouverture/fermeture des portes
  int ERRMAX = 2;
  int nbpark = 0;
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
