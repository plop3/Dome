# 1 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino"
/* 
Pilotage automatique de l'abri du telescope
Serge CLAUS
GPL V3
Version 2.6
22/10/2018-19/10/2019
*/

//---------------------------------------PERIPHERIQUES-----------------------------------------------
# 11 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino" 2
SoftwareSerial Ser2(13, 2); // RX, TX

// MCP23017
# 15 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino" 2
# 16 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino" 2
Adafruit_MCP23017 mcp; //MCP externe connecté à la carte 8 relais

// Clavier matriciel I2c
# 20 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino" 2



i2ckeypad kpd = i2ckeypad(0x26, 4, 4);

// LCD I2c
# 27 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino" 2
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

// LEDs neopixel
# 31 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino" 2

# 33 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino" 2




# 36 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino"
Adafruit_NeoPixel pixels(3, 4, ((1<<6) | (1<<4) | (0<<2) | (2)) /*|< Transmit as G,R,B*/ + 0x0000 /*|< 800 KHz data transmission*/);

//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties
# 53 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino"
// Entrées
# 63 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino"
// Constantes globales
# 75 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino"
//---------------------------------------Variables globales------------------------------------
# 92 "/home/serge/Documents/Projets/Astro/Dome/Dome.ino"
//#define TelPark 1

bool Manuel = false; // Mode manuel
bool LastPark = false; // Dernier état de Park
bool StateAff = true; // Etat de l'affichage du TM1638 (ON/OFF)
int BKLEVEL = 20; // PWM LCD
int LEDLEVEL = 15; // Intensité des LEDs du coffret

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial.begin(57600);
  Ser2.begin(9600);
  // MCP23017
  mcp.begin();

  // Clavier matriciel
  kpd.init();

  // Initialisation des relais
  digitalWrite(10 /* (R3) Alimentation 220V sous le télescope (PC Indi /Raspi) contact NF*/, 0x1); pinMode(10 /* (R3) Alimentation 220V sous le télescope (PC Indi /Raspi) contact NF*/, 0x1);
  digitalWrite(9 /* (R4) Alimentation télescope*/, 0x1); pinMode(9 /* (R4) Alimentation télescope*/, 0x1);
  digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, 0x1); pinMode(11 /* (R2) Alimentation 220V moteur abri*/, 0x1);
  digitalWrite(12 /* (R1) Ouverture/fermeture abri*/, 0x1); pinMode(12 /* (R1) Ouverture/fermeture abri*/, 0x1);
  digitalWrite(8 /* (R5) Relais 1 porte 1*/, 0x1); pinMode(8 /* (R5) Relais 1 porte 1*/, 0x1);
  digitalWrite(7 /* (R6) Relais 2 porte 1*/, 0x1); pinMode(7 /* (R6) Relais 2 porte 1*/, 0x1);
  digitalWrite(6 /* (R7) Relais 1 porte 2*/, 0x1); pinMode(6 /* (R7) Relais 1 porte 2*/, 0x1);
  digitalWrite(5 /* (R8) Relais 2 porte 2*/, 0x1); pinMode(5 /* (R8) Relais 2 porte 2*/, 0x1);
  digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, 0x1 /* Etat pour l'arret du moteur*/); // Coupure alimentation moteur abri

  // Activation des entrées (capteurs...)
  mcp.pinMode(7 /* MCP  // Capteur abri ouvert*/, 0x0); mcp.pullUp(7 /* MCP  // Capteur abri ouvert*/, 0x1);
  mcp.pinMode(5 /* MCP	// Capteur abri fermé*/, 0x0); mcp.pullUp(5 /* MCP	// Capteur abri fermé*/, 0x1);
  mcp.pinMode(6 /* MCP	// Capteur portes ouvertes*/, 0x0); mcp.pullUp(6 /* MCP	// Capteur portes ouvertes*/, 0x1);
  mcp.pinMode(4 /* MCP	// Capteur portes fermées*/, 0x0); mcp.pullUp(4 /* MCP	// Capteur portes fermées*/, 0x1);
  mcp.pinMode(2 /* MCP	// */, 0x0); mcp.pullUp(2 /* MCP	// */, 0x1);
  mcp.pinMode(0 /* MCP	// */, 0x0); mcp.pullUp(0 /* MCP	// */, 0x1);
  mcp.pinMode(3 /* MCP  // Bouton arret d'urgence*/, 0x0); mcp.pullUp(3 /* MCP  // Bouton arret d'urgence*/, 0x1);
  mcp.pinMode(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/, 0x0); //mcp.pullUp(PARK, LOW);
  pinMode(A6 /* Bouton M/A*/, 0x0);
  pinMode(3 /* Backlight LCD*/, 0x1);
  analogWrite(3 /* Backlight LCD*/, BKLEVEL);

  // LCD
  lcd.init();
  if ((!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) {
    lcd.backlight();
  }
  msgInfo("Demarrage...   ");

  // LEDs
  pixels.begin();
  pixels.clear();
  if ((!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) {
    pixels.setPixelColor(0, pixels.Color(LEDLEVEL, 0, 0));
    pixels.setPixelColor(1, pixels.Color(0, LEDLEVEL, 0));
    //pixels.setPixelColor(2, pixels.Color(0, 0, LEDLEVEL));
  }
  else {
    LastPark = true; // Empêche la LED de park de s'allumer
  }
  pixels.show();

  // Vérification de la position du dome au démarrage
  if (!(!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/)) && !(!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/))) {
    // Position incorrecte on passe en mode manuel
    Manuel = true;
  }
  // Etat du dome initialisation des interrupteurs
  if ( (!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/))) {
    digitalWrite(9 /* (R4) Alimentation télescope*/, 0x0); // Alimentation télescope
    digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, !0x1 /* Etat pour l'arret du moteur*/); // Alimentation du moteur de l'abri
  }

  lcd.clear();
  msgInfo("Ok");
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg = ""; // Message reçu sur le port série

