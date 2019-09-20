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
- 1 Afficheur + boutons Led & Keys TM1638
- 1 Module PCF8574
- 1 Clavier 4x4


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
  - A07:	Capteur abri fermé
  - A05:	Capteur abri ouvert
  - A06:	Porte 1 fermée
  - A04:	Porte 1 ouverte
  - A02: Porte 2 fermée
  - A00:	Porte 2 ouverte
  - B00: Bouton 1
  - B01: Bouton 2
  - B02: Bouton 3
  - B03; Bouton 4
  
E/S ESP32:
----------
Sorties:
  -  4: LEDs APA106
  - 13: Rétro-éclairage afficheur LCD

Entrées:
  - 36: Etat parqué du télescope.	

Communication:
  - 25: TM1638  (DIO)
  - 17: TM1638  (CLK)
  -  4: TM1638  (STB)
  - 22: I2c (SCL)
  - 21: I2c (SDA)

Convertisseur 5V/3.3V
---------------------
- Conversion I2c vers le MCP23017 alimenté en 5V.
- Conversion vers la LED park
- Conversion vers les LEDs APA106

Périphériques I2c:
-----------------
  - 0x20	MCP23017	entrées capteurs
  - 0x24	MCP23017	sorties relais
  - 0x26	PCF8574	  Clavier 4x4 (jumper 1 ON, 2 ON, 3 OFF)
  - 0x27	LCD 20x4	Afficheur LCD/I2c
