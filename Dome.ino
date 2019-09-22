/* Pilotage automatique de l'abri du telescope
  # Serge CLAUS
  # GPL V3
  # Version 3.2.1
  # 22/10/2018-21/09/2019
  # Version pour TTGO ESP32 LoRa + MCP23017
*/

//-------------------------------------FICHIERS EXTERNES---------------------------------------------
#include "Dome.h"

//---------------------------------------PERIPHERIQUES-----------------------------------------------
// MCP23017
#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;
Adafruit_MCP23017 mcpE;
// WiFi + OTA
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// NTP
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// TM1638 LEDs & Keys
#include <TM1638.h>
TM1638 module(4, 17, 25);

// Clavier matriciel I2c
#include <i2ckeypad.h>
#define ROWS 4
#define COLS 4
#define PCF8574_ADDR 0x26
i2ckeypad kpd = i2ckeypad(PCF8574_ADDR, ROWS, COLS);

// Serveur TCP
WiFiServer Server(23);

// Timer
#include <SimpleTimer.h>
SimpleTimer timer;

// MQTT
#include <PubSubClient.h>

// LEDs
#include <NeoPixelBus.h>
#define NBLEDS 3
#define PINRGB 13
NeoPixelBus<NeoGrbFeature, Neo400KbpsMethod> pixels(NBLEDS, PINRGB);
//Adafruit_NeoPixel pixels(NUMPIXELS, PINRGB, NEO_GRB + NEO_KHZ800);
#define colorSaturation 50
RgbColor green(colorSaturation, 0, 0);
RgbColor red(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// Reset ESP
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

//---------------------------------------CONSTANTES-----------------------------------------------

// Sorties
#define LEDPARK 2	// LED d'indication du park // TODO Remplacer par 1 led APA106 + éclairages intérieurs
#define LUMIERE 4  // Eclairage de l'abri TODO à modifier

// Sorties MCP23017
#define ALIM12V 2   // (R3) Alimentation 12V
#define ALIM24V 3   // (R4) Alimentation télescope
#define ALIMMOT 1   // (R2) Alimentation 220V moteur abri
#define MOTEUR  0   // (R1) Ouverture/fermeture abri
#define P11     4   // (R5) Relais 1 porte 1
#define P12     5   // (R6) Relais 2 porte 1
#define P21     6   // (R7) Relais 1 porte 2
#define P22     7   // (R8) Relais 2 porte 2

// Entrées
#define PARK  33	// Etat du telescope 0: non parqué, 1: parqué
/* TODO
   entrée ouverture portes
*/
#define AO 5        // Capteur abri ouvert
#define AF 7        // Capteur abri fermé
#define Po1 4       // Capteur porte 1 ouverte
#define Po2 0       // Capteur porte 2 ouverte
#define Pf1 6	    //  Capteur porte 1 fermée (BARU)
#define Pf2 2       // Capteur porte 2 fermée (BMA)

// Constantes globales
#define DELAIPORTES 40000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIPORTESCAPTEUR  30000L  // Durée d'ouverture/fermeture des portes (40000L)
#define DELAIMOTEUR 40000L  // Durée d'initialisation du moteur
#define DELAIABRI   15000L  // Durée de déplacement de l'abri (25000L)
#define MOTOFF HIGH         // Etat pour l'arret du moteur
#define MOTON !MOTOFF

#define LEDSTATUS 0
#define LEDCLAVIER 1


#define TPSVEILLE 30  //Delai avant la mise en veille du clavier (s)
const char *topicin = "domoticz/in";
const char *topicout = "domoticz/out";



//---------------------------------------Variables globales------------------------------------
#define AlimStatus  (!mcp.digitalRead(ALIM24V))    // Etat de l'alimentation télescope
#define PortesOuvert (!mcpE.digitalRead(Po1) && !mcpE.digitalRead(Po2))
#define PortesFerme (!mcpE.digitalRead(Pf1) && !mcpE.digitalRead(Pf2))
#define AbriFerme (!mcpE.digitalRead(AF))
#define AbriOuvert (!mcpE.digitalRead(AO))
#define MoteurStatus (!mcp.digitalRead(ALIMMOT))
#define StartTel mcp.digitalWrite(ALIM24V, LOW)
#define StopTel mcp.digitalWrite(ALIM24V, HIGH)
#define StartMot mcp.digitalWrite(ALIMMOT, MOTON)
#define StopMot mcp.digitalWrite(ALIMMOT, MOTOFF)
//#define TelPark digitalRead(PARK)
#define TelPark 1
#define NiveauAff 0

bool Lock = true; // Dome locké
bool Veille = false; // Clavier matriciel en veille
String SECRET = "1234";
String formattedDate;
String dayStamp;
String timeStamp;
int TypeAff = 1; // 0: Eteint, 1: Heure GMT, 2: T° /H%
bool StateAff = true;
int timerLED;

bool LastPark = false;
//---------------------------------------SETUP-----------------------------------------------

void setup() {
  Serial.begin(9600);
  mcpE.begin();	// Entrées capteurs
  mcp.begin(4);     // Sorties relais
  // Initialisation des relais
  pinMode(LEDPARK, OUTPUT);
  pinMode(LUMIERE, OUTPUT);
  mcp.digitalWrite(ALIM12V, HIGH);
  mcp.pinMode(ALIM12V, OUTPUT);
  mcp.digitalWrite(ALIM24V, HIGH); mcp.pinMode(ALIM24V, OUTPUT);
  mcp.digitalWrite(ALIMMOT, HIGH); mcp.pinMode(ALIMMOT, OUTPUT);
  mcp.digitalWrite(MOTEUR, HIGH); mcp.pinMode(MOTEUR, OUTPUT);
  mcp.digitalWrite(P11, HIGH); mcp.pinMode(P11, OUTPUT);
  mcp.digitalWrite(P12, HIGH); mcp.pinMode(P12, OUTPUT);
  mcp.digitalWrite(P21, HIGH); mcp.pinMode(P21, OUTPUT);
  mcp.digitalWrite(P22, HIGH); mcp.pinMode(P22, OUTPUT);

  mcp.digitalWrite(ALIMMOT, MOTOFF); // Coupure alimentation moteur abri
  // Activation des entrées (capteurs...)
  mcpE.pinMode(AO, INPUT); mcpE.pullUp(AO, HIGH);
  mcpE.pinMode(AF, INPUT); mcpE.pullUp(AF, HIGH);
  mcpE.pinMode(Po1, INPUT); mcpE.pullUp(Po1, HIGH);
  mcpE.pinMode(Pf1, INPUT); mcpE.pullUp(Pf1, HIGH);
  mcpE.pinMode(Po2, INPUT); mcpE.pullUp(Po2, HIGH);
  mcpE.pinMode(Pf2, INPUT); mcpE.pullUp(Pf2, HIGH);
  //pinMode(BARU, INPUT);
  //pinMode(BMA, INPUT);

  //pinMode(PARK, INPUT_PULLUP);
  pinMode(PARK, INPUT);
  pinMode(LEDPARK, OUTPUT);
  digitalWrite(LEDPARK, TelPark);

  // Etat du dome initialisation des interrupteurs
  if ( AbriOuvert) {
    StartTel; // Alimentation télescope
    StartMot; // Alimentation du moteur de l'abri
  }

  // Afficheur TM1638
  module.setupDisplay(1, 0);
  module.setDisplayToString("Start");

  // LEDs RGB
  pixels.Begin();
  pixels.Show();
  pixels.SetPixelColor(LEDCLAVIER, black);
  pixels.SetPixelColor(LEDSTATUS, red);
  pixels.Show();

  // Clavier matriciel
  kpd.init();

  // Connexion WiFi
  WiFi.mode(WIFI_AP_STA);
  //WiFi.hostname("dome");
  //access point part
  Serial.println("Creating Accesspoint");
  WiFi.softAP(assid, asecret, 7, 0, 5);
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());
  //station part
  IPAddress local_IP(192, 168, 0, 16);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(212, 27, 40, 240);
  IPAddress secondaryDNS(212, 27, 40, 241);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  Serial.print("connecting to...");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    attempts++;
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //MDNS.begin("dome");


  // OTA
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("dome2");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Serveur TCP
  Server.begin();
  module.setDisplayToString("GO   ");
  module.setLED(TM1638_COLOR_RED, 0);
  delay(1000);
  module.setDisplayToString("     ");

  // NTP
  timeClient.begin();
  //timeClient.setTimeOffset(3600); On reste en GMT

  // Timer
  timer.setInterval(1000, FuncSec);

  // Eteint l'afficheur si les portes sont fermées
  if (mcpE.digitalRead(Po1)) {
    //module.setupDisplay(0, 0);
  }
  pixels.SetPixelColor(LEDSTATUS, black);
  pixels.Show();
  delay(1000);
}