void loop() {
  // Lecture des boutons du clavier
  //byte keys = module.getButtons();
  char key = kpd.get_key();

  if (key != '\0') {
    tone(17 /*A3*/, 440, 100);
    // Tone désactive le pwm sur la sortie 3
    delay(200);
    analogWrite(3 /* Backlight LCD*/, BKLEVEL);
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
        Serial.println(((!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) ? "1" : "0");
      }
      else if (SerMsg == "P-") {
        changePortes(false);
        Serial.println((!(!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) ? "1" : "0");
      }
      else if (SerMsg == "D+") {
        deplaceAbri(true);
        Serial.println(((!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/))) ? "1" : "0");
      }
      else if (SerMsg == "D-") {
        deplaceAbri(false);
        Serial.println(((!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/))) ? "1" : "0");
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
        digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, !0x1 /* Etat pour l'arret du moteur*/);
      }
      else if (SerMsg == "m-") {
        digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, 0x1 /* Etat pour l'arret du moteur*/);
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
      digitalWrite(9 /* (R4) Alimentation télescope*/, 0x0);
      Serial.println("1");
    }
    else if ( SerMsg == "A-") {
      digitalWrite(9 /* (R4) Alimentation télescope*/, 0x1);
      Serial.println("1");
    }
    else if (SerMsg == "P?") {
      Serial.println(((!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) ? "1" : "0");
    }
    else if (SerMsg == "D?") {
      Serial.println(((!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/))) ? "0" : "1");
    }
    else if (SerMsg == "A?") {
      Serial.println((!digitalRead(9 /* (R4) Alimentation télescope*/)) /* Etat de l'alimentation télescope*/ ? "1" : "0");
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
      digitalWrite(10 /* (R3) Alimentation 220V sous le télescope (PC Indi /Raspi) contact NF*/, 0x0);
      delay(1000);
      digitalWrite(10 /* (R3) Alimentation 220V sous le télescope (PC Indi /Raspi) contact NF*/, 0x1);
    }
    else if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
    }
    else if (SerMsg == "M?") {
      Serial.println(Manuel ? "m" : "a");
    }
    else if (SerMsg == "C?") {
      Serial.print((!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/)));
      Serial.print((!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/)));
      Serial.print((!mcp.digitalRead(4 /* MCP	// Capteur portes fermées*/) && !mcp.digitalRead(0 /* MCP	// */)));
      Serial.print((!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */)));
      Serial.print((!digitalRead(9 /* (R4) Alimentation télescope*/)) /* Etat de l'alimentation télescope*/);
      Serial.print(mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/) ? "p" : "n");
      Serial.println(Manuel ? "m" : "a");
      Serial.print(mcp.digitalRead(4 /* MCP	// Capteur portes fermées*/));
      Serial.print(mcp.digitalRead(0 /* MCP	// */));
      Serial.print(mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/));
      Serial.println(mcp.digitalRead(2 /* MCP	// */));
    }
  }

  // LED état park

  if (LastPark != mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/)) {
    pixels.setPixelColor(1, pixels.Color( LEDLEVEL * mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/), 0, 0));
    pixels.show();
    LastPark = mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/);
  }

  // TEST DEPLACEMENT INOPINE DU DOME
  // TODO à décommenter quand installé

  if (!Manuel && !(!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/)) && !(!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/))) {
    ARU();
  }

  // Bouton Arret d'urgence
  if (!mcp.digitalRead(3 /* MCP  // Bouton arret d'urgence*/)) {
    ARU();
  }
  // Bouton Marche/Arret ? on ouvre la petite porte
  if ((analogRead(A6 /* Bouton M/A*/)<300)) {
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

void msgInfo(String texte) {
  texte += "                    ";
  texte = texte.substring(0, 15);
  lcd.setCursor(0, 1);
  lcd.print(texte);
}
// Ferme la petite porte
void fermePorte1(void) {
  msgInfo("P1 F...");
  digitalWrite(8 /* (R5) Relais 1 porte 1*/, 0x0);
  delay(40000L /* Durée d'ouverture/fermeture des portes (40000L)*/);
  msgInfo("P1 Close");
  //  module.setupDisplay(0, NiveauAff);
  digitalWrite(8 /* (R5) Relais 1 porte 1*/, 0x1);
}

// Ouvre la petite porte
void ouvrePorte1(void) {


  //module.setupDisplay(1, NiveauAff);
  msgInfo("P1 O...");

  digitalWrite(7 /* (R6) Relais 2 porte 1*/, 0x0);
  delay(40000L /* Durée d'ouverture/fermeture des portes (40000L)*/);
  msgInfo("P1 OPEN");
  digitalWrite(7 /* (R6) Relais 2 porte 1*/, 0x1);
}

// Ferme la porte2
void fermePorte2(void) {
  digitalWrite(6 /* (R7) Relais 1 porte 2*/, 0x0);
  delay(40000L /* Durée d'ouverture/fermeture des portes (40000L)*/);
  digitalWrite(6 /* (R7) Relais 1 porte 2*/, 0x1);
}

// Ouvre la porte2
void ouvrePorte2(void) {
  digitalWrite(5 /* (R8) Relais 2 porte 2*/, 0x0);
  delay(40000L /* Durée d'ouverture/fermeture des portes (40000L)*/);
  digitalWrite(5 /* (R8) Relais 2 porte 2*/, 0x1);
}

// Change la position des portes 0: ouverture 1 fermeture
bool changePortes(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && (!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) || (!etat && (!mcp.digitalRead(4 /* MCP	// Capteur portes fermées*/) && !mcp.digitalRead(0 /* MCP	// */)))) {
    msgInfo("Erreur position");

    return false;
  }
  if (etat) { // Ouverture des portes
    // Alimentation du moteur
    digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, !0x1 /* Etat pour l'arret du moteur*/); // On allume assez tôt pour laisser le temps de s'initialiser
    // Ouverture des portes
    //	 module.setupDisplay(1, NiveauAff);
    //	module.setDisplayToString("P1 O... ");
    pixels.setPixelColor(0, pixels.Color(LEDLEVEL, LEDLEVEL, 0));
    pixels.show();
    lcd.backlight();
    msgInfo("P1 O...");

    digitalWrite(7 /* (R6) Relais 2 porte 1*/, 0x0);
    if (!attendPorte(5000)) return false;
    digitalWrite(5 /* (R8) Relais 2 porte 2*/, 0x0);
    msgInfo("P12 O...");
    if (!attendPorte(30000L /* Durée d'ouverture/fermeture des portes (40000L)*/)) return false; // Délai minimum
    // On attend que les portes sont ouvertes
    while (!(!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) {
      if (!attendPorte(100)) return false;
    }
    // Délai pour finir le mouvement
    if (!attendPorte(5000)) return false;
    digitalWrite(7 /* (R6) Relais 2 porte 1*/, 0x1);
    digitalWrite(5 /* (R8) Relais 2 porte 2*/, 0x1);
    msgInfo("P12 Open");
    LastPark = !mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/); // Réactive la LED park
    pixels.setPixelColor(0, pixels.Color(LEDLEVEL, 0, 0));
    pixels.show();
  }
  else { // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!(!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/))) {

      msgInfo("Err POs");
      return false;
    }
    pixels.setPixelColor(0, pixels.Color(LEDLEVEL, LEDLEVEL, 0));
    pixels.show();
    digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, 0x1 /* Etat pour l'arret du moteur*/);
    msgInfo("P2 F...");
    digitalWrite(6 /* (R7) Relais 1 porte 2*/, 0x0);
    if (!attendPorte(5000)) return false;
    msgInfo("P12 F...");
    digitalWrite(8 /* (R5) Relais 1 porte 1*/, 0x0);
    if (!attendPorte(40000L /* Durée d'ouverture/fermeture des portes (40000L)*/)) return false;
    digitalWrite(8 /* (R5) Relais 1 porte 1*/, 0x1);
    digitalWrite(6 /* (R7) Relais 1 porte 2*/, 0x1);
    // Arret des éclairages
    lcd.noBacklight();
    pixels.clear();
    pixels.show();
  }
  return true;
}

