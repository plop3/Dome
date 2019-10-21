## Documentation

### Les commandes:
#### Mode Auto:
	P+#	Ouvre les portes
	P-#	Ferme les portes
	D+#	Ouvre l'abri
	D-#	Ferme l'abri
	AU#	Arret d'urgence
	MA#	Mode manuel
	
#### Mode Manuel:
	OK#	Retour au mode automatique
	2+#	Ouvre la porte 2
	2-#	Ferme la porte 2
	dd#	Déplace l'abri
	m+#	Alimente le moteur de l'abri
	m-# Coupe le moteur de l'abri
	
#### Mode Manuel/Auto:
	A+#	Alimentation 12V du télescope ON
	A-#	Alimentation 12V du télescope OFF
	p+#	Ouvre la porte 1
	p-#	Ferme la porte 1
	P?#	Etat des portes (1 ouvert 0 fermé)
	D?#	Etat de l'abri (1 ouvert 0 fermé)
	A?#	Etat de l'alim télescope (1 ON 0 OFF)
	C?#	Etat des capteurs et du télescope (park)
	TP#	Parque le télescope (TODO)
	PC#	Démarre le PC (ON/OFF/ON de la prise 220V)
	M?# Mode actuel (m Manuel, a Auto)

### Alimentation:
- 220V:
	- Moteur de l'abri
	- Alimentation PC Astro
	- Prises
	- Caméra de surveillance
- 12V:
	- Vérins électriques.
	- Télescope (Goto, focuser, antibuée, caméra).
- 5V:
	- Carte "Dome"
	- ESP32
	- Eclairages LEDs.
	
### Entrées/Sorties:
#### Sorties relais:
	0:	D12	Déplacement de l'abri.
	1:	D11	Alimentation 220V moteur/abri.
	2:	D10 Alimentation du PC (contact NF)
	3:	D9	Alimentation 12V du télescope
	4:	D8	Relais 1 porte 1
	5:	D7	Relais 2 porte 1
	6:	D6	Relais 1 porte 2
	7:	D5	Relais 2 porte 2
#### Entrées:
##### Directes Arduino Nano:
	A6		Bouton M/A (résistance Pullup 10k).
	D4		LEDs Neopixel.
	D3		Backlight écran LCD.
	A3		??? TODO
#### E/S
	D2/D13		Port série 2 (connecté à l'ESP32)
	A4/A5		I2c (LCD, Clavier, MCP23017)
	A0,A1,A2	TM1638
#### MCP23017:
	0:	Pf2		Porte 2 fermée
	1:	PARK	Télescope parqué
	2:	Po2		Porte 2 ouverte
	3:	BARU	Bouton ARRET D'URGENCE
	4:	Pf1		Porte 1 fermée
	5:	AF		Abri fermé
	6:	Po1		Porte 1 ouverte
	7:	AO		Abri ouvert
	
#### Disponibles:
##### Nano:
	A7
##### MCP23017:
	8 à 15

### Périphériques I2c
	0x20	MCP23017
	0x26	PCF8574 Clavier 4x4 (jumpers 1 ON, 2 ON, 3 OFF)
	0x27	LCD 20x4 I2c

### LEDs Neopixel:
	0:		Etat du dome 
	1:		Etat du télescope
	2-9		Eclairage abri
	10-17	Eclairage table
	18-25	Eclairage extérieur
	26		Eclairage clavier
	27 		Eclairage arrière
	
### LEDs TM1638:
	0:	Dome pret.
	1:	Télescope parqué.
	3:
	4:
	5:
	6:
	7:	Dome locké.
	
### Boutons TM1638:
	0:	Bascule affichage
	1:	M/A affichage
	2:
	3:
	4:
	5:
	6:
	7:	Park télescope

### Affichage TM1638:
	Heure/T° extérieur	
	T° ext / Hum %
	T° miroir / Pt de rosée
	
### Affichage LCD:

### Boutons:
	0:
	1:
	2:
	3:
	4:
	5:
	6:
	7:
	
### LED1 Status:
	Vert:	Abri Ok
	
### LED2 Télescope:
	Vert:	Parqué
	Orange:	Tracking ON
	
### LED clavier:
	S'éclaire en rouge sur appui d'une touche.
	Clignote à chaque appui d'une touche
	Passe en vert si dome débloqué (se rebloque automatiquemnt au bout de quelques minutes)
	S'eteint quand le dome est re-vérouillé ou quand le dome est ouvert.
	
### Clavier matriciel:
- Appui sur une touche, allume l'éclairage du clavier.
- Entrer un code pour déverrouiller l'abri.

- A:	Ouvre l'abri
- B:	Ferme l'abri (valider par #)
- C:	Ouvre les portes
- D: 	Ferme les portes
- 123	Eclairages (abri, table, extérieur) en rouge.
- 456	Eclairages -> eteint
- 789	Eclairages en blanc
- 0		Coupe tous les éclairages
- #
- *
	
