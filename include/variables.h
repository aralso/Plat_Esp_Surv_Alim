#ifndef VARIABLES_H

#define VARIABLES_H


// variables externes


#define ESP_VEILLE     // Rôle principal 
//#define ESP_VEILLE  // Rôle distant : envoi data

// Hardware
//#define MODE_WT32  // WT32-Eth01 sinon ESP32-CAM ou DOIT ESP32 Devkit V1

//#define DEBUG  // mode station, pas de websocket, pas de sécurite, emulation valeurs STM32
//#define ESP32_v1    // DOIT ESP32 DEVKIt V1

#ifdef ESP_VEILLE
  // en veille 11uA
  // activation pendant 0,4 secondes à 40mA, chaque 15 minutes => 17uA en moyenne
  // si bouton => activation pendant 30 secondes à 120mA (correspnd à 2 jours de veille)
  #define GRAPH2  1 // 0:HUMID_ABS  1:HUMID_REL  2:Capteur PIR
  #define GRAPH3  0
  //#define PIR_ACTIF
  
  //#define ESP32_Fire2
  //#define ESP32_uPesy
  #define ESP32_S3

  //#define Temp_int_HDC1080  // Capteur I2C HDC1080
  #define MODE_Wifi  // Wifi sinon Ethernet
  //#define Sans_securite
  #define Sans_websocket
  #define OTA
  #define ENVOI
  #define STOCKAGE
#endif

#ifdef ESP_TJ_ACTIF  
  #define ESP32_S3
  #define OTA
  //#define ESP32_Fire2
  #define MODE_Wifi  // Wifi sinon Ethernet
  #define Sans_websocket
  //#define Sans_securite
  //#define WatchDog
#endif


//#define Temp_int_DHT22
//#define Temp_int_DS18B20
#define Temp_int_HDC1080

// Réseau
//#define NO_RESEAU

//#define Wifi_AP    // AP sinon STA

//#define STM32  //incompatible du modbus, sauf à changer les pin
// #define OTA

#define LATITUDE "48.8461"  // Garches => pour récupération Temp Ext
#define LONGITUDE "2.1889"

// Definir le canal WIFI ici (doit correspondre au routeur pour la gateway)
// ⚠️ IMPORTANT : Ce canal DOIT correspondre au canal de votre routeur WiFi
// "garches" Pour le trouver : regardez les logs de la gateway au démarrage

// Adresse par défaut de la GW (B0:CB:D8:E9:0C:74)
//const uint8_t MAC_GW[] = {0x08, 0xA6, 0xF7, 0x1C, 0xD0, 0x90}; //B0, 0xCB, 0xD8, 0xE9, 0x0C, 0x74};



typedef struct {
    uint16_t temp;
    uint16_t ha;
    uint16_t hr;
} ValeurTHR;

#define MAX_PAYLOAD 200
#define MAX_TEMP 30   // taille max autorisée pour l'envoi des temp/humid
#define ADDRESS 'B'
#define SERVER_ADD 'H'

typedef struct __attribute__((packed)) {   // packed permet d'éviter les octets de padding ajoutés par le compilateur
    uint8_t destinataire;
    uint8_t emetteur;
    uint8_t longueur;
    uint8_t code;
    uint8_t code2;
    uint8_t payload[MAX_PAYLOAD];
} Message_EspNow;

template<typename T>
void payloadWrite(uint8_t* payload, uint8_t& pos, const T& value)
{
    memcpy(&payload[pos], &value, sizeof(T));
    pos += sizeof(T);
}

//  -------  CONFIGURATION DES PINS
//  -----------------------------------------------

/* PINOUT DOIT :
0: pour programmation
1: TX pour prog & debug
2: OUT2 : sortie PAC
3: RX pour prog & debug
4: OUT1 : sortie Pompe
5(pin4:markIO35): OUT3 : sortie Arret
12: Btn_reveil
14: SDA Eclairs
15: DSB1820 capteur temp piscine
17: SCL Eclairs
21-22 : SDA, SCL HDC1080
32: IN:capteur DHT22 N°2
33: IN:capteur DHT22 N°1
35:(pin17) (in only) IN : interruption éclairs
36: (in only) IN : capteur pression
39: (in only) IN
*/
/*#define BTN1 14  // Defaut secteur (pullup)
#define BTN2 12  // intrusion    (pullup)
#define BTN3 14  // autoprotection    (pullup)
#define BTN4 15  // marche/Arret    (no pull)  0V:arret 12V:marche
#define BNT5 16  // Reset pour Accesspoint*/