void DeplaceDomeARU(void) {
  /* Déplace le dome sans vérification /!\ */
  digitalWrite(12 /* (R1) Ouverture/fermeture abri*/, 0x0);
  delay(600);
  digitalWrite(12 /* (R1) Ouverture/fermeture abri*/, 0x1);
}

// Déplacement de l'abri 1: ouverture 0: fermeture
bool deplaceAbri(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && (!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/))) || (!etat && (!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/)))) {
    msgInfo("Err POS");
    return false;
  }
  // Test telescope parqué
  if (!mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/)) {
    msgInfo("Err PArk");
    return false;
  }
  digitalWrite(9 /* (R4) Alimentation télescope*/, 0x1); // Coupure alimentation télescope
  pixels.setPixelColor(0, pixels.Color(LEDLEVEL, LEDLEVEL, 0));
  pixels.show();
  if (!(!mcp.digitalRead(6 /* MCP	// Capteur portes ouvertes*/) && !mcp.digitalRead(2 /* MCP	// */))) {
    if (!(!digitalRead(11 /* (R2) Alimentation 220V moteur abri*/))) digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, !0x1 /* Etat pour l'arret du moteur*/); // Alimentation du moteur
    changePortes(true); //Ouverture des portes
  }
  else if (!(!digitalRead(11 /* (R2) Alimentation 220V moteur abri*/))) {
    // Attente d'initialisation du moteur de l'abri
    digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, !0x1 /* Etat pour l'arret du moteur*/);
    //Attente pour l'initialisation du moteur
    if (!attendPorte(40000L /* Durée d'initialisation du moteur*/)) return false; // Protection contre les déplacements intempestifs
  }
  // Deplacement de l'abri
  msgInfo("Deplacement abri...");
  digitalWrite(12 /* (R1) Ouverture/fermeture abri*/, 0x0);
  delay(600);
  digitalWrite(12 /* (R1) Ouverture/fermeture abri*/, 0x1);
  if (!attendDep(15000L /* Durée de déplacement de l'abri (25000L)*/)) return false;
  while (!(!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/)) && !(!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/))) {
    if (!attendDep(1000)) return false;
  }
  if (!attendDep(2000)) return false; // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat = (!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/));
  if (etat) {
    // Abri ouvert
    msgInfo("Abri ouvert");
    digitalWrite(9 /* (R4) Alimentation télescope*/, 0x0); // Alimentation télescope
    pixels.setPixelColor(0, pixels.Color(LEDLEVEL, 0, 0));
    pixels.show();
  }
  else {
    // Abri fermé
    digitalWrite(9 /* (R4) Alimentation télescope*/, 0x1); // Coupure alimentation télescope
    msgInfo("Abri fermé");
    delay(500);
    changePortes(false); // Fermeture des portes
    // Pas nécessaire (déjà fait à la fermeture des portes)
    digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, 0x1 /* Etat pour l'arret du moteur*/); // Coupure alimentation moteur abri
    digitalWrite(9 /* (R4) Alimentation télescope*/, 0x1); // Coupure alimentation dome
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
    if (!mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/)) nbpark++;
    if (nbpark >= ERRMAX) {
      ARU();
      return false;
    }
    // Bouton Arret d'urgence
    if (!mcp.digitalRead(3 /* MCP  // Bouton arret d'urgence*/)) {
      ARU();
      return false;
    }
    delay(100); // Sinon ça plante (delay(1) marche aussi)...
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
    if (!mcp.digitalRead(1 /*MCP	// Etat du telescope 0: non parqué, 1: parqué*/)) nbpark++;
    if (nbpark >= ERRMAX) {
      ARU();
      return false;
    }
    // Si le dome se déplace pendant le mouvement des portes: ARU
    if (!(!mcp.digitalRead(5 /* MCP	// Capteur abri fermé*/)) && !(!mcp.digitalRead(7 /* MCP  // Capteur abri ouvert*/))) {
      ARU();
      return false;
    }
    // Bouton Arret d'urgence
    if (!mcp.digitalRead(3 /* MCP  // Bouton arret d'urgence*/)) {
      ARU();
      return false;
    }
    delay(100); // Sinon ça plante (delay(1) marche aussi)...
  }
  return true;
}

// Commande d'arret d'urgence
void ARU() { // Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  Serial.println("ARU");
  digitalWrite(9 /* (R4) Alimentation télescope*/, 0x1);
  digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, 0x1);
  digitalWrite(12 /* (R1) Ouverture/fermeture abri*/, 0x1);
  digitalWrite(8 /* (R5) Relais 1 porte 1*/, 0x1);
  digitalWrite(7 /* (R6) Relais 2 porte 1*/, 0x1);
  digitalWrite(6 /* (R7) Relais 1 porte 2*/, 0x1);
  digitalWrite(5 /* (R8) Relais 2 porte 2*/, 0x1);
  digitalWrite(11 /* (R2) Alimentation 220V moteur abri*/, 0x1 /* Etat pour l'arret du moteur*/);
  // Passage en mode manuel
  Manuel = true;
  msgInfo("ARU !");
  pixels.setPixelColor(0, pixels.Color(0, LEDLEVEL, 0));
  // Ouverture des portes
  //changePortes(true);
  // ouvrePorte1();

  // Attente tant que le bouton Arret d'urgence est appuyé
  while (!mcp.digitalRead(3 /* MCP  // Bouton arret d'urgence*/)) {
    delay(100);
  }
}
