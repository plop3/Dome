Matériel:
--------
- 1 TTGO ESP32 LoRa
- 1 Carte 8 relais 5V
- 1 Alimentation 220V/12V 15A
- 1 Convertisseur 12V/5V 3A
- 8 capteurs inductifs 12V (portes, position abri)
- 1 Convertisseur I2c 4 voies 5V <-> 3.3V
- 1 LED
- 1/2 LEDs APA106 (Park,...)
- 3 Barrettes de LEDs APA106 (éclairage table, abri, extérieur). 


Alimentation:
-------------
  - 220V	pour le moteur de l'abri.
  - 12V  	Alimentation 220V/12V	Vérins électriques, télescope (Goto, focuser, caméra).
  - 5V	Convertisseur 12V/5V	ESP32, éclairages LEDs APA106.

Entrées/sorties:
----------------

MCP23017:
---------
Utilisation d'un MCP23017 pour la gestion des sorties relais et des entrées capteurs.
Sorties:
--------
  -0:	Ouverture/fermeture abri
  -1:	Alimentation 220V moteur/abri
  -2:	(Alimentation 12V) N/A
  -3:	Alimentation 12V télescope
  -4:	Relais 1 porte 1
  -5:	Relais 2 porte 1
  -6:	Relais 1 porte 2
  -7:	Relais 2 porte 2

Entrées:
--------
   -8:	Capteur abri fermé
   -9:	Capteur abri ouvert
  -10:	Porte 2 fermée
  -11:	Porte 1 ouverte
  -12:   Porte 2 fermée
  -13:	Porte 2 ouverte
  -14:
  -15:
  
E/S ESP32:
----------
Sorties:
  -2: LED Park
  -4: LEDs APA106

Entrées:
  -13: Etat parqué du télescope.	

Convertisseur 5V/3.3V
---------------------
- Conversion I2c vers le MCP23017 alimenté en 5V.
- Conversion vers la LED park
- Conversion vers les LEDs APA106

