Matériel:
--------
- 1 TTGO ESP32 LoRa
- 1 Carte 8 relais 5V
- 2 MCP23017  (carte relais, entrée capteurs + boutons)
- 1 Alimentation 220V/12V 15A
- 1 Convertisseur 12V/5V 3A
- 8 capteurs inductifs 12V (portes, position abri)
- 1 Convertisseur I2c 4 voies 5V <-> 3.3V
- 2 LEDs APA106 (Status, LED arrière abri)
- 3 Barrettes de LEDs APA106 (éclairage table, abri, extérieur). 


Alimentation:
-------------
  - 220V	pour le moteur de l'abri.
  - 12V  	Alimentation 220V/12V	Vérins électriques, télescope (Goto, focuser, caméra).
  - 5V	Convertisseur 12V/5V	ESP32, éclairages LEDs APA106.

Entrées/sorties:
----------------

Utilisation de deux MCP23017 pour la gestion des sorties relais, des boutons et des entrées capteurs.

MCP23017(1):
---------
Sorties:
--------
  - 0:	Ouverture/fermeture abri
  - 1:	Alimentation 220V moteur/abri
  - 2:	(Alimentation 12V) N/A
  - 3:	Alimentation 12V télescope
  - 4:	Relais 1 porte 1
  - 5:	Relais 2 porte 1
  - 6:	Relais 1 porte 2
  - 7:	Relais 2 porte 2

MCP23017(2):
----------

Entrées:
--------
  - 01:	Capteur abri fermé
  - 02:	Capteur abri ouvert
  - 03:	Porte 2 fermée
  - 04:	Porte 1 ouverte
  - 05: Porte 2 fermée
  - 06:	Porte 2 ouverte
  - 07: Bouton 1
  - 08: Bouton 2
  - 09: Bouton 3
  - 10; Bouton 4
  
E/S ESP32:
----------
Sorties:
  - 2: LED Park
  - 4: LEDs APA106

Entrées:
  - 13: Etat parqué du télescope.	

Convertisseur 5V/3.3V
---------------------
- Conversion I2c vers le MCP23017 alimenté en 5V.
- Conversion vers la LED park
- Conversion vers les LEDs APA106

