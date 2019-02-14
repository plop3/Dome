/* Pilotage automatique de l'abri du telescope
  # Serge CLAUS
  # GPL V3
  # Version 2.2
  # 22/10/2018-214/02/2019
*/

//---------------------------------------PERIPHERIQUES-----------------------------------------------

//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties
#define LEDPARK 13	// LED d'indication du park // TODO Remplacer par 1 led APA106 + éclairages intérieurs
#define LUMIERE 10  // Eclairage de l'abri
#define ALIM12V 27   // (R3) Alimentation 12V
#define ALIM24V 29   // (R4) Alimentation télescope
#define ALIMMOT 25   // (R2) Alimentation 220V moteur abri
#define MOTEUR  23   // (R1) Ouverture/fermeture abri
#define P11     31   // (R5) Relais 1 porte 1
#define P12     33   // (R6) Relais 2 porte 1
#define P21     35   // (R7) Relais 1 porte 2
#define P22     37   // (R8) Relais 2 porte 2
#define RESET   9    // Reset

// Entrées
#define PARK  2	// Etat du telescope 0: non parqué, 1: parqué
/* TODO 
 * entrée ouverture portes
 */
#define AO 4        // Capteur abri ouvert
#define AF 3        // Capteur abri fermé
#define PO 5       // Capteur portes ouvertes
#define PF 6       // Capteur portes fermées
#define BARU  7     // Bouton arret d'urgence
#define BMA   8     // Bouton marche/arret
/* TODO
 *  Ecran LCD I2c
 */

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes
#define DELAIABRI   25000L  // Durée de déplacement de l'abri
#define MOTOFF HIGH          // Etat pour l'arret du moteur

//---------------------------------------Variables globales------------------------------------

#define PortesOuvert !digitalRead(PO)
#define AbriFerme digitalRead(AF)
//#define AbriOuvert digitalRead(AO)
#define TelPark digitalRead(PARK)
//#define TelPark 1
#define AlimStatus  !digitalRead(ALIM12V)    // Etat de l'alimentation 12V

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial.begin(9600);
  // Initialisation des relais
  pinMode(LEDPARK, OUTPUT);
  pinMode(LUMIERE, OUTPUT);
  digitalWrite(ALIM12V,HIGH);pinMode(ALIM12V, OUTPUT);
  digitalWrite(ALIM24V,HIGH);pinMode(ALIM24V, OUTPUT);
  digitalWrite(ALIMMOT,HIGH);pinMode(ALIMMOT, OUTPUT);
  digitalWrite(MOTEUR,HIGH);pinMode(MOTEUR, OUTPUT);
  digitalWrite(P11,HIGH);pinMode(P11, OUTPUT);
  digitalWrite(P12,HIGH);pinMode(P12, OUTPUT);
  digitalWrite(P21,HIGH);pinMode(P21, OUTPUT);
  digitalWrite(P22,HIGH);pinMode(P22, OUTPUT);
  
  digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Activation des entrées (capteurs...)
  pinMode(AO, INPUT_PULLUP);
  pinMode(AF, INPUT_PULLUP);
  pinMode(PO, INPUT_PULLUP);
  pinMode(PF, INPUT_PULLUP);
  pinMode(BARU, INPUT_PULLUP);
  pinMode(BMA, INPUT_PULLUP);

  digitalWrite(RESET, HIGH);
  pinMode(RESET, OUTPUT);
  
  //pinMode(PARK, INPUT_PULLUP);
  pinMode(PARK, INPUT);
  pinMode(LEDPARK, OUTPUT);
  digitalWrite(LEDPARK, TelPark);

  // Etat du dome initialisation des interrupteurs
  if (PortesOuvert || !AbriFerme) {	// TODO remplacer par AbriOuvert quand le capteur sera changé
    digitalWrite(ALIM12V, LOW);
  }
  //if ( !AbriFerme) {	// TODO remplacer par AbriOuvert
    digitalWrite(ALIM24V, LOW); // Alimentation télescope
  //}
  // TODO Tant qu'on n'a pas les contacts portes fermées et abri fermé
  //PortesFerme = !PortesOuvert;
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg="";		// Message reçu sur le port série

void loop() {
  // Lecture des ordres reçus du port série
  if (Serial.available()) {
    SerMsg=Serial.readStringUntil(35);
	  if (SerMsg == "P+") {
      changePortes(true);
      Serial.println(PortesOuvert ? "1" : "0");
	  }
    else if (SerMsg == "P-") {
      changePortes(false);
      Serial.println(!PortesOuvert ? "1" : "0");
    }
    else if (SerMsg == "D+") {
      deplaceAbri(true);
	          Serial.println(!AbriFerme ? "1" : "0");
    }
    else if (SerMsg == "D-") {
      deplaceAbri(false);
	          Serial.println(AbriFerme ? "1" : "0");
    }
    else if (SerMsg == "A+") {
	    digitalWrite(ALIM12V,LOW);
	    Serial.println("1");
    }
    else if ( SerMsg == "A-") {
	    digitalWrite(ALIM12V,HIGH);
	    Serial.println("1");
    }
    else if (SerMsg == "P?") {
      Serial.println(PortesOuvert ? "1" : "0");
    }
    else if (SerMsg == "D?") {
            Serial.println(AbriFerme ? "0" : "1");
    }
    else if (SerMsg == "A?") {
            Serial.println(AlimStatus ? "1" : "0");
    }
    else if (SerMsg == "AU") {
	    Serial.println("0");
            ARU();
    }
    else if (SerMsg == "p-") {
	fermePorte1();
	Serial.println("0");
    }	  
    else if (SerMsg == "p+") {
	ouvrePorte1();
	Serial.println("0");
    }	  
    else if (SerMsg == "C?") {
      Serial.print(AbriFerme);
      Serial.print(!AbriFerme);
      //Serial.print(PortesFerme);
      Serial.print(!PortesOuvert);
      Serial.print(PortesOuvert);
      Serial.print(AlimStatus);
      Serial.println(TelPark ? "p" : "n");
    }
  }
  digitalWrite(LEDPARK, TelPark);
}

