#include "../include/variables.h"

// Definition of the PARAMS array (kept in a single translation unit to avoid multiple definition linker errors)
Param PARAMS[] = {
  {"reseau", 1, U8,  11, 14, 11, 0, nullptr, &mode_reseau, 0},
  {"nb_reset", 2, U16, 0, 65535, 0, 0, nullptr, &nb_reset, 0},

  // cycle and network-related registers mapped to SetReg order numbers
  {"cycle", 4, U16, 10, 120, 15, 0, nullptr, &periode_cycle, 0},    // registre 4 : période du cycle (min)
  {"Rap", 5, U8, 0, 255, 0, 0, nullptr, &mode_rapide, 0},           // registre 5 : cycle rapide
  {"LogD", 6, U8, 0, 5, 0, 0, nullptr, &log_detail, 0},             // registre 6 : détail logs
  {"DelWS", 7, U8,  1, 30, 1, 0, nullptr, &DelaiWebsocket, 0},      // registre 7 : délai écoute websocket (s)
  {"Skip", 8, U8, 1, 50, 2,0, nullptr, &skip_graph, 0},             // registre 8 : skip graph

  // Parameters from requete_SetReg_appli
  {"SeBa", 9, U16, 1800, 4500, 3000, 0, nullptr, &Seuil_batt_sonde, 0},  // registre 9 : seuil batterie sonde (mV)
  {"FrBL", 10, U8, 1, 15, 7, 0, nullptr, &Nb_jours_Batt_log, 0},         // registre 10 : nb jours log batterie
  {"Allu", 15, U8, 0, 1, 0, 0, nullptr, &pas_de_veille, 0},   // 0:veille 1:pas de mise en veille
  {"PVei", 16, U16, 15, 600, 30, 0, nullptr, &prolong_veille, 0}, 
          // registre 16 : duree allumage (s)

  // Application settings
  {"AcSt", 17, U8, 0, 1, 0, 0, nullptr, &action_stockage, 0},        // action stockage
  {"AcEn", 18, U8, 0, 1, 0, 0, nullptr, &action_envoi, 0},           // action envoi



  {"latitude", 34, STR, 0, 0, 0, 0, "48.8461", &latitude, 16},
  {"longitude", 35, STR, 0, 0, 0, 0, "2.3469", &longitude, 16},


  // WiFi channel (SetReg_appli uses 41/42)
  {"lWc", 41, U8, 0, 13, 0, 0, nullptr, &last_wifi_channel, 0},         // registre 41 : last_wifi_channel (not persisted)
  {"WifiC", 42, U8, 1, 13, 1, 0, nullptr, &WIFI_CHANNEL, 0},         // registre 42 : canal wifi preferentiel (persisted)

  // IP addresses stored as IPAddress objects
  {"ipAdd", 50, IP, 0, 0xFFFFFFFFu, 0, 0, nullptr, &local_ip, 0},
  {"ipGat", 51, IP, 0, 0xFFFFFFFFu, 0, 0, nullptr, &gateway, 0},
  {"ipSub", 52, IP, 0, 0xFFFFFFFFu, 0, 0, nullptr, &subnet, 0},       // 255.255.255.0
  {"ipDNS", 53, IP, 0, 0xFFFFFFFFu, 0, 0, nullptr, &primaryDNS, 0},   // 8.8.8.8
  {"ipDNS2", 54, IP, 0, 0xFFFFFFFFu, 0, 0, nullptr, &secondaryDNS, 0},// 8.8.4.4
  {"Rout", 55, STR, 0, 0, 0, 0,  "rout", nom_routeur, 16},                // nom routeur  
  {"Mdp", 56, STR, 0, 0, 0, 0, "mdp", mdp_routeur, 16},                
  {"WSOn", 57, U8, 0, 2, 1, 0, nullptr, &websocket_on, 0},            // 0 ou 1
  {"WSock", 58, STR, 0, 0, 0, 0, "websocket", ip_websocket, 40},              // websocket adresse
  {"WSId", 59, U8, 0, 9, 9, 0,nullptr, &id_websocket, 0},             // 1, 2, 3

  // Surveillance Box/Deco (registres 70-80)
  {"SuEn", 70, U8,  0, 1,   0, 0, nullptr, &surv_en,                0},
  {"SuIB", 71, STR, 0, 0,   0, 0, "192.168.247.1", surv_ip_box,    20},
  {"SuID", 72, STR, 0, 0,   0, 0, "192.168.253.1", surv_ip_deco,   20},
  {"SuPo", 73, U16, 70, 1500, 80, 0, nullptr, &surv_port,           0},
  {"SuTn", 74, U16, 30, 3600, 120, 0, nullptr, &surv_intervalle_normal,   0},
  {"SuTf", 75, U16, 10, 300,  30, 0, nullptr, &surv_intervalle_confirm,   0},
  {"SuBx", 76, U16, 30, 600, 300, 0, nullptr, &surv_boot_box,       0},
  {"SuBd", 77, U16, 30, 600, 150, 0, nullptr, &surv_boot_deco,      0},
  {"SuCf", 78, U8,  1, 10,   2, 0, nullptr, &surv_nb_confirm,       0},
  {"SuRB", 79, U8,  0, 39,   0, 0, nullptr, &surv_pin_relay_box,    0},
  {"SuRD", 80, U8,  0, 39,   0, 0, nullptr, &surv_pin_relay_deco,   0},

};

// Provide number of entries for other translation units
const size_t PARAMS_COUNT = sizeof(PARAMS) / sizeof(PARAMS[0]);