//---------------------------------------BOUCLE PRINCIPALE------------------------------------

String SerMsg = "";		// Message reçu sur le port série

void loop() {
  timer.run();
  ArduinoOTA.handle();
  // MQTT
  //WiFiClient mqttClient;
  //PubSubClient mqtt(mqttClient);

  // Lecture des ordres reçus du port série2 (ESP8266)
  WiFiClient client = Server.available();
  if (client) {
    SerMsg = client.readStringUntil(35);
    if (SerMsg == "P+") {
      changePortes(true);
      client.println((PortesOuvert) ? "1" : "0");
    }
    else if (SerMsg == "P-") {
      changePortes(false);
      client.println((!PortesOuvert) ? "1" : "0");
    }
    else if (SerMsg == "D+") {
      deplaceAbri(true);
      client.println((AbriOuvert) ? "1" : "0");
    }
    else if (SerMsg == "D-") {
      deplaceAbri(false);
      client.println((AbriFerme) ? "1" : "0");
    }
    else if (SerMsg == "A+") {
      StartTel;
      client.println("1");
    }
    else if ( SerMsg == "A-") {
      StopTel;
      client.println("1");
    }
    else if (SerMsg == "P?") {
      client.println((PortesOuvert) ? "1" : "0");
    }
    else if (SerMsg == "D?") {
      client.println((AbriFerme) ? "0" : "1");
    }
    else if (SerMsg == "A?") {
      client.println(AlimStatus ? "1" : "0");
    }
    else if (SerMsg == "AU") {
      client.println("0");
      ARU();
    }
    else if (SerMsg == "p-") {
      fermePorte1();
      client.println("0");
    }
    else if (SerMsg == "p+") {
      ouvrePorte1();
      client.println("0");
    }
    else if (SerMsg == "C?") {
      client.print(AbriFerme);
      client.print(AbriOuvert);
      client.print(PortesFerme);
      client.print(PortesOuvert);
      client.print(AlimStatus);
      client.println(TelPark ? "p" : "n");
      client.print(mcpE.digitalRead(Pf1));
      client.print(mcpE.digitalRead(Pf2));
      client.print(mcpE.digitalRead(Po1));
      client.println(mcpE.digitalRead(Po2));
    }
    client.stop();
  }

  // Lecture des boutons TM1638
  byte keys = module.getButtons();
  switch (keys) {
    case 1: // Bascule d'affichage
      TypeAff += 1;
      if (TypeAff > 2) {
        TypeAff = 1;
      }
      delay(500);
      break;
    case 2: // Arret/marche de l'affichage
      StateAff = !StateAff;
      module.setupDisplay(StateAff, NiveauAff);
      delay(500);
    case 128:
      deplaceAbri(false);
      break;
  }
  // Lecture du clavier matriciel
  char key = kpd.get_key();
  if (key != '\0') {
    if (key == 42) {
      // Active l'éclairage du clavier
      EclaireClavier();
    }
    else {
      // Clavier locké, lecture du code de déverrouillage
      if (Lock) {
        // TODO Allume la LED du clavier en mode éclairage
        if (ClavierCode(key)) {
          Lock = false;
        }
      }
      else {
        Serial.print(key);
        Serial.print(" ");
        Serial.println(char(key));
        // Clavier débloqué: commandes autorisées
        switch (char(key)) {
          case 65: // A
            deplaceAbri(true);
            break;
          case 66: // B
            deplaceAbri(false);
            break;
          case 67:  // C
            changePortes(true);
            break;
          case 68:  // D
            changePortes(false);
            break;
          case 49:  // 1
            ouvrePorte1();
            break;
          case 50:  // 2
            fermePorte1();
            break;
          case 35:  // #  TODO Verrouillage du dome
            module.setLED(0, 7);
            Lock = true;
            break;
        }
      }
    }
  }

  // LEDs
  if ((TelPark != LastPark) && AbriOuvert) {
    LastPark = !LastPark;
    if (TelPark) {
      pixels.SetPixelColor(LEDSTATUS, green);
    }
    else {
      pixels.SetPixelColor(LEDSTATUS, black);
    }
    //digitalWrite(LEDPARK, TelPark);
    pixels.Show();
  }
  // TEST DEPLACEMENT INOPINE DU DOME
  if (!AbriFerme && !AbriOuvert) {
    ARU();
  }
  // Bouton Arret d'urgence
  //if digitalRead(BARU) {ARU();}
}

