# Dome
Gestion de l'abri automatisé du télescope

L'abri est une petite cabane sur rails.
L'ouverture/fermeture des portes est assurée par 2 vérins électriques.
L'ouverture/fermeture de l'abri est commandée par une motorisation de porte de garage.

L'ensemble est piloté par:
    1 carte 8 relais
    1 Wemos mini D1 pro + MCP23017 (E/S)

Les principales commandes (liaison série 9600,8,N,1)
P+#     Ouvre les portes
P-#     Ferme les portes
D+#     Ouvre l'abri (et les portes si besoin)
D-#     Ferme l'abri et les portes
A+#     Mise en marche de l'alimentation 12V
A-#     Arret de l'alimentation 12V

