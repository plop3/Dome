# Dome
Gestion de l'abri automatisé du télescope

L'abri est une petite cabane sur rails.<br>
L'ouverture/fermeture des portes est assurée par 2 vérins électriques. <br>
L'ouverture/fermeture de l'abri est commandée par une motorisation de porte de garage.

L'ensemble est piloté par: <br>
  1 carte 8 relais <br>
  1 Arduino Mega + Wemos/ESPLink

Les principales commandes (liaison série 9600,8,N,1) <br>
  P+# Ouvre les portes <br>
  P-# Ferme les portes <br>
  D+# Ouvre l'abri (et les portes si besoin) <br>
  D-# Ferme l'abri et les portes <br>
  A+# Mise en marche de l'alimentation 12V <br>
  A-# Arret de l'alimentation 12V <br>
  p+# Ouverture de la porte 1 <br>
  p-# Fermeture de la porte 1 <br>
  C?# Informations de l'état des capteurs <br>