//---------------------------------------FONCTIONS--------------------------------------------

// Ferme la petite porte
void fermePorte1(void) {
  if (!AlimStatus) {
    // Mise en marche de l'alimentation 12V
    digitalWrite(ALIM12V, LOW);
    delay(3000);
  }	
  digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  digitalWrite(P11, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  if (!AlimStatus) {
    // Mise en marche de l'alimentation 12V
    digitalWrite(ALIM12V, LOW);
    delay(3000);
  }	
  digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  digitalWrite(P12, HIGH);
}


// Change la position des portes 0: ouverture 1 fermeture
void changePortes(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && PortesOuvert) || (!etat && !PortesOuvert)) {
    return;
  }
  if (!AlimStatus) {
    // Mise en marche de l'alimentation 12V
    digitalWrite(ALIM12V, LOW);
    delay(3000);
  }
  if (etat) {   // Ouverture des portes
    digitalWrite(P12, LOW);
    delay(5000);
    digitalWrite(P22, LOW);
    delay(DELAIPORTES);
    /* TODO attente changement capteurs
    while (!PortesOuvert) {
      delay(10);
    }
    */	
    delay(5000);
    digitalWrite(P12, HIGH);
    digitalWrite(P22, HIGH);
  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      return;
    }
    digitalWrite(P21, LOW);
    delay(5000);
    digitalWrite(P11, LOW);
    attendDep(DELAIPORTES);
    digitalWrite(P11, HIGH);
    digitalWrite(P21, HIGH);
    if (AbriFerme) {
      // Coupure de l'alimentation 12V
      digitalWrite(ALIM12V, HIGH);
    }
  }
}

// Déplacement de l'abri 1: ouverture 0: fermeture
void deplaceAbri(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && !AbriFerme) || (!etat && AbriFerme)) { // TODO Patch contacteur abri ouvert problématique
    return;
  }
  digitalWrite(ALIMMOT, !MOTOFF); // Alimentation du moteur
  // Pas d'alimentation 12V ?
  if (!AlimStatus) {
    digitalWrite(ALIM12V, false);
    // Boucle d'attente télescope parqué
    for (int i = 0; i < 15; i++) {
      if (TelPark) break;
      delay(500);
    }
  }
  // Test telescope parqué
   if (!TelPark) {
    return;
  }
  if (!PortesOuvert) {
    changePortes(true);    //Ouverture des portes
  }
  else {
    delay(30000); 	// A ajuster
    // Mini impulsion pour activer le moteur ???
  }
  
  digitalWrite(ALIM24V, HIGH); // Coupure alimentation télescope
  // Deplacement de l'abri
  digitalWrite(MOTEUR, LOW);
  delay(600);
  digitalWrite(MOTEUR, HIGH);
  attendDep(DELAIABRI);
  /*
    while(!AbriFerme) {	
      attendDep(1000);
    }
    */
  attendDep(5000);		   // Finir le déplacement
  digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat=!AbriFerme;
  if (etat) {
    // Abri ouvert
    digitalWrite(ALIM24V, LOW); // Alimentation télescope
  }
  else {
    // Abri fermé
    digitalWrite(ALIM24V, HIGH); // Coupure alimentation télescope
    delay(500);
    changePortes(false);             // Fermeture des portes
    digitalWrite(ALIM12V, HIGH); // Coupure alimentation 12V
  }
}

// Boucle d'attente lors du déplacement
void attendDep(unsigned long delai) {	// Boucle d'attente pendant le déplacement de l'abri
  int ERRMAX = 2;
  int nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    if (Serial.available()) {
    	SerMsg=Serial.readStringUntil(35);
    	if (SerMsg == "AU") {
	    Serial.println("0");
            ARU();
    	}
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++; // TODO capteur HS
    if (nbpark >= ERRMAX) ARU();
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

// Commande d'arret d'urgence
void ARU() {				// Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  digitalWrite(ALIM12V,HIGH);
  digitalWrite(ALIM24V,HIGH);
  digitalWrite(ALIMMOT,HIGH);
  digitalWrite(MOTEUR,HIGH);
  digitalWrite(P11,HIGH);
  digitalWrite(P12,HIGH);
  digitalWrite(P21,HIGH);
  digitalWrite(P22,HIGH);
  digitalWrite(ALIMMOT, MOTOFF);
  // Ouverture des portes
  changePortes(true);
  digitalWrite(RESET, LOW);
}
