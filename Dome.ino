/* Pilotage automatique de l'abri du telescope
  # Serge CLAUS
  # GPL V3
  # Version 2.6
  # 22/10/2018-19/10/2019
*/

//---------------------------------------PERIPHERIQUES-----------------------------------------------
#include <SoftwareSerial.h> // Port série 2 pour le module IHM (LCD,clavier...)
SoftwareSerial Ser2(2, 3); // RX, TX
//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties
#define ALIMPC 9   // (R3) Alimentation 220V sous le télescope (PC Indi /Raspi) contact NF
#define ALIMTEL 8   // (R4) Alimentation télescope
#define ALIMMOT 10   // (R2) Alimentation 220V moteur abri
#define MOTEUR  11   // (R1) Ouverture/fermeture abri
#define P11     7   // (R5) Relais 1 porte 1
#define P12     6   // (R6) Relais 2 porte 1
#define P21     5   // (R7) Relais 1 porte 2
#define P22     4   // (R8) Relais 2 porte 2
#define RESET   13   // Reset
#define BARU    12  // Bouton arret d'urgence
#define BMA     A7  // Bouton M/A

// Entrées
#define PARK  A6	// Etat du telescope 0: non parqué, 1: parqué
/* TODO
   entrée ouverture portes
*/
#define AO A0        // Capteur abri ouvert
#define AF A1       // Capteur abri fermé
#define Po1 A2       // Capteur portes ouvertes
#define Po2 A4      // Capteur portes fermées
#define Pf1 A3	    // BARU Bouton arret d'urgence
#define Pf2 A5      // BMA Bouton marche/arret

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIPORTESCAPTEUR  30000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIMOTEUR 40000L  // Durée d'initialisation du moteur
#define DELAIABRI   15000L  // Durée de déplacement de l'abri (25000L)
#define MOTOFF HIGH         // Etat pour l'arret du moteur
#define MOTON !MOTOFF

//---------------------------------------Variables globales------------------------------------

#define AlimStatus  (!digitalRead(ALIMTEL))    // Etat de l'alimentation télescope
#define PortesOuvert (!digitalRead(Po1) && !digitalRead(Po2))
#define PortesFerme (!digitalRead(Pf1) && !digitalRead(Pf2))
#define AbriFerme (!digitalRead(AF))
#define AbriOuvert (!digitalRead(AO))
#define MoteurStatus (!digitalRead(ALIMMOT))
#define StartTel digitalWrite(ALIMTEL, LOW)
#define StopTel digitalWrite(ALIMTEL, HIGH)
#define StartMot digitalWrite(ALIMMOT, MOTON)
#define StopMot digitalWrite(ALIMMOT, MOTOFF)
#define StopPC  digitalWrite(ALIMPC, LOW)
#define StartPC digitalWrite(ALIMPC, HIGH)
#define TelPark analogRead(PARK)>300
#define	BoutonMA analogRead(BMA)>300

//#define TelPark 1

bool Manuel = false;

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial.begin(9600);
  Ser2.begin(9600);

  // Initialisation des relais
  digitalWrite(ALIMPC, HIGH);
  pinMode(ALIMPC, OUTPUT);
  digitalWrite(ALIMTEL, HIGH); pinMode(ALIMTEL, OUTPUT);
  digitalWrite(ALIMMOT, HIGH); pinMode(ALIMMOT, OUTPUT);
  digitalWrite(MOTEUR, HIGH); pinMode(MOTEUR, OUTPUT);
  digitalWrite(P11, HIGH); pinMode(P11, OUTPUT);
  digitalWrite(P12, HIGH); pinMode(P12, OUTPUT);
  digitalWrite(P21, HIGH); pinMode(P21, OUTPUT);
  digitalWrite(P22, HIGH); pinMode(P22, OUTPUT);

  digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Activation des entrées (capteurs...)
  pinMode(AO, INPUT_PULLUP);
  pinMode(AF, INPUT_PULLUP);
  pinMode(Po1, INPUT_PULLUP);
  pinMode(Pf1, INPUT_PULLUP);
  pinMode(Po2, INPUT_PULLUP);
  pinMode(Pf2, INPUT_PULLUP);
  pinMode(BARU, INPUT_PULLUP);
  pinMode(BMA, INPUT);

  digitalWrite(RESET, HIGH);
  pinMode(RESET, OUTPUT);

  pinMode(PARK, INPUT);

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
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg = "";		// Message reçu sur le port série