//---------------------------------------FONCTIONS--------------------------------------------

// Fonction executée toutes les secondes
void FuncSec() {
  switch (TypeAff) {
    case 0: // Pas d'affichage
      module.setDisplayToString("        ");
      break;
    case 1: // Affichage de l'heure
      if (WiFi.status() != WL_CONNECTED) {
        module.setDisplayToString("--------");
      }
      else {
        while (!timeClient.update()) {
          timeClient.forceUpdate();
        }
        formattedDate = timeClient.getFormattedTime();
        module.setDisplayToString(formattedDate);
        break;
      case 2: // T° /H%
        module.setDisplayToString("T° / H%");
      }
  }
  //pixels.Show();

}

// Eclairage du clavier
void EclaireClavier() {
  // Allume la LED
  module.setLED(TM1638_COLOR_RED, 4);
  pixels.SetPixelColor(LEDCLAVIER, white);
  pixels.Show();
  // Demarre le temporisateur
  if (!Veille) {
    timerLED = timer.setTimeout(TPSVEILLE * 1000, EteintClavier);
    Veille = true;
  }
  // Eteint la LED du clavier

}
void EteintClavier() {
  // Eteint l'éclaraige du clavier
  pixels.SetPixelColor(LEDCLAVIER, black);
  pixels.Show();
  module.setLED(0, 4);
  Veille = false;
}

