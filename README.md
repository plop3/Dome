# Dome
Gestion de l'abri automatisé du télescope

L'abri est une petite cabane sur rails.<br>
L'ouverture/fermeture des portes est assurée par 2 vérins électriques. <br>
L'ouverture/fermeture de l'abri est commandée par une motorisation de porte de garage.

L'ensemble est piloté par: <br>
  1 carte 8 relais <br>
  1 TTGO ESP32 + LoRa<br>
  1 MCP23017 (Sorties relais, entrées capteurs)

Les principales commandes (liaison série 9600,8,N,1) <br>
  P+# Ouvre les portes <br>
  P-# Ferme les portes <br>
  D+# Ouvre l'abri (et les portes si besoin) <br>
  D-# Ferme l'abri et les portes <br>
  A+# Mise en marche de l'alimentation 12V télescope<br>
  A-# Arret de l'alimentation 12V télescope<br>
  p+# Ouverture de la porte 1 <br>
  p-# Fermeture de la porte 1 <br>
  C?# Informations de l'état des capteurs <br>
  
Commande manuelle en mode ARU:<br>
	P1#	Fermeture de la porte 1<br>
	P2# Fermeture de la porte 2<br>
	DD#	Deplacement de l'abri<br>
	TP#	Parquer le télescope<br>