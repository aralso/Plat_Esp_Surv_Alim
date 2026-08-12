# Ard_ESP_Chaudiere_Garches :

Modification adresse IP : Message type 2 ou 4 (à envoyer sur port série 115200 bauds)
2-1:13              // 13 : mode wifi station
4-1:192.168.251.31  // adresse IP du module  (et .32 pour Sonde thermometre)
4-2:192.168.251.1   // adresse IP de la gateway
4-3:255.255.255.0   // subnet IP
4-4:8.8.8.8         // DNS primaire
4-5:8.8.4.4         // DNS secondaire
4-6:garches        // nom routeur pour wifi
4-7: xxx            // mdp routeur pour wifi
4-8:1               // websocket (1:off, 2:on)
4-9:ws://webcam.hd.free.fr:8081       // websocket IP (pas utilisé ici)
4-10:3              // Websocket ID
4-11: B0:CB:D8:E9:0C:74 (Dans sonde : adresse mac de l'esp chaudiere)
puis ARST0   // Reset

Sonde : 
   Appuyer sur le bouton => le module se connecte au wifi et reste allumé pendant 30 seconde
   chaque message uart envoyé, prolonge de 30 secondes la durée d'allumage
   chaque requete /get ou /set sur la page web prolonge de 30s la durée d'allumage

Autres registres (type2) : 
                  1 : mode reseau (11-12:AP, 13:routeur, 14:filaire)
                  2 : Nb_reset : lecture seule
                  3 : reset : écrire 13 pour faire reset
                  4 : Periode cycle en minutes
                  5 : cycle rapide - écrire 12 pour augmenter vitesse cycle d'un facteur 60
                  6 : log detail : 0:rien 4:max (pas utilisé)
                  7 : délai écoute websocket
                  8 : Skip graph : 1 valeur sur X : si periode_cycle=15min et Reg8=4 => 1 temp chaque heure sur les graphiques
                  9 : Seuil batt sonde en volts : ex 3.2
                  10: Freq Log batterie(jours) : ex 2 jours
                  11 : consigne économie : ex 10°C
                  41 : canal wifi : dernier canal wifi utilisé
                  42 : canal wifi prérentiel (thermom)

Periode cycle : 
rapide : 2-5:12

Programme ota : C:\Users\<toi>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.x\tools\espota.py