// Déverrouillage au clavier
bool ClavierCode(char key) {
  String code = String(key);
  unsigned long previousMillis = millis();
  int delai = 10; // Delai pour rentrer le code en secondes
  module.setDisplayToString("Code    ");
  // TODO Allumage de la LED en orange pour éclairer le clavier
  pixels.SetPixelColor(LEDCLAVIER, white);
  pixels.Show();
  module.setLED(TM1638_COLOR_RED, 7);
  while (true) {
    unsigned long currentMillis = millis();
    // Attente d'une touche ou de la fin du délai
    if (currentMillis - previousMillis >= (delai * 1000)) {
      Serial.println("Mauvais code");
      // TODO Eteint la LED d'éclairage
      module.setLED(0, 7);
      pixels.SetPixelColor(LEDCLAVIER, black);
      pixels.Show();
      module.setDisplayToString("        ");
      return false;
    }
    key = kpd.get_key();
    if (key != '\0') {
      // Touche pressée
      code = code + key;
      int lg = 0;
      if (code.length() > 4) {
        lg = code.length() - 4;
      }
      code = code.substring(lg);
      Serial.println(code);
      if (code == SECRET) {
        Serial.println("Code OK");
        // TODO Allume la LED en vert pendant 2s
        module.setLED(TM1638_COLOR_RED, 6);
        module.setDisplayToString("Accept  ");
        pixels.SetPixelColor(LEDCLAVIER, green);
        pixels.Show();
        delay(2000);
        pixels.SetPixelColor(LEDCLAVIER, black);
        pixels.Show();
        module.setLED(0, 6);
        module.setDisplayToString("        ");
        return true;
      }
    }
  }
}