#ifdef MODE_WT32  // WT32_Eth01
// const int PIN_Tint = 11;   // GPIO IN1 Temp interieure DS18B20
const int PIN_Tint22 = 5;  // GPIO IN1 Temp interieure DHT22
const int PIN_Text = 36;   //  Text:Entrée analogique 32 à 36 et 39
#else                      // ESP32_DevKit
  // const int PIN_Tint = 13;  Défini dans le fichier appli.ino
  const int PIN_Tint22 = 5;  // GPIO IN1 Temp interieure DHT22

  #ifdef ESP32_S3
    #define PIN_REVEIL 12  // Pin de réveil (Bouton externe)
    #define PIN_Vbatt 10        // Pin Surveillance Batterie (LiPo/2)
    #define PIN_REVEIL2 11  // Pin d'entrée pour interruption (ex: detecteur)
    #define PIN_OUT0 4     // Power pour alimenter le capteur PIR : 10uA
    #define PIN_SDA 8
    #define PIN_SCL 9
  #endif

  // Pin Reveil
  #ifdef ESP32_v1
    #define PIN_REVEIL 12  // Pin de réveil (Bouton externe)
    #define PIN_Vbatt 0        // Pin Surveillance Batterie (LiPo/2)
    #define PIN_SDA 21
    #define PIN_SCL 22
  #endif
  #ifdef ESP32_Fire2    // Firebeetle
    #define PIN_REVEIL 4  // Pin de réveil (Bouton externe) PIN RTC : 0 à 7
    #define PIN_Vbatt 0        // Pin Surveillance Batterie (LiPo/2)
    #define PIN_SDA 19
    #define PIN_SCL 20
  #endif
  #ifdef ESP32_uPesy
    // Nota : BTN : 14 et 25
    #define PIN_Vbatt 35    // Pin Surveillance Batterie (LiPo/)
    #define PIN_OUT0 4     // Power pour alimenter le capteur PIR : 10uA
    #define PIN_REVEIL 25   // Pin de réveil (Bouton externe)
    #define PIN_REVEIL2 33  // Pin d'entrée pour interruption (ex: detecteur PIR)
    #define PIN_SDA 21
    #define PIN_SCL 22
  #endif

  // ESP32-C6 : pins restant à 0 au reset et au boot : 2, 3, 4, 6, 7, 14

  #ifdef ESP32_v1
    const int PIN_RXModbus = 16;  // s3:18  devkitv1:16 RO
    const int PIN_TXModbus = 17;  // s3:17  devkitv1:17 DI
  #endif
  #ifdef ESP32_Fire2
    const int PIN_RXModbus = 18;  // s3:18  devkitv1:16 RO
    const int PIN_TXModbus = 17;  // s3:17  devkitv1:17 DI
  #endif
  #ifdef ESP32_uPesy
    const int PIN_RXModbus = 16;  // s3:18  devkitv1:16 RO
    const int PIN_TXModbus = 17;  // s3:17  devkitv1:17 DI
  #endif
  // const int PIN_RE = 32;
  // const int PIN_DE = 33;
  const int PIN_RXSTM = 18;  // RX STM32
  const int PIN_TXSTM = 17;  // TX STM32
#endif
// #define sorties analogique : 25 ou 26 (avec Dacwrite)

// Modbus
#ifdef ESP32_v1
#define MAX485_RE_NEG 32  // S3:35  devkitv1:32
#define MAX485_DE 33      // s3:36  devkitv1:33
#endif
#ifdef ESP32_Fire2
#define MAX485_RE_NEG 35  // S3:35
#define MAX485_DE 36      // s3:36
#endif
#ifdef ESP32_uPesy
#define MAX485_RE_NEG 32  // S3:35  devkitv1:32
#define MAX485_DE 33      // s3:36  devkitv1:33
#endif

/* ESP32S3 : Serial0:Pin 42 et 43
 */

// Structure d'un message uart
#define MSG_SIZE 40
typedef struct {
  char message[MSG_SIZE];
  uint16_t length;
} UartMessage;

typedef struct {
  uint16_t longueur;  // longueur
  char msg[MSG_SIZE];
} UartMessage_t;

float readBatteryVoltage();
void lectureHeure();
void requete_status(char* json_response, uint8_t socket, uint8_t type);
void recep_message1(UartMessage_t* messa);  // recept_uart1
void modif_timer_cycle(void);
void traitement_rx(UartMessage_t* mess);
uint8_t requete_Get_appli(const char* var, float* valeur);
uint8_t requete_Set_appli(String param, float valf);
uint8_t requete_GetReg(int reg, float* valeur);
void  activation_writelog();
void     setup_nvs_rtc();
void enreg_24h( uint8_t veille);
void printMemoryStatus();
void resetI2C();
void i2cRecovery();
void i2cBootRecovery();
uint16_t selec_graph(uint8_t code, uint16_t hum, uint16_t ha, uint16_t pir);
void detection_pir();
uint8_t envoi_valeur_instant(float Tint, float Humid, float HA);
void envoi_temp_hygro();

void passage_deep_sleep(uint64_t temps);

extern float Vbatt_ESP;   // Tension batterie ESP
extern struct tm timeinfo;


typedef enum {
  EVENT_NONE = 0,
  EVENT_INIT,
  EVENT_UART,
  EVENT_SENSOR,
  EVENT_GPIO_ON,
  EVENT_GPIO_OFF,
  EVENT_ERREUR,
  EVENT_ECOUTE_WebSock,
  EVENT_WATCHDOG,
  EVENT_24H,
  EVENT_3min,
  EVENT_CYCLE,
  EVENT_UART1
} systeme_eve_type_t;

