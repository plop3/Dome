/* Pilotage automatique de l'abri du telescope
  # Serge CLAUS
  # GPL V3
  # Version 2.4
  # 22/10/2018-20/06/2019
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

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes
#define DELAIMOTEUR 40000L  // Durée d'initialisation du moteur
#define DELAIABRI   25000L  // Durée de déplacement de l'abri
#define MOTOFF HIGH          // Etat pour l'arret du moteur
#define MOTON !MOTOFF

//---------------------------------------Variables globales------------------------------------

#define AlimStatus  !digitalRead(ALIM24V)    // Etat de l'alimentation télescope
#define DomeStatus	!digitalRead(ALIM12V)
#define PortesOuvert !digitalRead(PO) 
#define AbriFerme !digitalRead(AF) 
#define AbriOuvert !digitalRead(AO)
#define MoteurStatus !digitalRead(MOTEUR)
#define StartDome digitalWrite(ALIM12V, LOW);delay(3000)
#define StopDome digitalWrite(ALIM12V, HIGH)
#define StartTel digitalWrite(ALIM24V, LOW)
#define StopTel digitalWrite(ALIM24V, HIGH)
#define StartMot digitalWrite(ALIMMOT, !MOTOFF)
#define StopMot digitalWrite(ALIMMOT, MOTOFF)
//#define TelPark digitalRead(PARK)
#define TelPark 1

//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial2.begin(9600);
  // Initialisation des relais
  pinMode(LEDPARK, OUTPUT);
  pinMode(LUMIERE, OUTPUT);
  digitalWrite(ALIM12V,HIGH);
  pinMode(ALIM12V, OUTPUT);
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


  // Mise en marche de l'alimentation 12V
  StartDome;
  
  // TODO Tant qu'on n'a pas des contacts portes fermées
  //PortesFerme = !PortesOuvert;
  
  // Etat du dome initialisation des interrupteurs
  if ( AbriOuvert) {
    StartTel; // Alimentation télescope
  }
  if (AbriFerme) {
	  StopDome;	// L'abri est fermé: alimentation coupée
  }
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg="";		// Message reçu sur le port série

void loop() {
  // Lecture des ordres reçus du port série2 (ESP8266)
  if (Serial2.available()) {
    SerMsg=Serial2.readStringUntil(35);
	  if (SerMsg == "P+") {
      changePortes(true);
      Serial2.println((PortesOuvert && DomeStatus) ? "1" : "0");
	  }
    else if (SerMsg == "P-") {
      changePortes(false);
      Serial2.println((!PortesOuvert || DomeStatus) ? "1" : "0");
    }
    else if (SerMsg == "D+") {
      deplaceAbri(true);
	          Serial2.println((AbriOuvert && DomeStatus) ? "1" : "0");
    }
    else if (SerMsg == "D-") {
      deplaceAbri(false);
	          Serial2.println((AbriFerme || !DomeStatus)? "1" : "0");
    }
    else if (SerMsg == "A+") {
	    StartTel;
	    Serial2.println("1");
    }
    else if ( SerMsg == "A-") {
	    StopTel;
	    Serial2.println("1");
    }
	else if ( SerMsg == "ON") {
		StartDome;
	}
	else if ( SerMsg == "OF") {
		StopDome;
	}
    else if (SerMsg == "P?") {
      Serial2.println((PortesOuvert && DomeStatus) ? "1" : "0");
    }
    else if (SerMsg == "D?") {
            Serial2.println((AbriFerme || !DomeStatus) ? "0" : "1");
    }
    else if (SerMsg == "A?") {
            Serial2.println(AlimStatus ? "1" : "0");
    }
    else if (SerMsg == "AU") {
	    Serial2.println("0");
            ARU();
    }
    else if (SerMsg == "p-") {
	fermePorte1();
	Serial2.println("0");
    }	  
    else if (SerMsg == "p+") {
	ouvrePorte1();
	Serial2.println("0");
    }	  
    else if (SerMsg == "C?") {
      Serial2.print(AbriFerme);
      Serial2.print(!AbriFerme);
      //Serial2.print(PortesFerme);
      Serial2.print(!PortesOuvert);
      Serial2.print(PortesOuvert);
      Serial2.print(AlimStatus);
	  Serial2.print(DomeStatus);
      Serial2.println(TelPark ? "p" : "n");
    }
  }
  digitalWrite(LEDPARK, TelPark);
    
  // TEST DEPLACEMENT INOPINE DU DOME
  if (!AbriFerme && !AbriOuvert && DomeStatus) {ARU();};
  
  // Bouton Arret d'urgence
  //if digitalRead(BARU) {ARU();}
  
}

//---------------------------------------FONCTIONS--------------------------------------------

// Ferme la petite porte
void fermePorte1(void) {
  bool etat12V=DomeStatus;
  if (!etat12V) {StartDome;}
  digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  digitalWrite(P11, HIGH);
  if (!etat12V) {StopDome;}
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  bool etat12V=DomeStatus;
  if (!etat12V) {StartDome;}
  digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  digitalWrite(P12, HIGH);
  if (!etat12V) {StopDome;}
}

// Change la position des portes 0: ouverture 1 fermeture
void changePortes(bool etat) {
  bool etat12V=DomeStatus;
  if (!etat12V) {StartDome;}
  // Commande identique à l'état actuel, on sort
  if ((etat && PortesOuvert) || (!etat && !PortesOuvert)) {
	// On remet l'alim dans l'état initial
	if (!etat12V) {StopDome;}
    return;
  }
  if (etat) {   // Ouverture des portes
	// Alimentation du moteur
	StartMot; // On allume assez tôt pour laisser le temps de s'initialiser
	// Ouverture des portes
    digitalWrite(P12, LOW);
    attendPorte(5000);
    digitalWrite(P22, LOW);
    attendPorte(DELAIPORTES); // Délai minimum
    // On attend que les portes sont ouvertes
    while (!PortesOuvert) {
      attendPorte(100);
    }
    //attendPorte(2000);
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
    StopDome;
  }
}

// Déplacement de l'abri 1: ouverture 0: fermeture
void deplaceAbri(bool etat) {
  bool etat12V=DomeStatus;
  if (!etat12V) {StartDome;}
  // Commande identique à l'état actuel, on sort
  if ((etat && AbriOuvert) || (!etat && AbriFerme)) { 
    if (!etat12V) {StopDome;}
    return;
  }
  // Test telescope parqué
   if (!TelPark) {
	   if (!etat12V) {StopDome;}
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
  while(!AbriFerme && !AbriOuvert) {	
    attendDep(1000);
  }
  attendDep(2000);		   // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat=AbriOuvert;
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
    if (Serial2.available()) {
    	SerMsg=Serial2.readStringUntil(35);
    	if (SerMsg == "AU") {
	    Serial2.println("0");
            ARU();
    	}
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++; // TODO capteur HS
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
    if (Serial2.available()) {
    	SerMsg=Serial2.readStringUntil(35);
    	if (SerMsg == "AU") {
	    Serial2.println("0");
            ARU();
    	}
    }
    // Si le telescope n'est plus parqué pendant le déplacement -> ARU
    if (!TelPark) nbpark++; // TODO capteur HS
    if (nbpark >= ERRMAX) ARU();
	// Si le dome se déplace pendant le mouvement des portes: ARU
	if (!AbriFerme && !AbriOuvert) {ARU();}
	// Bouton Arret d'urgence
    //if digitalRead(BARU) {ARU();}
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