// Ferme la petite porte
void fermePorte1(void) {
  module.setDisplayToString("P1 F... ");
  mcp.digitalWrite(P11, LOW);
  delay(DELAIPORTES);
  module.setDisplayToString("P1 CLOSE"); delay(500);
  module.setupDisplay(0, NiveauAff);
  mcp.digitalWrite(P11, HIGH);
}

// Ouvre la petite porte
void ouvrePorte1(void) {
  module.setupDisplay(1, NiveauAff);
  module.setDisplayToString("P1 O... ");
  mcp.digitalWrite(P12, LOW);
  delay(DELAIPORTES);
  module.setDisplayToString("P1 OPEN "); delay(500);
  mcp.digitalWrite(P12, HIGH);
}

// Change la position des portes 0: ouverture 1 fermeture
void changePortes(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && PortesOuvert) || (!etat && PortesFerme)) {
    module.setDisplayToString("Error   "); delay(500);
    return;
  }
  if (etat) {   // Ouverture des portes
    // Alimentation du moteur
    StartMot; // On allume assez tôt pour laisser le temps de s'initialiser
    // Ouverture des portes
    module.setupDisplay(1, NiveauAff);
    module.setDisplayToString("P1 O... ");
    mcp.digitalWrite(P12, LOW);
    attendARU(5000, false, false);
    mcp.digitalWrite(P22, LOW);
    module.setDisplayToString("P12 O...");
    attendARU(DELAIPORTESCAPTEUR, false, false); // Délai minimum
    // On attend que les portes soient ouvertes
    while (!PortesOuvert) {
      attendARU(100, false, false);
    }
    // Délai pour finir le mouvement
    attendARU(5000, false, false);
    mcp.digitalWrite(P12, HIGH);
    mcp.digitalWrite(P22, HIGH);
    module.setDisplayToString("P12 OPEN"); delay(500);

  }
  else {    // Fermeture des portes
    //if ((AbriOuvert && AbriFerme) || (!AbriOuvert && ! AbriFerme)) {
    if (!AbriFerme) {
      module.setDisplayToString("Error   "); delay(500);
      return;
    }
    if (!TelPark) {
      // Lance la commande de park sur OnStep
      module.setDisplayToString("Error   "); delay(500);
      return;
    }
    StopMot;
    module.setDisplayToString("P2 F... ");
    mcp.digitalWrite(P21, LOW);

    attendARU(5000, true, true);

    module.setDisplayToString("P12 F...");
    mcp.digitalWrite(P11, LOW);
    attendARU(DELAIPORTES, true, true);
    mcp.digitalWrite(P11, HIGH);
    mcp.digitalWrite(P21, HIGH);

    module.setDisplayToString("P12 CLOS"); delay(500);
    Lock = true;
    // Eteint les affichages
    module.setupDisplay(0, NiveauAff);
    module.setLED(0, 7);
    for (int i = 0; i < NBLEDS; i++) {
      pixels.SetPixelColor(i, black);
    }
    pixels.Show();
  }
}