// Structure d'un événement tache sequenceur
typedef struct {
  systeme_eve_type_t type;  // Type d'événement
  uint32_t data;            // Donnée associée (ex: valeur capteur, byte UART)
} systeme_eve_t;


/* Codes erreur*/
#define Code_erreur_Tint 1
#define Code_erreur_Text 2
#define Code_erreur_Heure 3
#define Code_erreur_depass_tab_status 4
#define Code_erreur_queue_full 5
#define Code_erreur_Json 5
#define Code_erreur_queue 6
#define Code_erreur_google 7
#define Code_erreur_http_local 8
#define Code_erreur_wifi 9
#define Code_erreur_esp_now 10


constexpr int NB_Graphique =
    6;  // Temp Ext, Temp int, Chaud, MoyText, MoyTint, Cout,
constexpr int NB_Val_Graph = 99;

#define NB_VAL_TAB  12

#define MAX_DUMP 6900              // 600 + 1050 car par graphique
extern char buffer_dmp[MAX_DUMP];  // max 250 logs, 16 octets chacun

extern  uint8_t esp_now_actif;  // 0:esp_now inactif  1:actif

extern uint8_t protocole;
extern QueueHandle_t eventQueue;  // File d'attente des événements sequenceur
extern uint16_t erreur_queue;
extern TimerHandle_t debounceTimer;
extern TimerHandle_t xTimer_activ_chaud;
extern  uint8_t periode_cycle;
extern  uint8_t mode_rapide;

extern  uint16_t compteur_detection;
extern  uint16_t Nb_PI[];

extern float Tint, Text, Humid;

extern uint8_t cpt_securite;
extern uint8_t WIFI_CHANNEL;
extern  uint8_t last_wifi_channel;     // Mémorisation du canal Wifi en DeepSleep
extern uint8_t rtc_valid;  // 0:cold reset  1:reset apres deep sleep
extern  uint16_t cpt_cycle_batt;                   // Compteur cycles pour mesure batterie
extern volatile uint8_t ackReceived;  // global pour indiquer que le peer a acké
extern volatile int ackChannel;       // canal où ça a marché
extern uint8_t mode_reseau;
extern  uint8_t init_time;
extern float heure;
extern  uint8_t skip_graph;

extern unsigned long last_remote_Tint_time, last_remote_Text_time,
    last_remote_heure_time;
extern  uint16_t err_Tint, err_Text, err_Heure;

extern  float tempI_moy24h, tempE_moy24h, Hum_24h, HA_moy24h, PIR_24h;
extern  uint16_t cpt24_Tint, cpt24_Text, cpt24_Hum, cpt24_HA, cpt24_PIR;

extern  uint8_t action_envoi;

extern char mdp_routeur[];
extern  int16_t graphique[NB_Val_Graph][NB_Graphique];
extern uint16_t Seuil_batt_sonde;  // millivolt
extern uint16_t Seuil_batt_arret_ESP;  // millivolt
extern uint8_t type_reveil;  //0:pas de reveil 1: réveil par timer, 2: réveil par PIR  3:inconnu
// 4:reveil par bouton_reveil   10:toujours actif


extern  uint8_t etat_now;
extern  uint8_t Nb_jours_Batt_log;

extern  bool force_stay_awake;
extern unsigned long wake_up_time;  // Temps de réveil/dernière activité

void writeLog(uint8_t code, uint8_t c1, uint8_t c2, uint8_t c3,
              const char* message);
void debounceCallback(TimerHandle_t xTimer);
uint16_t crc16_arc(const uint8_t* data, size_t length);
void log_erreur(uint8_t code, uint8_t valeur,
                uint8_t val2);  // Code:1:Tint, 2:Text, 3:TPac;
void init_10_secondes();
void setup_0();
void setup_nvs();
void setup_1();
void setup_2();
uint8_t requete_action_appli(const char* reg, const char* data);
void appli_event_on(systeme_eve_t evt);
void appli_event_off(systeme_eve_t evt);
uint8_t requete_Get_appli(String var, float* valeur);
uint8_t requete_Set_appli(String param, int val);
uint8_t requete_GetReg_appli(int reg, float* valeur);
uint8_t requete_SetReg_appli(int param, float valeurf);
uint8_t requete_Get_String_appli(uint8_t type, String var, char* valeur);
uint8_t requete_Set_String_appli(int param, const char* texte);
uint8_t lecture_Tint(float* mesure, float *hum );
uint8_t lecture_Text(float* mesure);
void event_cycle();
void envoi_detection();

// Fonctions WiFi
uint8_t connectWiFiWithDiagnostic();
void diagnoseWiFiError();
void protectUARTDuringWiFi();

// Configuration DHT22
#define DHT22_TIMEOUT_MS 5000       // Timeout de lecture DHT22 en millisecondes
#define DHT22_MIN_INTERVAL_MS 2000  // Intervalle minimum entre lectures DHT22

#endif