void loop() {
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
      else if (SerMsg == "AU") {
        Serial.println("0");
        ARU();
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
    else if (SerMsg == "C?") {
      Serial.print(AbriFerme);
      Serial.print(AbriOuvert);
      Serial.print(PortesFerme);
      Serial.print(PortesOuvert);
      Serial.print(AlimStatus);
      Serial.println(TelPark ? "p" : "n");
      Serial.print(digitalRead(Pf1));
      Serial.print(digitalRead(Pf2));
      Serial.print(digitalRead(Po1));
      Serial.println(digitalRead(Po2));
    }
  }

  // TEST DEPLACEMENT INOPINE DU DOME
  if (!Manuel && !AbriFerme && !AbriOuvert) {
    ARU();
  }

  // Bouton Arret d'urgence
  if (!digitalRead(BARU)) {
    ARU();
  }
  // Bouton Marche/Arret ? on ouvre la petite porte
  if (BoutonMA) {
    ouvrePorte1;
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

// Ferme la petite porte
void fermePorte1(void) {
  digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  digitalWrite(P11, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  digitalWrite(P12, LOW);
  delay(DELAIPORTES);
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
void changePortes(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && PortesOuvert) || (!etat && PortesFerme)) {
    return;
  }
  if (etat) {   // Ouverture des portes
    // Alimentation du moteur
    StartMot; // On allume assez tôt pour laisser le temps de s'initialiser
    // Ouverture des portes
    digitalWrite(P12, LOW);
    attendPorte(5000);
    digitalWrite(P22, LOW);
    attendPorte(DELAIPORTESCAPTEUR); // Délai minimum
    // On attend que les portes sont ouvertes
    while (!PortesOuvert) {
      attendPorte(100);
    }
    // Délai pour finir le mouvement
    attendPorte(5000);
    digitalWrite(P12, HIGH);
    digitalWrite(P22, HIGH);
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      return;
    }
    StopMot;
    digitalWrite(P21, LOW);
    attendPorte(5000);
    digitalWrite(P11, LOW);
    attendPorte(DELAIPORTES);
    digitalWrite(P11, HIGH);
    digitalWrite(P21, HIGH);
  }
}

void DeplaceDomeARU(void) {
  /* Déplace le dome sans vérification /!\ */
  digitalWrite(MOTEUR, LOW);
  delay(600);
  digitalWrite(MOTEUR, HIGH);
}

// Déplacement de l'abri 1: ouverture 0: fermeture
void deplaceAbri(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && AbriOuvert) || (!etat && AbriFerme)) {
    return;
  }
  // Test telescope parqué
  if (!TelPark) {
    return;
  }
  StopTel; // Coupure alimentation télescope
  if (!PortesOuvert) {
    if (!MoteurStatus) StartMot; // Alimentation du moteur
    changePortes(true);    //Ouverture des portes
  }
  else if (!MoteurStatus) {
    // Attente d'initialisation du moteur de l'abri
    StartMot;
    //Attente pour l'initialisation du moteur
    attendPorte(DELAIMOTEUR); // Protection contre les déplacements intempestifs
  }
  // Deplacement de l'abri
  digitalWrite(MOTEUR, LOW);
  delay(600);
  digitalWrite(MOTEUR, HIGH);
  attendDep(DELAIABRI);
  while (!AbriFerme && !AbriOuvert) {
    attendDep(1000);
  }
  attendDep(2000);		   // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat = AbriOuvert;
  if (etat) {
    // Abri ouvert
    StartTel; // Alimentation télescope
  }
  else {
    // Abri fermé
    StopTel; // Coupure alimentation télescope
    delay(500);
    changePortes(false);             // Fermeture des portes
    // Pas nécessaire (déjà fait à la fermeture des portes)
    StopMot; // Coupure alimentation moteur abri
    StopTel; // Coupure alimentation dome
  }
}

// Boucle d'attente lors du déplacement
void attendDep(unsigned long delai) {	// Boucle d'attente pendant le déplacement de l'abri
  int ERRMAX = 2;
  int nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    SerMsg = LireCmd();
    if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) ARU();
    // Bouton Arret d'urgence
    //if digitalRead(BARU) {ARU();}
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

void attendPorte(unsigned long delai) {	// Boucle d'attente pendant l'ouverture/fermeture des portes
  int ERRMAX = 2;
  int nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    SerMsg = LireCmd();
    if (SerMsg == "AU") {
      Serial.println("0");
      ARU();
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++;
    if (nbpark >= ERRMAX) ARU();
    // Si le dome se déplace pendant le mouvement des portes: ARU
    if (!AbriFerme && !AbriOuvert) {
      ARU();
    }
    // Bouton Arret d'urgence
    //if digitalRead(BARU) {ARU();}
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

// Commande d'arret d'urgence
void ARU() {				// Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  digitalWrite(ALIMTEL, HIGH);
  digitalWrite(ALIMMOT, HIGH);
  digitalWrite(MOTEUR, HIGH);
  digitalWrite(P11, HIGH);
  digitalWrite(P12, HIGH);
  digitalWrite(P21, HIGH);
  digitalWrite(P22, HIGH);
  digitalWrite(ALIMMOT, MOTOFF);
  // Ouverture des portes
  changePortes(true);
  digitalWrite(RESET, LOW);
}
