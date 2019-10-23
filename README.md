# Dome
Gestion de l'abri automatisé du télescope

L'abri est une petite cabane sur rails.<br>
L'ouverture/fermeture des portes est assurée par 2 vérins électriques. <br>
L'ouverture/fermeture de l'abri est commandée par une motorisation de porte de garage.

- Ouverture/fermeture du dome par clavier codé ou à distance.
- Gestion des éclairages du dome.
- Park du télescope et fermeture du dome en cas de pluie et au matin.
- Alerte par SMS en cas de problème.
- Affichage de l'heure et des informations météo.

### MATERIEL
- Abri en bois roulant sur rails.
- 2 vérins électriques pour l'ouverture/fermeture des portes.
- 1 moteur de porte de garage pour le déplacement de l'abri.
- 1 accès réseau par WiFi.
- 1 carte "dome":
	- Arduino Nano.
	- Wemos + ESP-Link.
	- MCP23017 pour les entrées capteurs.
- 1 carte LoRa:
	ESP32 TTGO LoRa
- 1 carte 8 relais.
- 1 détecteur de pluie.
- 2 claviers matriciels 4x4 + PCF8574.
- 1 afficheur LCD 20x4
- 4 barrettes de 8 LEDs Neopixel.
- 3 LEDs Neopixel.
- 1 alimentation 220V/12V 15A
- 1 régulateur 12V -> 5V 3A

Les principales commandes (liaison série 57600,8,N,1) <br>
  P+# Ouvre les portes <br>
  P-# Ferme les portes <br>
  D+# Ouvre l'abri (et les portes si besoin) <br>
  D-# Ferme l'abri et les portes <br>
  A+# Mise en marche de l'alimentation 12V <br>
  A-# Arret de l'alimentation 12V <br>
  p+# Ouverture de la porte 1 <br>
  p-# Fermeture de la porte 1 <br>
  C?# Informations de l'état des capteurs <br>