// Déplacement de l'abri 1: ouverture 0: fermeture
void deplaceAbri(bool etat) {
  // Commande identique à l'état actuel, on sort
  if ((etat && AbriOuvert) || (!etat && AbriFerme)) {
    module.setDisplayToString("Error   "); delay(500);
    return;
  }
  // Test telescope parqué
  if (!TelPark) {
    // Lance la commande de park sur OnStep
    module.setDisplayToString("Error   "); delay(500);
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
    attendARU(DELAIMOTEUR, true, false); // Protection contre les déplacements intempestifs
  }
  // Deplacement de l'abri
  module.setDisplayToString("Abri d..");
  mcp.digitalWrite(MOTEUR, LOW);
  delay(600);
  mcp.digitalWrite(MOTEUR, HIGH);
  attendARU(DELAIABRI, true, false);
  while (!AbriFerme && !AbriOuvert) {
    attendARU(1000, true, false);
  }
  attendARU(2000, true, false);		 // Finir le déplacement
  // Etat réel de l'abri au cas ou le déplacement soit inversé
  etat = AbriOuvert;
  if (etat) {
    // Abri ouvert
    module.setDisplayToString("Abr OPEN");
    StartTel; // Alimentation télescope
    delay(500);
  }
  else {
    // Abri fermé
    StopTel; // Coupure alimentation télescope
    delay(500);
    changePortes(false);             // Fermeture des portes
    // Pas nécessaire (déjà fait à la fermeture des portes)
    StopMot; // Coupure alimentation moteur abri
    StopTel; // Coupure alimentation dome
    module.setDisplayToString("Abr CLOS");
    delay(500);
  }
}

void attendARU(unsigned long delai, bool park, bool depl) {	// Boucle d'attente pendant l'ouverture/fermeture des portes
  int ERRMAX = 2;
  int nbpark = 0;
  unsigned long Cprevious = millis();
  while ((millis() - Cprevious) < delai) {
    // Lecture des ordres reçus du port série
    WiFiClient client = Server.available();
    if (client) {
      SerMsg = client.readStringUntil(35);
      if (SerMsg == "AU") {
        client.println("0");
        ARU();
      }
      client.stop();
    }
    if (park) {
      // Si le telescope n'est plus parqué pendant le déplacement -> ARU
      if (!TelPark) nbpark++;
      if (nbpark >= ERRMAX) ARU();
      // Si le dome se déplace pendant le mouvement des portes: ARU
    }
    if (depl) {
      if (!AbriFerme && !AbriOuvert) {
        ARU();
      }
    }
    // Bouton Arret d'urgence
    //if digitalRead(BARU) {ARU();}
    delay(100);    // Sinon ça plante (delay(1) marche aussi)...
  }
}

void hard_restart() {
  // Reset de l'ESP
  esp_task_wdt_init(1, true);
  esp_task_wdt_add(NULL);
  while (true);
}

// Commande d'arret d'urgence
void ARU() {				// Arret d'urgence
  // Arret de l'alimentation de l'abri
  // Initialisation des relais
  mcp.digitalWrite(ALIM12V, HIGH);
  mcp.digitalWrite(ALIM24V, HIGH);
  mcp.digitalWrite(ALIMMOT, HIGH);
  mcp.digitalWrite(MOTEUR, HIGH);
  mcp.digitalWrite(P11, HIGH);
  mcp.digitalWrite(P12, HIGH);
  mcp.digitalWrite(P21, HIGH);
  mcp.digitalWrite(P22, HIGH);
  mcp.digitalWrite(ALIMMOT, MOTOFF);
  // Ouverture des portes
  changePortes(true);
  module.setDisplayToString("Aru Aru ");
  // Attente d'un retour a la normale (abri ouvert ou fermé et télescope parqué)
  while ((!AbriFerme && !AbriOuvert) || (AbriFerme && AbriOuvert) || !TelPark) {
    delay(1000);
  }
  module.setDisplayToString("reboot..");
  delay(1000);
  hard_restart();
}
