/* 

TODO : 

A compiler pour une firebeetle esp32-C6 ou uPesy vroom
Avantages Platformio : Ifdef, intellisense, temps compil, backtrace

v1.3 05/2026 board upesy, humidite absolue, envoi vers serveur
v1.2 03/2026 Surveillance batterie, log 24h en eeprom, OTA à la demande
v1.1 03/2026 copie de plat_esp_chad_gar v1.11 de 3/2026

Graphique 1 : par cycle : G1:temp_int(vert) G2:temp ext(bleu) G3:Nbdetect par cycle
Graphique 2 : par 24h :   G1:Temp int(vert) G2:temp_ext(bleu) G3:Nb detect/24h

Interrogation des données par la liaison série (recep_message) : 2-1 (type1, reg1)

Conso sonde : 37uA en veille, 17mA pdt 1sec => 19uA. Avec 2000mAh => 4 Années

Configuration des options de programmation : 
- usb CDC on boot :  enable (pour permettre les serial.print)
- cpu frequency : 
- code debug level : non ou info  
-usb dfu on boot : 
-usb firmware msc on boot : 
- Events run on Core 1
- Arduino run on Core 0
- Flash mode : QIO 80MHz
- jtag adaptater : 
- upload mode: Uart0
- usb mode : hardware cdc & jtag (usage basique)
- partition : custom (pour permettre code>1,5MOctets)
*/




#define DELAI_PING  180  // en secondes, pour le websocket
#define Version "V1.3"


#define DEBUG_ETHERNET_WEBSERVER_PORT Serial

// Debug Level from 0 to 4
#define _ETHERNET_WEBSERVER_LOGLEVEL_ 1

#define temps_boucle_loop  2   // secondes
#define attente_init     10     // 10 secondes avant de demasquer les entrées
#define attente_mise_heure  30  // 30 secondes apres le temps d'init ci-dessus pour verifier/maj l'heure

static bool eth_connected = false;

#include <Arduino.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "variables.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>  // pour nvs eeprom

// #include <OneWire.h>   // ESP32
// #include <DallasTemperature.h>  // ESP32
// #include <OneWireNg_DS18B20.h>  // plus large
// #include <OneWireNg_CurrentPlatform.h>  // plus large

#include <DHT.h>
#include <PID_v1.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "esp_pm.h"


//#include <ESP32Time.h>
#include "site_web.h"
#include "time.h"
#include <base64.h>  // Nécessaire pour encoder les données binaires
#include <ArduinoOTA.h>  // nécessaire pour OTA
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include "driver/rtc_io.h"


#ifdef __cplusplus
extern "C" {
#endif
#include "esp_log.h"
#ifdef __cplusplus
}
#endif
extern "C" {
  #include "esp_timer.h"
  #include "esp_sleep.h"
  #include "esp_system.h"
  #include "esp_partition.h"
  #include "esp_task_wdt.h"
  #include "nvs_flash.h"
  #include "esp_log.h"
  // #include "esp_rom/rtc.h"  // si tu le réactives
}

#include <Wire.h>
#include "esp_log.h"
#include "esp_timer.h"
//#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"
#include "i2c.h"
#include "FS.h"
#include "SD_MMC.h"

uint8_t err_wifi_repet;  // permet de resetter si le wifi ne se rétablit pas au bout de 4 jours

uint8_t init_masquage=1;
RTC_DATA_ATTR uint8_t cpt24h_batt;

uint8_t envoi_data_gateway(Message_EspNow mess_esp);
uint8_t parseMacString(const char* str, uint8_t mac[6]);

// variable globale de 4000c en RAM pour dump log et autres requetes
char buffer_dmp[MAX_DUMP];  // max 250 logs, 16 octets chacun


#ifdef MODE_Wifi
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  // Suppression des warnings de dépréciation pour ArduinoWebsockets
  #include <ArduinoWebsockets.h>

  // adresse IP pour le Wifi_Access Point
  const char *ssid_AP = "ESP32";  // Enter SSID here pour l'AP
  const char *password_AP = "";   //Enter Password here
  /* Put IP Address details */
  IPAddress local_ip_AP(192, 168, 254, 1);  
  IPAddress gateway_AP(192, 168, 254, 1);
  IPAddress subnet_AP(255, 255, 255, 0);

  
  // Adresse IP pour le wifi_station (en mode debug)
  const char *ssid = "garches";               // Enter SSID du Routeur Wifi : garches:garches  kerners:kerners catalane:catalane  Theoule:SFR_EFF0
  const char *password = "196492380";    //Enter Password here
  IPAddress Slocal_ip(192, 168, 251, 50);  // Définir l'adresse IP statique souhaitée Garches:251.50 Catalane:248.5
  IPAddress Sgateway(192, 168, 251, 1);   // Définir la passerelle (généralement l'adresse du routeur)
  IPAddress Ssubnet(255, 255, 255, 0);    // Masque de sous-réseau
  IPAddress SprimaryDNS(8, 8, 8, 8);      // DNS Primaire (Google DNS)
  IPAddress SsecondaryDNS(8, 8, 4, 4);    // DNS Secondaire (Google DNS)

  IPAddress local_ip;  // Définir l'adresse IP statique souhaitée Garches:251.50 Catalane:248.5
  IPAddress gateway;   // Définir la passerelle (généralement l'adresse du routeur)
  IPAddress subnet;    // Masque de sous-réseau
  IPAddress primaryDNS;      // DNS Primaire (Google DNS)
  IPAddress secondaryDNS;    // DNS Secondaire (Google DNS)


#else  // WT32
  #include <ETH.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <ArduinoWebsockets.h>

  // Ethernet : Select the IP address according to your local network
  IPAddress myIP(192, 168, 251, 16);
  IPAddress myGW(192, 168, 251, 1);
  IPAddress mySN(255, 255, 255, 0);
  IPAddress myDNS(8, 8, 8, 8);  // Google DNS Server IP


  #ifndef ETH_PHY_TYPE
    #define ETH_PHY_TYPE ETH_PHY_LAN8720
    #define ETH_PHY_ADDR 0
    #define ETH_PHY_MDC 23
    #define ETH_PHY_MDIO 18
    #define ETH_PHY_POWER -1
    #define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
  #endif
#endif

uint16_t test1=0;
uint16_t test2=0;
volatile bool debounceFlag = false;

// etat d'un system externe commandable par Pin_on et pin_off  : 0 éteint, 1:allumé
uint8_t systeme_marche=0;

uint8_t mode_reseau=13;  //  0:pas de reseau 11:wifi_AP_usine  12:wifi_AP   13:wifi_routeur  14:Ethernet filaire 
#ifdef NO_RESEAU
  mode_reseau=0;
#endif

char nom_routeur[16]="";
char mdp_routeur[16]="";
unsigned long last_remote_Tint_time = 0, last_remote_Text_time=0, last_remote_heure_time=0;

RTC_DATA_ATTR int16_t  graphique [NB_Val_Graph][NB_Graphique];

// Status
RTC_DATA_ATTR uint32_t rtc_magic = 0xDEADBEEF;
uint8_t rtc_valid;  // 0:non valide-cold reset  1:reset apres deep sleep : RTC valide
uint16_t nb_err_reseau;
RTC_DATA_ATTR uint16_t cpt_cycle_batt; // Compteur cycles pour mesure batterie
uint16_t erreur_queue=0;
uint8_t num_err_queue[5];
uint8_t cpt_init=0;
RTC_DATA_ATTR uint8_t etat_ESP_stop;  // 0:normal 1:arrêté pour batterie faible (deepsleep longue duree)

// websocket
uint8_t websocket_on=0;     // 1:off 2:on
char ip_websocket[40]="";   // ws://webcam.hd.free.fr:8081";
uint8_t id_websocket=0;
using namespace websockets;
WebsocketsClient wsClient;  // Client WebSocket
bool isWebSocketConnected = false;
uint8_t DelaiWebsocket = 1;
uint16_t cpt_ws_timeout=0, cpt_ws_ping=0;

bool force_stay_awake = false; // Flag pour rester éveillé après appui bouton
uint8_t type_reveil;  //0:power_on 1: réveil par timer, 2: réveil par PIR 3:inconnu 4:reveil par BTN 10:actif
unsigned long wake_up_time = 0; // Temps de réveil

//x seconds Watchdog 
#define WDT_TIMEOUT 80  // reset watchdog dividé par 3 => 24 secondes
uint16_t param_wdt_delay = 1;


const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600 * 1;      // fuseau horaire GMT+1h
const int daylightOffset_sec = 3600 * 1;  // heure d'été
struct tm timeinfo;
RTC_DATA_ATTR uint8_t init_time = 0;  // 0:pas initialisé, 1:à8h, 3:avec internet 4:manuel
RTC_DATA_ATTR uint8_t last_wifi_channel; // Mémorisation du canal Wifi en DeepSleep
RTC_DATA_ATTR uint8_t esp_now_actif;  // 0:esp_now inactif  1:actif
float Vbatt_ESP = 0.0;   // Stockage tension batterie

RTC_DATA_ATTR uint8_t periode_cycle;


//Freertos
QueueHandle_t eventQueue;  // File d'attente des événements sequenceur
QueueHandle_t QueueUart;   // file d'attente pour message uart en réception
QueueHandle_t QueueUart1;   // file d'attente pour message uart en réception

//timers
esp_timer_handle_t timer;

//taches
TaskHandle_t taskHandle = NULL;

// EEprom avec la partition NVS : permet une sauvegarde de variables
Preferences preferences_nvs;

// partition Log_flash pour permettre 250 logs : utilisation de 2 pages de 4ko = 8ko au total
#define PAGE_SIZE 4096     // Taille d'une page LOG_FLASH
#define LOG_ENTRY_SIZE 16   // 4 (timestamp) + 1 (code) + 3 (char) + 8 car
#define NB_ENTRY  255       // 1 de moins qu'indiqué. Nb de log par page : PAGE_SIZE/LOG_ENTRY_SIZE -2
//#define PAGE_1_START 0     // Début de la première page
//#define PAGE_2_START 4096  // Début de la deuxième page

uint8_t activePage=0;  // 0: non defini  1 ou 2
uint16_t activeIndex=0;  // 0:non défini


// partition Log_flashG pour permettre 250 Event24h Graphique : utilisation de 2 pages de 4ko = 8ko au total
#define PAGE_SIZEG 4096     // Taille d'une page LOG_FLASHG
#define LOG_ENTRY_SIZEG 16   // 4 (timestamp) + 1 (code) + 3 * uint16_t
#define NB_ENTRYG  255       // 1 de moins qu'indiqué. Nb de log par page : PAGE_SIZE/LOG_ENTRY_SIZE -2

uint8_t activePageG;  // 0: non defini  1 ou 2
uint16_t activeIndexG;  // 0:non défini




typedef struct __attribute__((packed)) {  // padding 16 : pour éviter  mauvaises surprises
  uint32_t timestamp;  // 4 octets
  uint8_t code;      // 1 octet
  uint8_t c1;          // 1 octet
  uint8_t c2;          // 1 octet
  uint8_t c3;    // 1 octet
  char message[LOG_ENTRY_SIZE-8];    // 8 octets
} LogEntry;

const esp_partition_t* logPartition;
size_t logOffset = 0;
uint8_t log_err=2;  // 1 si la partition log_flash n'est pas trouvée  2:autre problème
uint8_t log_detail;

typedef struct __attribute__((packed)) {  // padding 16 : pour éviter  mauvaises surprises
  uint32_t timestamp;  // 4 octets
  uint8_t code;      // 1 octet
  uint16_t c1;          // 2 octet
  uint16_t c2;          // 2 octet
  uint16_t c3;        // 2 octet
  uint8_t reste8;      // 1 octet
  uint32_t reste32;   // 4 octets
} LogEntryG;

const esp_partition_t* logPartitionG;
size_t logOffsetG = 0;
uint8_t log_errG=2;  // 1 si la partition log_flashG n'est pas trouvée  2:autre problème
uint8_t log_detailG;

TimerHandle_t xTimer_Init;
TimerHandle_t xTimer_Websocket;
TimerHandle_t xTimer_Watchdog;
TimerHandle_t xTimer_3min;
TimerHandle_t xTimer_24H;
TimerHandle_t xTimer_Cycle;
//TimerHandle_t xTimer_Compresseur;
TimerHandle_t xTimer_Securite;



WiFiClient client;

RTC_DATA_ATTR uint8_t mode_rapide;

uint16_t nb_reset;
uint32_t time_reset0;  // temps lors du precedent reset
uint32_t time_reset1;  // temps lors reset actuel

uint8_t etat_connect_ethernet = 0;

AsyncWebServer server(80);

uint8_t cycle24h;
RTC_DATA_ATTR float  tempI_moy24h=0, tempE_moy24h=0, Hum_24h=0, HA_moy24h=0, PIR_24h=0;
RTC_DATA_ATTR uint16_t cpt24_Tint=0, cpt24_Text=0,  cpt24_Hum=0, cpt24_HA=0, cpt24_PIR=0;
RTC_DATA_ATTR uint16_t TextV=0, TintV=0, HumV=0, HAV=0, PIRV=0;  // pour stockage dans la partition log_flashG


// OTA
bool otaEnabled = false;
unsigned long otaStartTime = 0;

#define DEBOUNCE_INTERVAL 100  // Temps anti-rebond en ms
#define VALIDATION_COUNT 5  // Nombre de lectures consécutives pour valider un changement
volatile unsigned long lastInterruptTime = 0; 

TimerHandle_t debounceTimer;
#define BTN_COUNT 1  // Nombre de boutons
volatile int lastButtonState[BTN_COUNT] = {HIGH};  // États précédents
volatile int stableButtonState[BTN_COUNT] = {HIGH}; // États stables validés
int pressCounter[BTN_COUNT] = {0}; // Compteurs de validation

const int BTN_PIN[BTN_COUNT] = {14};  // Pins des boutons


#define configASSERT_CODE( x, code ) if( ( x ) == 0 ) { \
    taskDISABLE_INTERRUPTS();                           \
    writeLog('A', code, 0, 0, "ASSERT");         \
    Serial.print("ASSERT FAILED, code: ");              \
    Serial.println(code);                               \
    while(1) { }                                         \
}
// utiliser comme cela : configASSERT_CODE(ptr != NULL, 3);   // code 3 = pointeur NULL

uint8_t resetReason0;
esp_sleep_wakeup_cause_t source_reveil;
char resetREASON0[50];
char duree_RESET[60];

// Gestion des erreurs
#define Nb_erreurs 10
uint8_t erreur_code[Nb_erreurs];
uint8_t erreur_valeur[Nb_erreurs];
uint8_t erreur_val2[Nb_erreurs];
uint8_t erreur_jour[Nb_erreurs];
uint8_t erreur_heure[Nb_erreurs];
uint8_t erreur_minute[Nb_erreurs];

// index
uint8_t index_val;



// heure ESP32
uint8_t an, ms, jM, jS, hr, mn, sd;
float heure;
uint16_t date_ac;
char heure_string[16] = "\"\"";
int periode_lecture_heure = 5;  // init :5 secondes  ensuite:30 sec


// securite modif
unsigned long temps_activ_secu;
uint8_t cpt_securite = 0;


unsigned long previousMillis_temp = 20000;  // 1er à 20s
unsigned long previousMillis_inittime;
char St_Uptime[35];
RTC_DATA_ATTR uint8_t skip_graph;



void init_time_ps();
int myLogPrinter(const char *fmt, va_list args);
const char* dumpTasksInfo();
void setupRoutes();
void startWebServer();
uint8_t reConnectWifi();
uint8_t testConnexionGoogle();
uint8_t testHttpServerLocal();
void recep_message(char *messa);
void connectWebSocket();
void printMemoryStatus();
void checkPartitions();
int readLastLogsBinary(uint8_t *buffer, int nombre);
void requete_status(char *json_response, uint8_t socket, uint8_t type);
uint8_t requete_Set(uint8_t type, const char* param, const char* valStr);
uint8_t requete_Get(uint8_t type, const char* var, float *valeur);
uint8_t requete_Get_String (uint8_t type, String var, char *valeur);
uint8_t requete_SetReg(int param, float valeurf);
uint8_t requete_SetRegM(uint8_t param, int valeur);
uint8_t requete_Set_String(int param, const char *texte);
uint8_t requete_Set_Action(const char *reg, const char *data);
uint16_t trouve_index (uint8_t page);
uint8_t requete_GetReg(int reg, float *valeur);
void writeLogG(uint8_t code, uint16_t c1, uint16_t c2, uint16_t c3);
uint16_t trouve_indexG (uint8_t page);





const char* verbose_reset_reason(int reason) {
  static char resetREAS[50]; // static car permet de conserver la variable au retour de la fonction
    switch (reason) {
    case 0: return "reset inconnu";
    case 1: return "power on";
    case 2: return "signal externe";
    case 3: return "soft:esp_restart";
    case 4: return "Exception";
    case 5: return "watchdog interrupt";
    case 6: return "watchdog tache";
    case 7: return "watchdog autre";
    case 8: return "reveil deep sleep";
    case 9: return "brown out-tension";
    case 10: return "periph SDIO";
    case 11: return "periph USB";
    case 12: return "reset JTAG";
    case 13: return "reset EFUSE";
    case 14: return "reset OTP";
    case 15: return "reset GPIO";
    case 16: return "reset CPU";
    case 17: return "reset CPU0";
    case 18: return "reset CPU1";
    case 19: return "reset CPU2";
    case 20: return "reset CPU3";
    default: 
      snprintf(resetREAS, sizeof(resetREAS), "reset (code: %d)", reason);
    return resetREAS;
  }
}



#ifndef MODE_Wifi  // Pour Ethernet cablé
// WARNING: onEvent is called from a separate FreeRTOS task (thread)!
void onEvent(arduino_event_t event) {
  switch (event.event_id) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      // The hostname must be set after the interface is started, but needs
      // to be set before DHCP, so set it from the event handler thread.
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      etat_connect_ethernet = 2;
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.println(ETH);
      eth_connected = true;
      etat_connect_ethernet = 2;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      etat_connect_ethernet = 0;
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      etat_connect_ethernet = 0;
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      etat_connect_ethernet = 0;
      eth_connected = false;
      break;
    default: break;
  }
}
#endif


// timeout à la fin des 10 premières secondes apres l'initialisation
void vTimerInitCallback(TimerHandle_t xTimer)
{
  systeme_eve_t evt = { EVENT_INIT, 0 };
  if (xQueueSendFromISR(eventQueue, &evt, NULL) != pdTRUE) 
  {
      if (erreur_queue<5) num_err_queue[erreur_queue]=1;
      erreur_queue++;
  }
}

// timeout pour fin de securite modif
void vTimerSecuriteCallback(TimerHandle_t xTimer)
{
  cpt_securite = 0;
}

// timeout chaque 3 minute : (test wifi)
void vTimer3minCallback(TimerHandle_t xTimer)
{
    systeme_eve_t evt = { EVENT_3min, 0 }; 
    if (xQueueSendFromISR(eventQueue, &evt, NULL) != pdTRUE) 
    {
      if (erreur_queue<5) num_err_queue[erreur_queue]=2;
      erreur_queue++;
    }
}

// timeout chaque 24 heures : (ping, keepalive)
void vTimer24HCallback(TimerHandle_t xTimer)
{
    systeme_eve_t evt = { EVENT_24H, 0 };  // Exemple : donnée = 123
    if (xQueueSendFromISR(eventQueue, &evt, NULL) != pdTRUE) 
    {
      if (erreur_queue<5) num_err_queue[erreur_queue]=3;
      erreur_queue++;
    }
}

// timeout chaque 15 minutes : cycle
void vTimerCycleCallback(TimerHandle_t xTimer)
{
    systeme_eve_t evt = { EVENT_CYCLE, 0 };  // Exemple : donnée = 123
    if (xQueueSendFromISR(eventQueue, &evt, NULL) != pdTRUE) 
    {
      if (erreur_queue<5) num_err_queue[erreur_queue]=3;
      erreur_queue++;
    }
}


// timeout pour délai ecoute websocket
void vTimerWebsocketCallback(TimerHandle_t xTimer)
{
  systeme_eve_t evt = { EVENT_ECOUTE_WebSock, 0 };
  if (xQueueSendFromISR(eventQueue, &evt, NULL) != pdTRUE) 
    {
      if (erreur_queue<5) num_err_queue[erreur_queue]=4;
      erreur_queue++;
    }
}

// timeout pour watchdog
void vTimerWatchdogCallback(TimerHandle_t xTimer)
{
  systeme_eve_t evt = { EVENT_WATCHDOG, 0};
  if (xQueueSendFromISR(eventQueue, &evt, NULL) != pdTRUE) 
    {
      if (erreur_queue<5) num_err_queue[erreur_queue]=5;
      erreur_queue++;
    }
}




// timer debounce pour lire toutes les entrées
void IRAM_ATTR onButtonInterrupt() {
    xTimerStartFromISR(debounceTimer, NULL);  // Un seul timer pour tous les boutons
}


void uartTask(void * parameter) {
  char uartInput[MSG_SIZE];
  uartInput[0] = '\0';
  uint16_t uartLong=0;

  for (;;) {
    // Protection contre les interférences WiFi
    if (WiFi.status() == WL_CONNECTED) {
      // Pause plus longue quand WiFi est actif pour éviter les conflits
      vTaskDelay(20 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    while (Serial.available()) {
      char c = Serial.read();
      //Serial.print("recu");
      if (c == '\n' || c == '\r')
      { // Fin de ligne
        if (uartLong > 0) {
          if (uartLong > MSG_SIZE-1)
            uartLong = MSG_SIZE-1;
          uartInput[uartLong]=0; // terminaison

          UartMessage_t mess;
          strncpy(mess.msg, uartInput, uartLong+1);  // Copie sécurisée
          mess.msg[MSG_SIZE - 1] = '\0';  // Assurer la terminaison          
          mess.longueur = uartLong+1;
          uint8_t status = xQueueSend(QueueUart, &mess, 0);
          if (!status) 
          {
            if (erreur_queue<5) num_err_queue[erreur_queue]=6;
            erreur_queue++;
          }

          systeme_eve_t evt;
          evt.type = EVENT_UART;
          evt.data = uartLong+1;
          status = xQueueSend(eventQueue, &evt, 0);
          if (!status)
          {
            if (erreur_queue<5) num_err_queue[erreur_queue]=7;
            erreur_queue++;
          }
          if (status!=pdPASS) log_erreur(Code_erreur_queue_full, status, 0);
          uartInput[0]=0;
          uartLong=0;
        }
      } else
      {
        // Filtre : On ne garde que les caractères imprimables pour éviter les parasites
        if (c >= 32 && c <= 126) 
        {
          if (uartLong < MSG_SIZE-1)
          {
            uartInput[uartLong++] = c;
          }
        }
      }
    }
  }
}

void uart1Task(void * parameter) {
  char uartInput[MSG_SIZE];
  uartInput[0] = '\0';
  uint16_t uartLong=0;

  for (;;) {
    while (Serial1.available()) {
      char c = Serial1.read();
      //Serial.print("recu");
      if (c == '\n' || c == '\r')
      { // Fin de ligne
        if (uartLong > 0) {
          if (uartLong > MSG_SIZE-1)
            uartLong = MSG_SIZE-1;
          uartInput[uartLong]=0; // terminaison

          UartMessage_t mess;
          strncpy(mess.msg, uartInput, uartLong+1);  // Copie sécurisée
          mess.msg[MSG_SIZE - 1] = '\0';  // Assurer la terminaison          
          mess.longueur = uartLong+1;
          uint8_t status = xQueueSend(QueueUart1, &mess, 0);
          if (!status) 
          {
            if (erreur_queue<5) num_err_queue[erreur_queue]=8;
            erreur_queue++;
          }

          systeme_eve_t evt;
          evt.type = EVENT_UART1;
          evt.data = uartLong+1;
          status = xQueueSend(eventQueue, &evt, 0);
          if (!status)
          {
            if (erreur_queue<5) num_err_queue[erreur_queue]=9;
            erreur_queue++;
          }

          if (status!=pdPASS) log_erreur(Code_erreur_queue_full, status, 1);
          uartInput[0]=0;
          uartLong=0;
        }
      } else
      {
        if (uartLong < MSG_SIZE-1)
        {
          uartInput[uartLong++] = c;
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // Petite pause
  }
}


void payloadWrite16(uint8_t* payload, uint8_t& pos, uint16_t val)
{
    payload[pos++] = val & 0xFF;
    payload[pos++] = (val >> 8) & 0xFF;
}

uint16_t selec_graph(uint8_t code, uint16_t ha, uint16_t hum,  uint16_t pir)
{
  if (code==0) return ha;
  else if (code==1) return hum;
  else return pir;
}

// log batterie et Temp moyenne toutes les 24h.
void enreg_24h( uint8_t veille)
{
    // Log toutes les jours/semaines : nb d'erreurs wifi et batteries
  if (Nb_jours_Batt_log)
  {
    float vbatt = readBatteryVoltage();
    Serial.printf("Tension Batterie : %.2f V\n", vbatt);

    if ((Seuil_batt_arret_ESP) && (vbatt < Seuil_batt_arret_ESP) && (vbatt > 2400))
    {
      writeLog('S', 7, 0, 0, "Batt Stp");
      etat_ESP_stop = 1;
    }
    cpt24h_batt++;
    if (cpt24h_batt >= Nb_jours_Batt_log)  // log chaque X jour 
    {
      cpt24h_batt=0;

      //Serial.printf("Tension Batterie 24h : %.2f %.2f V\n", vbatt, Vbatt_Th);
      if (veille)
      {
        writeLog('K', (uint8_t)((vbatt-2.0)*100), 0, 0, "24H_Res");
      }
      else
      {
        if (nb_err_reseau>255) nb_err_reseau=255;
        writeLog('K', (uint8_t)((vbatt-2.0)*100), 0, (uint8_t)nb_err_reseau, "24H_Res");
        nb_err_reseau=0;
      }
    }
  }

  // erreurs Tint, Text, heure
  if (err_Tint>254) err_Tint=255;
  if (err_Tint)
            log_erreur(Code_erreur_Tint, err_Tint,0);
  if (err_Text>254) err_Text=255;
  if (err_Text)
            log_erreur(Code_erreur_Text, err_Text,0);
  if (err_Heure>254) err_Heure=255;
  if (err_Heure)
            log_erreur(Code_erreur_Heure, err_Heure,0);
  err_Tint=0;
  err_Text=0;
  err_Heure=0;

  // graphique des temperatures quotidiennes
  uint16_t tempI=1, tempE=1, Hum=1, HA=1, PIR=1;
  if (cpt24_Tint)  tempI = (uint16_t)(tempI_moy24h/cpt24_Tint*10);
  if (cpt24_Text)  tempE = (uint16_t)(tempE_moy24h/cpt24_Text*10);
  if (cpt24_Hum) Hum = (uint16_t)(Hum_24h/cpt24_Hum*10);
  if (cpt24_HA) HA = (uint16_t)(HA_moy24h/cpt24_HA*10);
  if (cpt24_PIR) PIR = (uint8_t)(PIR_24h/cpt24_PIR*10);
  if (!tempI) tempI=1;  // permet d'afficher quand meme le point sur le graphique
  if (!tempE) tempE=1;  // permet d'afficher quand meme le point sur le graphique
  if (!Hum) Hum=1;  // permet d'afficher quand meme le point sur le graphique
  if (!HA) HA=1;  // permet d'afficher quand meme le point sur le graphique
  if (!PIR) PIR=1;  // permet d'afficher quand meme le point sur le graphique
  TextV = tempE;
  TintV = tempI;
  HumV = Hum;
  HAV = HA;
  PIRV = PIR;

  //Serial.printf("Temp24h I:%.2f %i E:%.2f %i C:%.2f %i\n\r", tempI_moy24h, cpt24_Tint, tempE_moy24h, cpt24_Text, cout_moy24h, cpt24_Cout);

  tempI_moy24h=0;
  tempE_moy24h=0;
  Hum_24h=0;
  HA_moy24h=0;
  PIR_24h=0;
  cpt24_Tint=0;
  cpt24_Text=0;
  cpt24_Hum=0;
  cpt24_HA=0;
  cpt24_PIR=0;

  uint8_t i;
  for (i = NB_Val_Graph - 1; i; i--) {
    graphique[i][3] = graphique[i - 1][3];
    graphique[i][4] = graphique[i - 1][4];
    graphique[i][5] = graphique[i - 1][5];
  }
  graphique[0][3] = tempI;
  graphique[0][4] = selec_graph(GRAPH2, HA, Hum, PIR);
  graphique[0][5] = selec_graph(GRAPH3, HA, Hum, PIR);  

  Serial.printf("Graphique 24h : Tint:%i Text:%i Hum:%i HA:%i\n\r", tempI, tempE, Hum, HA);
  writeLogG('G', tempI, HA, Hum); // Enregistrment en Flash des 3 valeurs du graphique

  if (action_envoi && esp_now_actif)
  {
    Message_EspNow message;

    message.destinataire = SERVER_ADD & 0x80;  // 0x80 = message hexa
    message.emetteur = ADDRESS;
    message.code = 'C';
    message.code2 = 'J';

    uint8_t pos = 0;
    payloadWrite(message.payload, pos, tempI);
    payloadWrite(message.payload, pos, Hum);
    message.longueur = 5 + pos - 3;

    uint8_t result = envoi_data_gateway(message);

  }      
}


void taskHandler(void *parameter) {
    systeme_eve_t evt;
    #ifdef WatchDog
      esp_err_t status = esp_task_wdt_add(NULL);  //add current thread to WDT watch
      //Serial.printf("enreg wdt taskhandler : %i\n\r", status);
    #endif

    Serial.printf("Taskhandler : coeur %d\n\r", xPortGetCoreID());

    while (true) {
        // Attendre un 2 événement (bloque tant qu'il n'y a rien)
        if (xQueueReceive(eventQueue, &evt, portMAX_DELAY) == pdTRUE) 
        {
            // Gestion des événements
            //Serial.println(evt.type);
            switch (evt.type) {
 
                case EVENT_INIT: {
                  if (!cpt_init)
                  {
                    // 10 secondes apres l'initialisation : fin masquage entrees
                    init_10_secondes();
                    init_masquage=0;
                    Serial.println("fin masquage des entrees");
                    xTimerChangePeriod(xTimer_Init,attente_mise_heure*(1000/portTICK_PERIOD_MS),100);  // passage à 30 secondes
                    xTimerStart(xTimer_Init,100);
                    cpt_init++;
                  }
                  else
                  {
                    // 10sec+30secondes apres init : vérif heure
                    // au bout de 2 minutes : vérif chaque 30 minutes
                    if (mode_reseau == 11)
                      xTimerStop(xTimer_Init,100); // arret
                    else
                    {
                      Serial.println("verification de l'heure");
                      // Serial.flush();
                      init_time_ps();
                      if (init_time>=3)  xTimerStop(xTimer_Init,100); // arret
                    }
                  }
                }
                break;
                case EVENT_UART: {
                    // Si on est en mode "Stay Awake", on prolonge de 30s à chaque message complet reçu
                    #ifdef ESP_VEILLE
                        // Si une commande UART arrive, on force le réveil si ce n'est pas déjà fait
                        if (!force_stay_awake) {
                            force_stay_awake = true; 
                            Serial.println("INTERCEPTION UART: Activation du délai de 30s !");
                        }
                    #endif

                    if (force_stay_awake) {
                        wake_up_time = millis() + 50000; // Prolonger sur réception  message UART
                        Serial.println("Activité UART détectée : prolongation du délai de 30s.");
                    }
                    
                    UartMessage_t uartMsg;
                    while (xQueueReceive(QueueUart, &uartMsg, 0) == pdTRUE) {
                        Serial.printf("Message UART (%d octets) : ", uartMsg.longueur);
                        for (uint16_t i = 0; i < uartMsg.longueur-1 && i < MSG_SIZE; i++) {
                            Serial.print(uartMsg.msg[i]);
                        }
                        Serial.println();
                        recep_message(uartMsg.msg);
                    }                  
                    break;
                }

                case EVENT_UART1: {
                  //Serial.printf(">> Événement UART reçu: byte = %s\n", evt.msg); 
                  // 1-12  1-12:48
                  UartMessage_t uartMsg1;
                  while (xQueueReceive(QueueUart1, &uartMsg1, 0) == pdTRUE) {
                    Serial.printf("Message UART1 (%d octets) : ", uartMsg1.longueur);
                    for (uint16_t i = 0; i < uartMsg1.longueur-1 && i < MSG_SIZE; i++) {
                      Serial.print(uartMsg1.msg[i]);
                    }
                    Serial.println();
                    recep_message1(&uartMsg1);
                  }                  
                  break;
                }
 
                case EVENT_GPIO_OFF:  
                    Serial.printf("GPIO:off:%i\n\r", evt.data);
                    appli_event_off(evt);
                    break;

                case EVENT_GPIO_ON:  // 0:BTN0 1:BTN1
                    Serial.printf("GPIO:on:%i\n\r", evt.data);
                    appli_event_on(evt);
                    break;

               case EVENT_SENSOR:
                    Serial.printf(">> Événement Capteur: valeur = %lu\n", (unsigned long)evt.data);
                    break;

                case EVENT_ECOUTE_WebSock:   // chaque seconde
                  if (websocket_on==2) {
                    wsClient.poll();  // Vérifie les messages entrants
                    cpt_ws_ping++;

                    if (cpt_ws_ping>DELAI_PING)   // envoie un message ping toutes les 60 secondes
                    {
                      if (wsClient.available()) {
                        String jsonMessage = "1ping"; //"{\"action\":\"ping\"}";
                        wsClient.send(jsonMessage);
                        Serial.println("ping -> websocket");
                      }
                      else
                        Serial.println("60s");

                      cpt_ws_ping=0;
                   }
                    cpt_ws_timeout++;
                    if (cpt_ws_timeout> (DELAI_PING * 2.1))   // Vérifier si la connexion WebSocket est active : chaque 126 secondes
                    {
                      cpt_ws_timeout=0;
                      if (!isWebSocketConnected) 
                      {
                        Serial.println("⏳ Timeout WebSocket ! Déconnexion détectée.");
                        connectWebSocket();
                      }
                      isWebSocketConnected=false;
                    }
                  }
                  break;

                case EVENT_WATCHDOG:   // chaque 7.5 secondes
                {
                  #ifdef WatchDog
                    //delay(param_wdt_delay * 100);
                    vTaskDelay(param_wdt_delay * 100 / portTICK_PERIOD_MS); //  pause pour faire reset watchdog

                    esp_task_wdt_reset();  // taskHandler
                    //Serial.println("reset watchdog Event");
                    vTaskDelay( 100/ portTICK_PERIOD_MS);  // Important de mettre au moins 1ms
                  #endif

                }
                break;

                case EVENT_3min:
                {
                  #ifndef NO_RESEAU
                    uint8_t test_goog = testConnexionGoogle();
                    uint8_t test_wifi=3;
                    if (mode_reseau != 14)
                      test_wifi = WiFi.status();
                    if ((test_wifi != 3) || (test_goog))
                    {
                      Serial.println("pas de connec reseau");
                      delay(100);
                      nb_err_reseau++;
                      if (mode_reseau!=14)
                      { 
                        //server.end();  // sécuriser l'état précédent
                        reConnectWifi();
                        //startWebServer();
                      }
                    }
                    else
                      Serial.println("test reseau ok");
                  #endif
                }
                break;

                case EVENT_24H:
                {
                  #ifdef WatchDog
                    esp_task_wdt_reset();
                  #endif
                  uint8_t test_goog=0, test_wifi=3;                  
                  getLocalTime(&timeinfo);  // déclenche resynchro réseau à chaque appel. 5s si pb reseau

                  // faire un ping pour vérifier que la liaison IP fonctionne
                  if (mode_reseau==13)
                  {
                    test_goog = testConnexionGoogle();
                    test_wifi = WiFi.status();
                  }
                  #ifndef NO_RESEAU
                    if (mode_reseau != 14)
                      test_wifi = WiFi.status();
                  #endif

                  /*uint8_t test_http = testHttpServerLocal();
                  test_http=0;
                  if (test_http)
                    log_erreur(Code_erreur_http_local,test_http, 0);*/

                  vTaskDelay(1000/ portTICK_PERIOD_MS);

                  if ((test_wifi!=3) || (test_goog))  // relance wifi
                  {
                    #ifndef NO_RESEAU
                      log_erreur(Code_erreur_wifi, test_wifi, test_goog);
                      if (err_wifi_repet > 2)  // au bout de 4 jours sans reseau => reset
                      {
                        // reset
                        writeLog('S', 8, 0, 0, "Boot wi");
                        ESP.restart();
                      }
                      else // premieres erreurs
                      {
                        err_wifi_repet=1;
                        server.end();  // sécuriser l'état précédent
                        if (mode_reseau!=14)
                          reConnectWifi();
                        startWebServer();
                      }
                    #endif
                  }

                  enreg_24h(0);

                  break;
                }

                case EVENT_CYCLE:
                  event_cycle();
                  break;

                  
                case EVENT_ERREUR:


                default:
                    Serial.println("evenement inconnu !");
                    break;
            } // fin du case
            if (erreur_queue) {
              log_erreur(Code_erreur_queue, erreur_queue, num_err_queue[0]);
              if (erreur_queue>1)
              {
                log_erreur(Code_erreur_queue, num_err_queue[1], num_err_queue[2]);
                if (erreur_queue>3)
                {
                  log_erreur(Code_erreur_queue, num_err_queue[3], num_err_queue[4]);
                }
              }
              erreur_queue = 0;
              for (uint8_t k=0; k<5; k++)  num_err_queue[k]=0;
            }

            // Remettre l'ESP32 en light sleep
            //Serial.println(">> Passage en light sleep...");
            //esp_light_sleep_start();
        }
    } // fin du while
}

// init des variables RTC, lors d'un cold Reset
void init_rtc_variables()
{
  cpt_cycle_batt = 1;
}

void init_ram_variables()
{
  cpt24h_batt=6;   // pour esp_chaudiere (chaque jour)
  err_Tint=0;
  err_Text=0;
  err_Heure=0;
  nb_err_reseau=0;
  Tint=20.0;
  Text=10.0;
}

void resetI2C() {
  Wire.end();              // stop I2C

  delay(10);

  Wire.begin(PIN_SDA, PIN_SCL); // Forçage des pins SDA=8, SCL=9 pour ESP32 S3 DevKit V1
  Wire.setClock(100000);  // stable (100 kHz conseillé)
}

void i2cBootRecovery() {
  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, INPUT_PULLUP);

  delay(50);

  Wire.end(); // sécurité

  // optionnel mais utile si bus bloqué
  i2cRecovery();  // ou simple reset clock SCL

  delay(20);
}

void i2cRecovery() {
  pinMode(PIN_SDA, OUTPUT);
  pinMode(PIN_SCL, OUTPUT);

  for (int i = 0; i < 9; i++) {
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
  }

  // STOP condition
  digitalWrite(PIN_SDA, LOW);
  digitalWrite(PIN_SCL, HIGH);
  digitalWrite(PIN_SDA, HIGH);

  Wire.end();
  Wire.begin(PIN_SDA, PIN_SCL); // Forçage des pins SDA=8, SCL=9 pour ESP32 S3 DevKit V1
}

void setup()
{
  
  //delay(1000);
  // Cause reset :
  resetReason0 = (uint8_t) esp_reset_reason();
  Serial.begin(115200);
  
  // Cause réveil du deep/light_sleep (undefined si pas de reveil deep/light sleep)
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause(); // 2:ext0(1pin) 3:Ext1(2pins) 7:GPIO 4:Timer
  uint8_t reveil=0;

  /* autre (Type_reveil=0, pas rtc): poweron => chargement nvs_rtc, puis comme timer
      rtc & type_reveil=1; timer  => pas de nvs, lecture temp. si stockage ram =>rien
                                    si envoi => réseau
                                    si 24h => partition log_flash_G
      type_reveil=4; ext1:BTN = nvs complet, demarrage reseau, 1 min.
      rtc & type_reveil=2; ext1:PIR = pas de nvs. si stockage flash => partition logèflash
      rtc & type_reveil=3(inconnu) => lecture_temp
      type_reveil=10 = tj_actif
      */
  if (wakeup_reason)
  {
    reveil = 3;  // inconnu
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
      reveil = 1;
    }
    else if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0   || wakeup_reason == ESP_SLEEP_WAKEUP_GPIO || wakeup_reason == ESP_SLEEP_WAKEUP_EXT1 )
    {
      reveil = 4;
      uint64_t wakeup_pins = esp_sleep_get_ext1_wakeup_status();
      Serial.printf("wakeup_pins: %016llX\n", wakeup_pins);
      if (wakeup_pins & (1ULL << PIN_REVEIL)) {
          Serial.println("Réveil par PIN1");
          force_stay_awake = true; // Réveil par bouton Reveil : on reste éveillé pour l'UART
          wake_up_time = millis() + 30000; // Prolonger si Bouton réveil est appuyé
          Serial.println("\n*** RÉVEIL PAR BOUTON : Mode configuration UART actif pour 30s ***");
      }

      if (wakeup_pins & (1ULL << PIN_REVEIL2)) {
          Serial.println("Réveil par PIN2-PIR");  // PIR
          reveil = 2;
      }
    }
  }
  else
  {
      force_stay_awake = true; // Réveil par Cold reset : on reste eveillé 50 secondes
      wake_up_time = millis() + 50000;
  }

  strncpy(resetREASON0, verbose_reset_reason(resetReason0), sizeof(resetREASON0) - 1);
  resetREASON0[sizeof(resetREASON0) - 1] = '\0'; // Garantit la terminaison null

  // determination si la RTC reste valide (apres un reveil deepsleep)
  if (rtc_magic != 0x05343211 )  
  {
    // RTC non valide = cold reset
    rtc_valid = 0;
    init_rtc_variables(); // initialisation des variables RTC, la ram normale est mise à 0
    rtc_magic = 0x05343211;
  }
  else
  {
    // RTC valide : hot reset (sortie deepsleep,watchdog, soft reset,..)
    rtc_valid = 1;
  }
  init_ram_variables(); // RTC conservé. initialisation de la ram normale (aléatoire)

  /* autre (Type_reveil=0, pas rtc): poweron => chargement nvs_rtc, puis comme timer
      rtc & type_reveil=1; timer  => pas de nvs, lecture temp. si stockage ram =>rien
                                    si envoi => réseau
                                    si 24h => partition log_flash_G
      type_reveil=4; ext1:BTN = nvs complet, demarrage reseau, 1 min.
      rtc & type_reveil=2; ext1:PIR = pas de nvs. si stockage flash => partition logèflash
      rtc & type_reveil=3(inconnu) => lecture_temp
      type_reveil=10; tj_actif
      */
  type_reveil=0;       // power_on 
  if (reveil==4)  type_reveil=4;  //BTN
  else if (rtc_valid)
  {
    if (reveil==1) type_reveil=1; // Timer 15 min
    else if (reveil==2) type_reveil=2; // PIR
    else type_reveil=3;           // inconnu
  }
  #ifdef ESP_TJ_ACTIF
    type_reveil = 10;  // power on, toujours actif
  #endif

  // Optimisation processeur : autorise le processeur à s'arrêter si inactif
  /*esp_pm_config_esp32_t pm_config = {
      .max_freq_mhz = 80,
      .min_freq_mhz = 10,
      .light_sleep_enable = true
  };
  esp_pm_configure(&pm_config);*/

  // Optimisation consommation : arrêt Bluetooth
  //btStop();

  //source_reveil=esp_sleep_get_wakeup_cause();

  // Délai de stabilisation pour éviter les conflits UART/WiFi
  //delay(500);

  #ifdef ESP_TJ_ACTIF
    esp_log_level_set("*", ESP_LOG_WARN);  // niveau minimum   - ESP_LOG_INFO  ou rien
    esp_log_level_set("HTTPClient", ESP_LOG_WARN);
    esp_log_level_set("NetworkManager", ESP_LOG_WARN);
    esp_log_level_set("NetworkClient", ESP_LOG_WARN);

    esp_log_set_vprintf(&myLogPrinter);  // redirige les log vers ma fonction

    printMemoryStatus();

    checkPartitions();
  #endif

  //  -------  Initialisation Watchdog --------------

  #ifdef WatchDog
    esp_task_wdt_deinit();
    esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMEOUT * 1000,                 // Converting ms
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Bitmask of all cores, https://github.com/espressif/esp-idf/blob/v5.2.2/examples/system/task_watchdog/main/task_watchdog_example_main.c
      .trigger_panic = true                             // Enable panic to restart ESP32
    };
    esp_err_t ESP32_WDT_ERROR = esp_task_wdt_init(&wdt_config);
    //Serial.println("WAtchdog : " + String(esp_err_to_name(ESP32_WDT_ERROR)));
    //enableLoopWDT();  // active le reset-watchdog pour loop - inutile
    esp_err_t status = esp_task_wdt_add(NULL);  //add current thread to WDT watch
    //Serial.printf("enreg wdt loop : %i\n\r", status);
    delay(1000);
    esp_task_wdt_reset();  // setup
    //Serial.println("reset watchdog debut setup");
    delay(10);
  #endif

  for (uint8_t k=0; k<5; k++)  num_err_queue[k]=0;

  cpt_securite = 0;
  #ifdef Sans_securite
    cpt_securite = 10;
  #endif


  Serial.printf("**** Initialisation - reset: %s  type_rev:%i Sleep:%i rtc:%i\n\r",resetREASON0, type_reveil, wakeup_reason, rtc_valid );
  setup_0();   //  --- valeur initiales des graphiques

  if (type_reveil>=4) // bouton ou toujours actif
  {
    // ---------  Creation des taches et des queues -------------------

    // Création de la queue sequenceur (stocke max 10 événements de type int)
    eventQueue = xQueueCreate(50, sizeof(systeme_eve_t));

    // création de la queue de reception des message uart
    QueueUart = xQueueCreate(20, sizeof(UartMessage_t));

    // Création de la tâche FreeRTOS
    //xTaskCreatePinnedToCore (taskHandler, "TaskHandler", 4096, NULL, 1, &taskHandle,0);
    xTaskCreate (taskHandler, "TaskHandler", 8192, NULL, 1, &taskHandle);

    xTaskCreate(uartTask, "UARTTask", 8192, NULL, 3, NULL);  // priorité 3 plus élevée pour éviter les interruptions WiFi
  
    #ifdef STM32
      QueueUart1 = xQueueCreate(20, sizeof(UartMessage_t));
      xTaskCreate(uart1Task, "UART1Task", 8192, NULL, 3, NULL);  // priorité 3 plus élevée
    #endif

    // ------  Configuration des PIN sorties       -------------------------------

    pinMode(PIN_OUT0, OUTPUT);
    // Configurer l'interruption GPIO sur GPIO 18 (ex: bouton poussoir)
    //pinMode(18, INPUT_PULLUP);
    //attachInterrupt(digitalPinToInterrupt(18), onGPIO, FALLING);
      pinMode(BTN_PIN[0], INPUT_PULLUP);
      pinMode(BTN_PIN[1], INPUT_PULLUP);
      /*pinMode(BTN_PIN[1], INPUT_PULLUP);
      pinMode(BTN_PIN[2], INPUT_PULLUP);
      pinMode(BTN_PIN[3], INPUT_PULLUP);
      pinMode(BTN_PIN[4], INPUT_PULLUP);*/
      for (int i = 0; i < BTN_COUNT; i++) {
          attachInterrupt(digitalPinToInterrupt(BTN_PIN[i]), onButtonInterrupt, CHANGE);
      }
      //pinMode(PIN_REVEIL, INPUT_PULLDOWN); // Bouton de réveil / Wifi_AP au démarrage
      //pinMode(PIN_REVEIL2, INPUT_PULLDOWN); // Bouton de réveil : Detecteur



    // ------   Initialisation MODBUS       --------------

    // https://www.modbustools.com/download.html
    #ifdef MODBUS
    //gpio_reset_pin((gpio_num_t)MAX485_RE_NEG);
    //gpio_reset_pin((gpio_num_t)MAX485_DE);
      pinMode(MAX485_RE_NEG, OUTPUT);
      pinMode(MAX485_DE, OUTPUT);
      // Init in receive mode
      digitalWrite(MAX485_RE_NEG, 0);
      digitalWrite(MAX485_DE, 0);

      // Transmission mode: MODBUS-RTU, Baud rate: 9600bps, Data bits: 8, Stop bit: 1, Check bit: no
      Serial1.begin(MODBUS_SPEED, MODBUS_PARITY, PIN_RXModbus, PIN_TXModbus);  //Baudrate: 19200  Data bits: 8  Stop bits: 2  Parity: 0 (NONE)


      // Modbus:Slave address: the factory default is 01H (set according to the need, 00H to FCH)
      node.begin(1, Serial1);
      node.preTransmission(preTransmission);
      node.postTransmission(postTransmission);
    #endif  // MODBUS

    // ----- init port Serial 1  ------------

    #ifdef STM32
      Serial1.begin(115200, SERIAL_8N1, PIN_RXSTM, PIN_TXSTM); // UART1 = liaison avec l'autre ESP32
      //esp_sleep_enable_uart_wakeup(UART_NUM_1);  
    #endif
  }

  // ------  NVS Eeprom  ---------------------

  if ((type_reveil>=4) || (!type_reveil))
  {
    preferences_nvs.begin("NVS_App", false);

    setup_nvs_rtc();

    // Initialisation variable skip graph
    skip_graph = preferences_nvs.getUChar("Skip", 0);
    if ((!skip_graph) || (skip_graph > 50))  // entre 1 et 50
    {
      Serial.println("Raz skip graph : valeur par defaut:2");
      skip_graph = 12;  // 1 valeur sur 12
      preferences_nvs.putUChar("Skip", skip_graph);
    }


    if (type_reveil>=4)
    {
      // lecture nb de reset en nvs
      nb_reset = preferences_nvs.getUShort("nb_reset", 0);
      nb_reset++;
      preferences_nvs.putUShort("nb_reset", nb_reset);

      // mode reseau
      mode_reseau = preferences_nvs.getUChar("reseau", 0);  // 11:wifi_AP_usine  12:wifi_AP 13:wifi_routeur  14:Ethernet filaire  autre:wifi_routeur
      if ((mode_reseau<11) || (mode_reseau>14)) {  //init de nvs
        mode_reseau=11;
        preferences_nvs.putUChar("reseau", 11);
        Serial.printf("Raz mode reseau Wifi AP : val par defaut 11\n\r");
      }
      else Serial.printf("mode reseau : %i\n\r", mode_reseau);


      #ifdef NO_RESEAU
        mode_reseau=0;
      #endif

      // delai écoute websocket
      DelaiWebsocket = preferences_nvs.getUChar("DelWS", 0);  // en secondes
      if ((!DelaiWebsocket) || (DelaiWebsocket>30)) { 
        DelaiWebsocket=1;
        preferences_nvs.putUChar("DelWS", 1);
        Serial.printf("New Delai ecoute websocket : val par defaut :1\n\r");
      }
      else Serial.printf("Delai ecoute websocket : %i\n\r", DelaiWebsocket);


      // lecture de détail_log
      log_detail = preferences_nvs.getUChar("LogD", 100);
      if (log_detail > 4)
      {
        log_detail = 0; 
        preferences_nvs.putUChar("LogD", log_detail);
        Serial.printf("New : Détail_log : %i \n\r", log_detail);
      }
      //else
      //  Serial.printf("Détail log : %i \n\r", log_detail);

      uint8_t err_ip=0;
      // routeur : SSID & mot de passe
      String storedString = preferences_nvs.getString("Rout", "");
      if ((storedString.length() < 15) && (storedString.length())) {
        storedString.toCharArray(nom_routeur, sizeof(nom_routeur));
        Serial.printf("nom_routeur : %s\n\r", nom_routeur);
      }

      String storedString2 = preferences_nvs.getString("Mdp", "");
      if ((storedString.length()) && (storedString2.length() < 15)) {
        storedString2.toCharArray(mdp_routeur, sizeof(mdp_routeur));
      }
      if (!nom_routeur[0]) {
        err_ip=1;
        Serial.println("pas de routeur");
      }
      Serial.printf("routeur:%s  \n\r", nom_routeur);

      // adresse IP
      uint32_t storedIP = preferences_nvs.getULong("ipAdd", 0);
      local_ip =  IPAddress(storedIP);
      Serial.printf("Adresse IP : %s\n\r", local_ip.toString().c_str());

      storedIP = preferences_nvs.getULong("ipGat", 0);
      gateway =  IPAddress(storedIP);
      Serial.printf("Gateway IP : %s\n\r", gateway.toString().c_str());

      storedIP = preferences_nvs.getULong("ipSub", 0);
      subnet = IPAddress(storedIP);

      storedIP = preferences_nvs.getULong("ipDNS", 0);
      primaryDNS = IPAddress(storedIP);

      storedIP = preferences_nvs.getULong("ipDNS2", 0);
      secondaryDNS = IPAddress(storedIP);

      if ((!local_ip[0]) || (!gateway[0]))  err_ip=1;
      if ((!subnet[0]) || (!primaryDNS[0]) || (!secondaryDNS[0]))  err_ip=1;

      if (err_ip) { 
        mode_reseau=11;  // si une info manquante => activation en Access_point
        Serial.printf("Err=>activ access point %d %d %d %d %d\n\r", local_ip[0], gateway[0], subnet[0], primaryDNS[0], secondaryDNS[0]);
      }
      #ifdef Wifi_AP
        mode_reseau=11;
      #endif

      #ifndef Sans_websocket
        //lecture websocket ws://webcam.hd.free.fr:8081
        // lecture id websocket
        websocket_on = preferences_nvs.getUChar("WSOn", 0);
        if (websocket_on!=1 && websocket_on!=2)
        {
          websocket_on=1;
          preferences_nvs.putUChar("WSOn", 1);
          Serial.printf("New websocket OFF : %i\n\r", websocket_on);
        }
        else
          Serial.printf("websocket ON : %i\n\r", websocket_on);

        //if (websocket_on==2)
        //{
          storedString = preferences_nvs.getString("WSock", "");
          if ((storedString.length() < 40) && (storedString.length() > 3)) {
            storedString.toCharArray(ip_websocket, sizeof(ip_websocket));
            Serial.printf("websocket : %s\n\r", ip_websocket);
          }
          // lecture id websocket
          id_websocket = preferences_nvs.getUChar("WSId", 0);
          if (!id_websocket || id_websocket>=10)
          {
            id_websocket=9;
            preferences_nvs.putUChar("WSId", id_websocket);
            Serial.printf("New Id websocket : %i\n\r", id_websocket);
          }
          else
            Serial.printf("Id websocket : %i\n\r", id_websocket);
      #endif // fin sans_websocket



      setup_nvs();  // NVS appli
    }

    // lecture du Bouton BTN0 : si actif pendant 1 secondes => Wifi_AP
    int buttonState = digitalRead(BTN_PIN[0]);
    if (!buttonState)
    {
      delay(1000);
      Serial.println("Test appui long BTN0 pour Wifi_AP");
      buttonState = digitalRead(BTN_PIN[0]);
      if (!buttonState)
      {
        Serial.println("Appui BTN0 => Wifi_AP");
        mode_reseau=11;
      }
    }
  }

  setup_1();  // --------------   initialisation sonde temperature------------


  // -------------  Capteur/Detecteur : stockage ou envoie infos à la gateway -------------------

  if (type_reveil ==2)   // Detection PIR
  {
    detection_pir();
    uint64_t sleep_time = (uint64_t)periode_cycle * 60 * 1000000;
    if (mode_rapide==12)
    sleep_time = (uint64_t)periode_cycle * 1000000;
    passage_deep_sleep( sleep_time); // 30ULL * 1000000ULL);
  }
  else 
  {
    if (type_reveil == 1)  event_cycle(); // réveil par timer
    else 
    {
      envoi_temp_hygro();
    }
    uint8_t etat_BTN0 = digitalRead(BTN_PIN[0]);  // pull_up => 1, appui => 0
    if ((type_reveil != 4) || (etat_BTN0))  // reveil par BTN0 encore appuyé => pas sleep
    {
      uint64_t sleep_time = (uint64_t)periode_cycle * 60 * 1000000;
      if (mode_rapide==12)
      sleep_time = (uint64_t)periode_cycle * 1000000;
      passage_deep_sleep( sleep_time); // 30ULL * 1000000ULL);
    }
  }



  // -------------- partition "log_flash" custom  pour Write-log -------------------

  logPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x99, "log_flash");


  if (!logPartition) {
    Serial.println("Partition 'log_flash' non trouvée !");
    log_err=1;
  }
  else
  {
    log_err=0; // ok
    Serial.println("Partition 'log_flash' trouvée.");
  
    delay(500 + random(1, 1001) );
    writeLog('R', resetReason0, rtc_valid, wakeup_reason, "Reset");
    delay(500);

    readLastLogsBinary((uint8_t*)buffer_dmp, 10);  
    delay(200);
  }

  // Recherche de la partition "log_flashG" custom
  logPartitionG = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x98, "log_flashG");

  if (!logPartitionG) {
    Serial.println("Partition 'log_flash_G' non trouvée !");
    log_errG=1;
  }
  else
  {
    log_errG=0;
    Serial.println("Partition 'log_flash_G' trouvée.");
  }

  // -------------   Configuration des timers FreeRTOS (max 49 jours)   --------------------

  // Timer à l'initialisation pour masquer 10 secondes les envois au transmetteur et init heure, puis 30 sec
  xTimer_Init= xTimerCreate ("Init", (uint32_t)attente_init*(1000/portTICK_PERIOD_MS), pdTRUE, (void *) 0, vTimerInitCallback);
  if (xTimer_Init == NULL)  Serial.println("Erreur : timer xTimer_Init non créé !");

  // Timer chaque 3 minutes pour test wifi
  xTimer_3min= xTimerCreate ("3min", (uint32_t)3*60*(1000/portTICK_PERIOD_MS), pdTRUE, (void *) 0, vTimer3minCallback);
  if (xTimer_3min == NULL)  Serial.println("Erreur : timer xTimer_3min non créé !");

  // Timer chaque 24heures
  xTimer_24H= xTimerCreate ("24H", (uint32_t)24*60*60*(1000/portTICK_PERIOD_MS), pdTRUE, (void *) 0, vTimer24HCallback);
  if (xTimer_24H == NULL)  Serial.println("Erreur : timer xTimer_24h non créé !");

  // Timer pour éviter les faux rebonds des boutons
  debounceTimer = xTimerCreate("debounce", pdMS_TO_TICKS(DEBOUNCE_INTERVAL), pdFALSE, 0, debounceCallback);
  if (debounceTimer == NULL)  Serial.println("Erreur : timer debounceTimer non créé !");

  //xTimerStart(debounceTimer, 100);  // lecture initiale pour lire l'état des boutons


  // Timer cycle : lecture temp exterieur par internet
  uint16_t perio = periode_cycle*60;  // minutes -> secondes
  if (mode_rapide==12) perio = periode_cycle;   // en secondes
  xTimer_Cycle= xTimerCreate ("Cycle", (uint32_t)perio*(1000/portTICK_PERIOD_MS), pdTRUE, (void *) 0, vTimerCycleCallback);
  if (xTimer_Cycle == NULL)  Serial.println("Erreur : timer xTimer_cycle non créé !");


  // Timer de Délai pour fin de modifs autorisée (securité) : 15 minutes
  xTimer_Securite= xTimerCreate ("Securite",15*60*(1000/portTICK_PERIOD_MS), pdFALSE, (void *) 0, vTimerSecuriteCallback);
  if (xTimer_Securite == NULL)  Serial.println("Erreur : timer xTimer_Securite non créé !");

  // Timer de Délai pour websocket
  xTimer_Websocket= xTimerCreate ("Websocket", (uint32_t)DelaiWebsocket*(1000/portTICK_PERIOD_MS), pdTRUE, (void *) 0, vTimerWebsocketCallback);
  if (xTimer_Websocket == NULL)  Serial.println("Erreur : timer xTimer_Websocket non créé !");

  // Timer de Délai pour watchdog : 13 sec
  xTimer_Watchdog= xTimerCreate ("Watchdog", (uint32_t)WDT_TIMEOUT*(300/portTICK_PERIOD_MS), pdTRUE, (void *) 0, vTimerWatchdogCallback);
  if (xTimer_Watchdog == NULL)  Serial.println("Erreur : timer xTimer_Watchdog non créé !");



  #ifdef WatchDog
    esp_task_wdt_reset();
    Serial.println("reset watchdog milieu setup");
    delay(10);  // Important de mettre au moins 1ms
    xTimerStart(xTimer_Watchdog,100);
  #endif

  delay(1000); // Attente 4 sec pour que les boutons se stabilisent

  xTimerStart(xTimer_Init,100);
  xTimerStart(xTimer_24H,100);
  xTimerStart(xTimer_Cycle,100);
  //xTimerStart(xTimer_Compresseur,100);

  delay(1000); // Attente 4 sec pour que les boutons se stabilisent

  // Reset du watchdog avant de démarrer le réseau
  #ifdef WatchDog
    esp_task_wdt_reset();
  #endif
  //vTaskDelay(4000 / portTICK_PERIOD_MS); // Attente 4 sec pour que les boutons se stabilisent

  if (websocket_on==2)
  {
    xTimerStart(xTimer_Websocket,100);    
  }



  // -------------------- demarrage du reseau  ------------------

  Serial.printf("mode reseau avant wifi:%i\n\r", mode_reseau);

  #ifndef NO_RESEAU 

  #ifdef MODE_Wifi
    #ifdef DEBUG
      mode_reseau=13;  // mode station en Debug
    #endif

    Serial.printf("reseau:%i\n\r", mode_reseau);

    if ((mode_reseau==11) || (mode_reseau==12)) // mode Access Point
    {
      WiFi.mode(WIFI_AP_STA);
      Serial.printf("lancement mode access point:%i au 192.168.254.1\n\r", mode_reseau);
      WiFi.softAP(ssid_AP, password_AP);
      WiFi.softAPConfig(local_ip_AP, gateway_AP, subnet_AP);
      Serial.println(" Starting AP Wifi Web server " + String(ARDUINO_BOARD));
      etat_connect_ethernet = 1;
    }
    else  // mode=13 : Station
    {
      xTimerStart(xTimer_3min,100);

      //WiFi.mode(WIFI_STA);
      #ifdef DEBUG
        local_ip = Slocal_ip;
        gateway = Sgateway;
        subnet = Ssubnet;
        primaryDNS = SprimaryDNS;
        secondaryDNS = SsecondaryDNS;
        
        strncpy(nom_routeur, ssid, sizeof(nom_routeur) - 1);
        nom_routeur[sizeof(nom_routeur) - 1] = '\0'; // Sécurisation de la terminaison
        strncpy(mdp_routeur, password, sizeof(mdp_routeur) - 1);
        mdp_routeur[sizeof(mdp_routeur) - 1] = '\0'; // Sécurisation de la terminaison
      #endif
      //if (!WiFi.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS))
      //  Serial.println("Wifi STA Failed to configure");
      
      // Protection UART avant connexion WiFi
      //protectUARTDuringWiFi();
      
      uint8_t wifiResult = connectWiFiWithDiagnostic();
      
      // Protection UART après connexion WiFi
      //protectUARTDuringWiFi();
      
      if (wifiResult == 0) {
        etat_connect_ethernet = 2;
        eth_connected = true;
      }
      if (WiFi.status() == WL_CONNECTED) {
        //Serial.println("Wifi OK");
        // Optimisation consommation : activation du Modem Sleep
        //WiFi.setSleep(true);
      }
      else Serial.println("Wifi pas ok");
    }

  #else  // WT32 Ethernet
    xTimerStart(xTimer_3min,100);
    Serial.print("\nStarting AdvancedWebServer on " + String(ARDUINO_BOARD));
    //Serial.println(" with " + String(SHIELD_TYPE));

    //Serial.println(ASYNC_WEBSERVER_WT32_ETH01_VERSION);

    // To be called before ETH.begin()
    Network.onEvent(onEvent);
    ETH.begin();
    ETH.config(myIP, myGW, mySN, myDNS);

    delay(1000);

    if (eth_connected) {
      Serial.println("Connecté Ethernet");
      etat_connect_ethernet = 2;
    } else
      Serial.println("pas connecté Ethernet");

    Serial.print(F("HTTP EthernetWebServer is @ IP : "));
    Serial.println(ETH.localIP());

  #endif // wifi

  // Configuration du serveur NTP
    configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", ntpServer);
    //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    setupRoutes();
    server.begin();


  #endif // No_reseau

  printMemoryStatus();
  delay(100);


  // ------- changement de frequence CPU -------------------

  uint16_t Cpu_freq = getCpuFrequencyMhz();
  setCpuFrequencyMhz(80);
  Serial.printf("CPU Freq: avant:%i  apres:%u\n", Cpu_freq, (unsigned int)getCpuFrequencyMhz());



  // Redémarrage final du watchdog après la configuration réseau
  #ifdef WatchDog
    esp_task_wdt_reset();
    Serial.println("reset watchdog après configuration réseau");
  #endif


  printMemoryStatus();

  setup_2();  // Esp_now

  // ------------  Configuration OTA -----------------

  #ifdef OTA
    ArduinoOTA.setHostname("ESP32_Tempa");
    ArduinoOTA.setPassword("Corail2025");

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
      else type = "filesystem";
      Serial.println("Mise à jour OTA: " + type);
      //WiFi.setSleep(false);  // accelere l'ota
    });

    ArduinoOTA.onEnd([]() {
      Serial.println("\nFin");
      //WiFi.setSleep(true); 

    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progression: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Erreur[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    //WiFi.setTxPower(WIFI_POWER_19_5dBm); // puissance max
    ArduinoOTA.begin();
    Serial.println("OTA prêt");
  #endif  // fin OTA

 // 🔥 Fenêtre OTA de secours
  Serial.println("Fenêtre OTA 5 secondes...");
  unsigned long start = millis();
  while (millis() - start < 5000) {
    ArduinoOTA.handle();
    delay(10);
  }

  //WiFi.setSleep(true);

  Serial.println("fin setup:");

  #ifdef WATCHDOG
    esp_task_wdt_delete(NULL); // desinscription de la tache setup/loop de la surveillance watchdog : permet d'éviter le reset_wdt dans la tache loop
  #endif

}

void heartBeatPrint()
{
  static int num = 1;

  Serial.print(F("."));

  if (num == 80) {
    Serial.println();
    num = 1;
  } else if (num++ % 10 == 0) {
    Serial.print(F(" "));
  }
}




//************ lecture de l'heure sur ESP32 ************************************
void lectureHeure() {
  if (!getLocalTime(&timeinfo,2000)) {
    Serial.println("erreur lecture date_heure");
    return; // ou gestion d'erreur
  }

  //time_t now = time(nullptr);
  //timeinfo = localtime(&now);
  hr = timeinfo.tm_hour;
  mn = timeinfo.tm_min;
  sd = timeinfo.tm_sec;
  snprintf(heure_string, sizeof(heure_string), "\"%.2d:%.2d:%.2d\"", hr, mn, sd);
  heure = hr + (float)mn / 60 + (float)sd / 3600;
  int nb_annees = timeinfo.tm_year + 1900 - 2020;
  date_ac = timeinfo.tm_yday + 365 * nb_annees + (nb_annees + 3) / 4;  // Nombre de jours depuis le 1/1/2020
  //Serial.println(heure_string);
  last_remote_heure_time = millis();
}




// Activation du Light Sleep
//Serial.println("Passage en Light Sleep...");
//esp_light_sleep_start();


void sauve_nvs_8bytes(uint8_t *tableau, char *titre) {
  Serial.print("sauve_nvs_tab:");
  if (!tableau) return;
  Serial.println(*tableau);
  // transfert dans un tableau temporaire
  // compte longueur :
  uint8_t sizeN;
  char buffer[NB_Val_Graph];  // prepare a buffer for the data
  // sonde autonome :
  for (sizeN = 0; sizeN < NB_Val_Graph; sizeN++) {
    if (*(tableau + sizeN))
      buffer[sizeN] = *(tableau + sizeN);
    else
      break;
  }
  preferences_nvs.putBytes(titre, buffer, sizeN);
}

void sauve_nvs_16bytes(uint16_t *tableau, char *titre) {
  Serial.print("sauve_nvs_tab:");
  if (!tableau) return;
  Serial.println(*tableau);
  // transfert dans un tableau temporaire
  // compte longueur :
  uint8_t sizeN;
  char buffer[NB_Val_Graph * 2];  // prepare a buffer for the data
  // sonde autonome :
  for (sizeN = 0; sizeN < NB_Val_Graph; sizeN++) {
    if (*(tableau + sizeN)) {
      buffer[sizeN * 2] = (*(tableau + sizeN)) >> 8;
      buffer[sizeN * 2 + 1] = (*(tableau + sizeN)) & 0xFF;
      Serial.print("sauve ");
      Serial.print(sizeN, DEC);
      Serial.print(":");
      Serial.println(*(tableau + sizeN), DEC);
    } else
      break;
  }
  preferences_nvs.putBytes(titre, buffer, sizeN * 2);
}

// fonction à robustifier car le buffer[BufLen] est affecté dans la pile ce qui peut poser problème si c'est supérieur à 200 octets
void load_nvs_8bytes(uint8_t *tableau, char *titre) {
  size_t BufLen = preferences_nvs.getBytesLength(titre);
  Serial.print(titre);
  Serial.print(":Size:");
  Serial.println(BufLen);
  if ((BufLen) && (BufLen <= NB_Val_Graph))  // si dif 0  => load tableau blob sonde 0
  {
    char buffer[BufLen];  // prepare a buffer for the data
    preferences_nvs.getBytes(titre, buffer, BufLen);
    // transfert dans tableau
    uint8_t i;
    for (i = 0; i < (BufLen); i++)
      *(tableau + i) = buffer[i];
  }
}

void load_nvs_16bytes(uint16_t *tableau, char *titre) {
  size_t BufLen = preferences_nvs.getBytesLength(titre);
  /*Serial.print(titre);
  Serial.print(":Size:");
  Serial.println(BufLen);*/
  if ((BufLen) && (BufLen <= NB_Val_Graph * 2))  // si dif 0  => load tableau blob sonde 0
  {
    char buffer[BufLen];  // prepare a buffer for the data
    preferences_nvs.getBytes(titre, buffer, BufLen);
    // transfert dans tableau
    uint8_t i;
    for (i = 0; i < (BufLen / 2); i++) {
      *(tableau + 2 * i) = ((uint16_t)(buffer[2 * i]) << 8) + buffer[2 * i + 1];
      Serial.print("buff ");
      Serial.print(i, DEC);
      Serial.print(":");
      Serial.println(*(tableau + 2 * i), DEC);
    }
  }
}

void log_erreur(uint8_t code, uint8_t valeur, uint8_t val2)  // Code:1:Tint, 2:Text, 3:TPac
{
  Serial.printf("erreur:%i:%i\n\r", code, valeur);
  uint8_t i;
  for (i = Nb_erreurs - 1; i; i--) {
    erreur_code[i] = erreur_code[i - 1];
    erreur_valeur[i] = erreur_valeur[i - 1];
    erreur_val2[i] = erreur_val2[i - 1];
    erreur_jour[i] = erreur_jour[i - 1];
    erreur_heure[i] = erreur_heure[i - 1];
    erreur_minute[i] = erreur_minute[i - 1];
  }
  erreur_code[0] = code;
  erreur_valeur[0] = valeur;
  erreur_val2[0] = val2;
  getLocalTime(&timeinfo, 400);
  delay(50);
  erreur_jour[0] = timeinfo.tm_mday;
  erreur_heure[0] = timeinfo.tm_hour;
  erreur_minute[0] = timeinfo.tm_min;

  Serial.printf("date de l'erreur:%i:%i:%i\n\r", timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
  delay(100);
  
  if (code >3)  writeLog('E', code, valeur, val2, "Err"); // pas de write pour Tint, Text, Heure

}

void printHexByte(byte b) {
  Serial.print((b >> 4) & 0xF, HEX);
  Serial.print(b & 0xF, HEX);
  Serial.print(' ');
}

uint8_t read_modbus(uint8_t param, int16_t *valeur)  // lecture des input register - commence à 0x3000 - 2 octets
{
  
  uint8_t result = 0;  // ok
  *valeur=12;

  //Serial.printf(".");
  //Serial.printf("lectmodbus param:%i\n\r", param);
  //vTaskDelay(100/ portTICK_PERIOD_MS);
#ifdef MODBUS
  uint8_t lg = 1;
  ModReadBuffer[0] = 0;
  //result = node.readHoldingRegisters( param, lg); 224=ku8MBInvalidSlaveID
  result = node.readInputRegisters(param, lg);
  //Serial.printf("read result = %i param=%i\r\n", result, param);
  vTaskDelay(30/ portTICK_PERIOD_MS);

  // do something with data if read is successful
  if (result == node.ku8MBSuccess) {
    for (uint8_t j = 0; j < lg; j++) {
      ModReadBuffer[j] = node.getResponseBuffer(j);
      //printHexByte(ModReadBuffer[j]);
    }
    //Serial.print("\n\r");
  vTaskDelay(30/ portTICK_PERIOD_MS);
  }
  *valeur = ModReadBuffer[0];

#endif //MODBUS
  return result;
}

/* Format réponse : 
  B0 : adresse node
  B1 : code fonction
  B2 : longueur en octets
  B3 à BX : données
  2 Bytes:CRC
  */

uint8_t save_modbus(uint16_t param, uint16_t valeur) {
  // registre 0, valeur:2
  uint8_t result = 0;
#ifdef MODBUS
  result = node.writeSingleRegister(param, valeur);
  Serial.printf("write result = %i par=%i val=%i\r\n\r", result, param, valeur);

#endif
  return result;
}

#ifdef MODBUS
void preTransmission() {
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission() {
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}
#endif


void onMessageCallback(WebsocketsMessage message) {
  Serial.println("📩 Message reçu du WebSocket VPS : " + message.data());

  isWebSocketConnected = true;

  // Créer un document JSON pour analyser le message
  JsonDocument doc;


  // Parser le message JSON
  DeserializationError error = deserializeJson(doc, message.data());

  // Vérifier si l'analyse a réussi
  if (error) {
    Serial.print("Erreur de parsing JSON:");
    Serial.println(error.c_str());
    return;
  }

  const char* action = doc["action"];
  const char* reqId = doc["_reqId"];  // Récupère l'ID de la requête
  Serial.print("Action : ");
  Serial.println(action);

  uint8_t res = 1;

  if (!strncmp(action, "pong",5)) {
    // pong recu
  }

  else if (!strncmp(action, "favicon",8)) {
    // demande favicon
    String encoded = base64::encode(favicon, sizeof(favicon));
    JsonDocument responseDoc;
    responseDoc["_reqId"] = reqId;
    responseDoc["response"] = encoded;
    String out;
    serializeJson(responseDoc, out);
    wsClient.send(out);
  }

  else if (!strncmp(action, "verif",6)) {
    JsonDocument responseDoc;
    responseDoc["_reqId"] = reqId;
    responseDoc["response"] = "OK";
    String out;
    serializeJson(responseDoc, out);
    wsClient.send(out);
  }

  else if (!strncmp(action, "GetPage",8))  {
    // Récupération de la page web
    int totalLength = strlen(index_html);
    int chunk_size = 300;
    for (int i = 0; i < totalLength;) {
      String chunk;
      int endIdx = i + chunk_size;
      if (endIdx > totalLength) endIdx = totalLength;
      while ((endIdx > i) &&  ((index_html[endIdx] & 0xC0) == 0x80)) endIdx--; // UTF-8

      if (endIdx > i) {
        if (i == 0)
          chunk = "HTM1" + String(index_html).substring(i, endIdx);
        else
          chunk = "HTML" + String(index_html).substring(i, endIdx);

        wsClient.send(chunk);
        i = endIdx;
        delay(10);
      }
    }
    wsClient.send("ENDP");  // Signal de fin
  }

  else if (!strncmp(action, "GetLogs",8)) {
    // Renvoi des logs binaires encodés base64
    int count = 10; // valeur par défaut
    //count = doc["count"].as<int>();
    if (doc["count"]) {
    //if (doc["count"].is<int>()) {
        count = doc["count"].as<int>();
    }    
    int maxLogs = MAX_DUMP / LOG_ENTRY_SIZE -3;
    if (count <= 0 || count > maxLogs) count = maxLogs;
    int nbLogs = readLastLogsBinary((uint8_t*)buffer_dmp, count);
    int totalSize = nbLogs * LOG_ENTRY_SIZE;

    String encoded = base64::encode((uint8_t*)buffer_dmp, totalSize);
    JsonDocument responseDoc;
    responseDoc["_reqId"] = reqId;
    responseDoc["response"] = encoded;
    String out;
    serializeJson(responseDoc, out);
    wsClient.send(out);
  }

  else if (!strncmp(action, "status",7)) {
    printMemoryStatus();
    uint8_t type = doc["type"];
    if (type) type = 1; else type = 0;

    requete_status(buffer_dmp, 1, type);
    delay(100);

    int totalLength = strlen(buffer_dmp);
    for (int i = 0; i < totalLength; i += 300) {
      String chunk;
      if (i)
        chunk = "STAT" + String(buffer_dmp).substring(i, i + 300);
      else
        chunk = "STA1" + String(buffer_dmp).substring(i, i + 300);

      wsClient.send(chunk);
      delay(20);
    }
    wsClient.send("ENDS");  // Signal de fin
  }

  else if ((!strncmp(action, "Set",4)) || (!strncmp(action, "Get",4))) {
    uint8_t type = doc["type"];
    const char* varStr = doc["reg"];
    const char* valStr = doc["val"];

    Serial.print("type: ");
    Serial.println(type);
    Serial.print("Var : ");
    Serial.println(varStr);
    Serial.print("Valeur : ");
    Serial.println(valStr);

    if (strncmp(action, "Set",4) == 0)
    {
      if (varStr != nullptr && valStr != nullptr)   // Gestion d'erreur : chaîne manquante dans JSON
      {
        res = requete_Set(type, varStr, valStr);

        uint8_t res2 = 1;
        float valeur = 0;
        char valeur_S[50] = "";

        if (type < 4) res2 = requete_Get(type, varStr, &valeur);
        else res2 = requete_Get_String(type, varStr, valeur_S);

        JsonDocument responseDoc;
        responseDoc["_reqId"] = reqId;

        if (!res2) {
          if (type == 1) responseDoc["reg"] = varStr;
          if (type == 2) responseDoc["reg"] = "get_reg_value";
          if (type == 3) responseDoc["reg"] = "get-regM-value";
          if (type == 4) responseDoc["reg"] = "get-regT-value";

          if (type < 4) responseDoc["val"] = valeur;
          else responseDoc["val"] = valeur_S;
        } else {
          responseDoc["error"] = "lecture après set impossible";
        }

        String out;
        serializeJson(responseDoc, out);
        Serial.println(out);
        wsClient.send(out);
      }
    }

    if (!strncmp(action, "Get",4)) {
      float valeur = 0;
      char valeur_S[50] = "";

      if (type < 4) res = requete_Get(type, varStr, &valeur);
      else res = requete_Get_String(type, varStr, valeur_S);

      JsonDocument responseDoc;
      responseDoc["_reqId"] = reqId;

      if (!res) {
        if (type == 1) responseDoc["reg"] = varStr;
        if (type == 2) responseDoc["reg"] = "get-reg-value";
        if (type == 3) responseDoc["reg"] = "get-regM-value";
        if (type == 4) responseDoc["reg"] = "get-regT-value";

        if (type < 4) responseDoc["val"] = valeur;
        else responseDoc["val"] = valeur_S;
      }
      else {
        responseDoc["error"] = "erreur lecture";
      }

      String out;
      serializeJson(responseDoc, out);
      Serial.println(out);
      wsClient.send(out);
    }
  }
}

// Fonction pour connecter l'ESP32 au WebSocket VPS
void connectWebSocket()
{
  uint8_t internet_available=0;
  #ifdef MODE_Wifi 
    if (WiFi.status() == WL_CONNECTED)  internet_available=1;
  #else
    if (ETH.linkUp()) internet_available=1;
  #endif

  if (internet_available)
  {
    Serial.println("🔗 Connexion au WebSocket du VPS...");
    wsClient.close();
    vTaskDelay(1000/ portTICK_PERIOD_MS);
    if (wsClient.connect(ip_websocket)) {
      Serial.println("✅ Connecté au WebSocket !");
      isWebSocketConnected = true;
      wsClient.send("ID:" + String(id_websocket));
      wsClient.onMessage(onMessageCallback);  // Définition du callback pour les messages reçus
    } else {
      Serial.println("❌ Échec de connexion WebSocket !");
    }
  }
}


uint8_t requete_Set(uint8_t type, const char* param, const char* valStr)
{
  uint8_t res = 1;
  uint8_t res2=1;
  int val = atoi(valStr);
  float valf = atof(valStr);
  uint8_t paramV = atoi(param);

  if (type==2)  // set registre
  {
    res = requete_SetReg(paramV, valf);
  }
  if (type==3)  // set registre Modbus
  {
  }
  if (type==4)  // set registre texte
  {
    res = requete_Set_String(paramV, valStr);     
  }
  if (type==5)  // Action
  {
    res = requete_Set_Action(param, valStr);     
  }
  if (type==1)  // set variable
  {
    if (cpt_securite) {
      if (strcmp(param, "minute_m") == 0) {
        //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        getLocalTime(&timeinfo,400);
        //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        timeinfo.tm_min = val;
        //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        time_t sec = mktime(&timeinfo);  // make time_t
        //localtime(&sec); // set time
        timeval tv;
        tv.tv_sec = sec;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
        getLocalTime(&timeinfo,400);
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        res = 0;
      }

      else if (strcmp(param, "heure_m") == 0) {
        getLocalTime(&timeinfo,400);
        timeinfo.tm_hour = val - 1;
        const time_t sec = mktime(&timeinfo);  // make time_t
        timeval tv;
        tv.tv_sec = sec;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
        getLocalTime(&timeinfo,400);
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        res = 0;
      }

    }

    if (strcmp(param, "coche_secu") == 0)  // pour activation modif
    {
      res = 0;
      cpt_securite = 0;
      if (val)  // pre-activ_secu
        temps_activ_secu = millis();
    }
    if (strcmp(param, "code_secu") == 0) {
      res = 0;
      unsigned long temps = millis() - temps_activ_secu;
      //#ifdef DEBUG
      Serial.println(temps);
      //#endif
      cpt_securite = 0;
      if ((strcmp(valStr, "Temp2025") == 0) && (temps < 14000) && (temps > 3000))  // entre 3sec et 14sec
      {
        #ifndef Sans_securite
          xTimerStop(xTimer_Securite,100);
          xTimerChangePeriod(xTimer_Securite,(uint32_t)15*60*(1000/portTICK_PERIOD_MS),100);
          xTimerStart(xTimer_Securite,100);  // permet de couper la securite au bout de 2 minutes
        #endif
        cpt_securite = 1;
        Serial.println("Secu actif");
      }
      temps_activ_secu = 0;
    }
    if (cpt_securite)
      res2 = requete_Set_appli (param, valf);
  }
  Serial.printf("res:%i res2:%i\n\r", res, res2);
  return (res+res2-1);
}

uint8_t requete_Get(uint8_t type, const char* var, float *valeur)
{
  uint8_t res=1;
  uint8_t res2=1;

  if (type==2) // get registre
  {
    int paramV = atoi(var);
    res = requete_GetReg(paramV, valeur);
    //Serial.println(*valeur);
    //Serial.println(res);
  }
  if (type==3)  // get registre modbus
  {
  }
  if (type==1)  // get variable
  {
      if (strncmp(var, "codeR_secu",11) == 0) {
        res = 0;
        if (cpt_securite)  *valeur=1.0f;
        else *valeur=0.0f;
      }
      res2 = requete_Get_appli(var, valeur);
  }
  //return (res+res2-1);
  return (res == 0 || res2 == 0) ? 0 : 1;
}

// type 4
uint8_t requete_Get_String (uint8_t type, String var, char *valeur) 
{
  int paramV = var.toInt();

  uint8_t res = 1;
  uint8_t res2 = 1;
  uint8_t len = 50;

  if (len == 0 || valeur == nullptr) return 1;  // sécurité de base

  if (paramV == 1)  // registre 1 : adresse IP
  {
    res = 0;
    local_ip.toString().toCharArray(valeur, 16);
  }
  if (paramV == 2)  // registre 2 : adresse gateway
  {
    res = 0;
    gateway.toString().toCharArray(valeur, 16);
  }
  if (paramV == 3)  // registre 3 : adresse subnet
  {
    res = 0;
    subnet.toString().toCharArray(valeur, 16);
  }
  if (paramV == 4)  // registre 4 : adresse DNS primaire
  {
    res = 0;
    primaryDNS.toString().toCharArray(valeur, 16);
  }
  if (paramV == 5)  // registre 5 : adresse DNS secondaire
  {
    res = 0;
    secondaryDNS.toString().toCharArray(valeur, 16);
  }
  if (paramV == 6)  // registre 6 : nom routeur
  {
    res = 0;
    strncpy(valeur, nom_routeur, 16);
    valeur[15] = '\0';
  }
  if (paramV == 7)  // registre 7 : mdp routeur
  {
    if (cpt_securite) {
      res = 0;
      strncpy(valeur, mdp_routeur, 16);
      valeur[15] = '\0';
    }
  }
  if (paramV == 8)  // registre 8 : websocket On
  {
    res = 0;
    valeur[0]=websocket_on+'0';
    valeur[1] = '\0';
  }
  if (paramV == 9)  // registre 9 : adresse websocket
  {
    res = 0;
    strncpy(valeur, ip_websocket, 40);
    valeur[39] = '\0';
  }
  if (paramV == 10)  // registre 10 : websocket Id
  {
    res = 0;
    valeur[0]=id_websocket+'0';
    valeur[1] = '\0';
  }

  res2 = requete_Get_String_appli(type, var, valeur);


  return (res+res2-1);
}

// execution d'une action avec une valeur - type 5
uint8_t requete_Set_Action(const char *reg, const char *data)
{
  uint8_t res = 1;
  uint8_t res2 = 1;

    // affichage des log
    if (strcmp(reg, "LOG") == 0) 
    {  
      res = 0;
      uint16_t nb_log=10;
      nb_log = strtoul(data, NULL, 10);
      if (nb_log>(MAX_DUMP/LOG_ENTRY_SIZE-3)) nb_log=MAX_DUMP/LOG_ENTRY_SIZE-3;
      readLastLogsBinary((uint8_t*)buffer_dmp, nb_log);  
    }

  if (cpt_securite)
  {
    // enregistrement d'un log
    if (strcmp(reg, "LOGW") == 0)
    {
      res = 0;
      uint8_t val1 =  strtoul(data, NULL, 10);
      writeLog('Z', 2, val1, 0, "Log");
    }

    // Test : log une erreur fictive
    if (strcmp(reg, "LOG_E") == 0) 
    { 
      res=0; 
      log_erreur(Code_erreur_depass_tab_status, 9, 0);
    }

    // affichage de la mémoire dispo
    if (strcmp(reg, "MEM") == 0) 
    { 
      res=0; 
      printMemoryStatus();
    }

    // efface la memoire NVS
    if (strcmp(reg, "RAZ_NVS") == 0) 
    { 
      res=0; 
      esp_err_t err = nvs_flash_erase();
      nvs_flash_init();
      if (err == ESP_OK) {
        Serial.println("NVS erased successfully.");
      } else {
        Serial.printf("Error erasing NVS: 0x%x\n", err);
      }
    }

    // affichage de la charge CPU
    if (strcmp(reg, "CPU") == 0) 
    { 
     /* res=0; 
      idleTaskHandle = xTaskGetIdleTaskHandleForCPU(0);  // CPU 0 (ESP32 est dual core)
      vTaskDelay(pdMS_TO_TICKS(1000)); // Attendre 1 seconde

      // Utiliser uxTaskGetSystemState pour avoir les temps CPU
      TaskStatus_t *taskStatusArray;
      volatile UBaseType_t taskCount;
      volatile uint32_t totalRunTime;

      taskCount = uxTaskGetNumberOfTasks();
      taskStatusArray = pvPortMalloc(taskCount * sizeof(TaskStatus_t));

      if (taskStatusArray != NULL)
      {
          // Récupérer les infos des tâches
          taskCount = uxTaskGetSystemState(taskStatusArray, taskCount, &totalRunTime);

          uint32_t idleRunTime = 0;
          for (int i = 0; i < taskCount; i++) {
              if (strcmp(taskStatusArray[i].pcTaskName, "IDLE") == 0) {
                  idleRunTime = taskStatusArray[i].ulRunTimeCounter;
                  break;
              }
          }
          free(taskStatusArray);

          // totalRunTime est en "ticks" CPU, idleRunTime aussi
          // Charge CPU estimée = 1 - (idleRunTime / totalRunTime)
          float cpuLoad = 1.0f - ((float)idleRunTime / (float)totalRunTime);
          printf("CPU Load: %.2f%%\n", cpuLoad * 100);
      }*/
    }

    // plantage CPU
    if (strcmp(reg, "RST1") == 0) 
    { 
      res=0; 
      uint8_t var1=0;
      int *ptr = nullptr;
      writeLog('S', 9, 0, 0, "Boot P");
      Serial.println("Crash imminent...");
      delay(2000);

      uint8_t var2 = 58 / var1; // provoque un crash
      *ptr = 42+var2;  // provoque aussi un crash
    }
    
    // redémarrage soft
    if (strcmp(reg, "RST0") == 0) 
    { 
      res=0; 
      writeLog('S', 9, 0, 0, "Boot S");
      delay(1000);
      esp_restart();  // Redémarrage logiciel sans effacer la RTC RAM
    }

    if (strcmp(reg, "PING0") == 0) 
    { 
      res=0; 
      uint8_t err = testConnexionGoogle();
      delay(200);
      Serial.printf("res google:%i\n\r",err);
    }

    if (strcmp(reg, "PING1") == 0) 
    { 
      res=0; 
      uint8_t err = testHttpServerLocal();
      delay(200);
      Serial.printf("res local:%i\n\r",err);
    }

    if (strcmp(reg, "PING2") == 0) 
    { 
      res=0; 
      uint8_t etat = WiFi.status();
      delay(200);
      Serial.printf("wifi status:%i\n\r",etat);
    }

    // liste taches
    if (strcmp(reg, "TASK") == 0) 
    { 
      res=0; 
      dumpTasksInfo();
      delay(200);
    }
    // liste les taches
    if (strcmp(reg, "VTASKS") == 0) 
    { 
      res=0; 
      //vTaskList(buffer_dmp);
    }


    // relance reseau wifi
    if (strcmp(reg, "RESL") == 0) 
    { 
      res=0; 
      uint8_t err = reConnectWifi();
      delay(200);
      Serial.printf("relance wifi:%i\n\r",err);
    }

    // relance serveur http
    if (strcmp(reg, "HTTPL") == 0) 
    { 
      res=0; 
      server.end();  // sécuriser l'état précédent
      startWebServer();
      delay(200);
      Serial.printf("relance serveur web\n\r");
    }

    // relance serveur http
    if (strcmp(reg, "E_NOW") == 0) 
    { 
      res=0; 
      //envoi_data_gateway(1, 20.1, 20.2);
      delay(200);
      //Serial.printf("envoi esp_now T=20.2\n\r");
      delay(1000);
    }
  
    res2 = requete_action_appli(reg, data);
  }
  
  return (res+res2-1);
}

// type 4
uint8_t requete_Set_String(int param, const char *texte)
{
  //Serial.printf("set2:p:%i lg:%i  %s\n\r",param, strlen(texte), texte);

  uint8_t res = 1;
  uint8_t res2 = 1;

  //Serial.println(strlen(texte));
  //Serial.println(param);

  if ((cpt_securite) && (texte != nullptr) && (strlen(texte) >= 1) && (strlen(texte) <= 40))
  {
    IPAddress ip;
    
    if (param == 1)  // registre 1 : Adresse IP
    {
      if (ip.fromString(texte))
      {
        res = 0;
        uint32_t IP32 = (uint32_t)ip;  // transforme en uint32_t
        //IP32 = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
        //Serial.printf("ip:%s %08X\n\r", ip.toString().c_str(), IP32);
        preferences_nvs.putULong("ipAdd", IP32);
        local_ip = ip;
      }
    }
    if (param == 2)  // registre 2 : Adresse gateway
    {
      if (ip.fromString(texte))
      {
        res = 0;
        uint32_t IP32 = (uint32_t)ip;  // transforme en uint32_t
        preferences_nvs.putULong("ipGat", IP32);
        gateway = ip;
      }
    }
    if (param == 3)  // registre 3 : Adresse subnet
    {
      if (ip.fromString(texte))
      {
        res = 0;
        uint32_t IP32 = (uint32_t)ip;  // transforme en uint32_t
        preferences_nvs.putULong("ipSub", IP32);
        subnet = ip;
      }
    }
    if (param == 4)  // registre 4 : Adresse DNS
    {
      if (ip.fromString(texte))
      {
        res = 0;
        uint32_t IP32 = (uint32_t)ip;  // transforme en uint32_t
        preferences_nvs.putULong("ipDNS", IP32);
        primaryDNS = ip;
      }
    }
    if (param == 5)  // registre 5 : Adresse DNS2
    {
      if (ip.fromString(texte))
      {
        res = 0;
        uint32_t IP32 = (uint32_t)ip;  // transforme en uint32_t
        preferences_nvs.putULong("ipDNS2", IP32);
        secondaryDNS = ip;
      }
    }
    if ((param == 6) && (strlen(texte) <= 15)) // registre 6 : nom routeur
    {
      res = 0;
      preferences_nvs.putString("Rout", texte);
      strncpy(nom_routeur, texte, sizeof(nom_routeur) - 1);
      nom_routeur[sizeof(nom_routeur) - 1] = '\0';  // Assure la terminaison
    }
    if ((param == 7) && (strlen(texte) <= 15)) // registre 7 : mdp routeur
    {
      res = 0;
      preferences_nvs.putString("Mdp", texte);
      strncpy(mdp_routeur, texte, sizeof(mdp_routeur) - 1);
      mdp_routeur[sizeof(mdp_routeur) - 1] = '\0';  // Assure la terminaison
    }

    if (param == 8)  // registre 8 : websocket On
    {
      if (isdigit(*texte))
      {
        uint8_t WSOn = atoi(texte);
        if ((WSOn==1) || (WSOn==2))
        {
          res = 0;
          preferences_nvs.putUChar("WSOn", WSOn);
          websocket_on = WSOn;
          if (WSOn==1)    xTimerStop(xTimer_Websocket,100);    
          else     xTimerStart(xTimer_Websocket,100);   
        }
      }
    }
    if (param == 9)  // registre 9 : adresse websocket
    {
      res = 0;
      preferences_nvs.putString("WSock", texte);
      strncpy(ip_websocket, texte, sizeof(ip_websocket) - 1);
      ip_websocket[sizeof(ip_websocket) - 1] = '\0';  // Assure la terminaison
    }
    if (param == 10)  // registre 10 : websocket id : 1 à 10
    {
      if (isdigit(*texte))
      {
        uint8_t IDWS = atoi(texte);
        if ((IDWS) && (IDWS <10))
        {
          res = 0;
          preferences_nvs.putUChar("WSId", IDWS);
          id_websocket = IDWS;
        }
      }
    }

    res2 = requete_Set_String_appli(param, texte);

  }
  return (res+res2-1);
}

static bool setRtcDateFromNumericValue(int32_t numericDate)
{
  if (numericDate < 10100 || numericDate > 311299)
    return false;

  uint8_t day = numericDate / 10000;
  uint8_t month = (numericDate / 100) % 100;
  uint16_t year = 2000 + (numericDate % 100);

  if (month < 1 || month > 12 || day < 1 || day > 31)
    return false;

  const uint8_t monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint8_t maxDay = monthDays[month - 1];
  if (month == 2) {
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    if (leap) {
      maxDay++;
    }
  }
  if (day > maxDay)
    return false;

  struct tm localTime;
  if (!getLocalTime(&localTime, 1000)) {
    time_t now = time(nullptr);
    if (now == ((time_t)-1) || localtime_r(&now, &localTime) == NULL) {
      localTime.tm_hour = 0;
      localTime.tm_min = 0;
      localTime.tm_sec = 0;
    }
  }

  localTime.tm_mday = day;
  localTime.tm_mon = month - 1;
  localTime.tm_year = year - 1900;

  time_t sec = mktime(&localTime);
  if (sec < 0)
    return false;

  timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  getLocalTime(&timeinfo, 1000);
  Serial.printf("RTC date set %02u/%02u/%04u %02u:%02u:%02u\n",
                day, month, year,
                localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
  return true;
}

static bool setRtcTimeFromNumericValue(int32_t numericTime)
{
  if (numericTime < 0 || numericTime > 235959)
    return false;

  int hours;
  int minutes;
  int seconds;

  if (numericTime <= 99) {
    hours = numericTime;
    minutes = 0;
    seconds = 0;
  } else if (numericTime <= 9999) {
    hours = numericTime / 100;
    minutes = numericTime % 100;
    seconds = 0;
  } else {
    seconds = numericTime % 100;
    minutes = (numericTime / 100) % 100;
    hours = numericTime / 10000;
  }

  if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59)
    return false;

  struct tm localTime;
  if (!getLocalTime(&localTime, 1000)) {
    localTime = timeinfo;
    localTime.tm_sec = 0;
    localTime.tm_min = 0;
    localTime.tm_hour = 0;
  }

  localTime.tm_hour = hours;
  localTime.tm_min = minutes;
  localTime.tm_sec = seconds;

  time_t sec = mktime(&localTime);
  if (sec < 0)
    return false;

  timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  getLocalTime(&timeinfo, 1000);
  Serial.printf("RTC time set %02u:%02u:%02u\n", hours, minutes, seconds);
  return true;
}


// type 2
uint8_t requete_GetReg(int reg, float *valeur) {
  uint8_t res = 1;
  uint8_t res2 = 1;

  if (reg == 1)  // registre 1 : mode reseau
  {
    res = 0;
    *valeur = mode_reseau;
  }
  if (reg == 2)  // registre 2 : nb_reset
  {
    res = 0;
    *valeur = nb_reset;
  }

  if (reg == 4)  // registre 4 : cycle lecture
  {
    res = 0;
    *valeur = periode_cycle;
  }
  if (reg == 5)  // registre 5 : cycle rapide
  {
    res = 0;
    *valeur = mode_rapide;
  }
  if (reg == 6)  // registre 6 : détail log
  {
    res = 0;
    *valeur = log_detail;
  }

  if (reg == 8)  // registre 8 : ski_graph : 1 valeur sur x
  {
    res = 0;
    *valeur = skip_graph;
    //Serial.printf("skip:%i\n\r", skip_graph);
  }
  if (reg == 13)  // registre 13 : activation OTA
  {
    res = 0;
    *valeur = otaEnabled;
  }
  if (reg == 43)  // registre 43 : puissance d'emission wifi
  {
    res = 0;
    int8_t power;
    esp_wifi_get_max_tx_power(&power);
    *valeur = power/4;  // Convertir en dBm  : unité = 0.25 dBm => diviser par 4

  }
  if (reg == 44)  // registre 44 : wifi_sleep
  {
    wifi_ps_type_t mode;
    esp_err_t err = esp_wifi_get_ps(&mode);    
    if (err == ESP_OK) {
      res = 0;
      *valeur =  (float)mode;
    }
  }
  if (reg == 45) // registre 45 : niveau de reception wifi
  {
    res = 0;
    *valeur = WiFi.RSSI();
  }

  res2 = requete_GetReg_appli(reg, valeur);

  //if (!res) *valeur = (float)val16;
  Serial.printf("val_get: %i %f %i\n\r", reg, *valeur, skip_graph);
  return (res+res2-1);
}

// type 2
uint8_t requete_SetReg(int param, float valeurf)
{
  int32_t valeur = int32_t(valeurf);
  uint8_t res = 1;
  uint8_t res2= 1;


  if (cpt_securite)
  {
    if (param == 1)  // registre 1 : mode reseau
    {
      if ((valeur >= 11) && (valeur <= 14)) {
        res = 0;
        mode_reseau = valeur;
        Serial.printf("modif 1:%i\n\r", mode_reseau);
        preferences_nvs.putUChar("reseau", valeur);
      }
    }
    if (param == 2)  // registre 2 : nb de reset
    {
      res = 0;
      nb_reset = valeur;
      preferences_nvs.putUShort("nb_reset", nb_reset);
    }
    if (param == 3)  // registre 3 : reset par watchdog
      if (valeur == 13) {
        //ESP.restart();
        res = 0;
        writeLog('S', 9, 0, 0, "Boot W");
        delay(500);
        //param_wdt_delay = WDT_TIMEOUT * 12;
        esp_restart();  // Redémarrage logiciel sans effacer la RTC RAM
      }

    if (param == 4)  // registre 4 : Periode cycle (en minutes)
    {
      //#ifndef DEBUG
      if ((valeur >= 2) && (valeur <= 60))  // entre 2 et 60 minutes
      {
        res = 0;
        periode_cycle = valeur;
        preferences_nvs.putUChar("cycle", periode_cycle);
        // modification du timer
        modif_timer_cycle();
      }
    }
    if (param == 5)  // registre 5 : cycle rapide =12
    {
      if ((valeur == 12) || (!valeur)) {
        res = 0;
        mode_rapide = valeur;
        preferences_nvs.putUChar("Rap", mode_rapide);
        modif_timer_cycle();
      }
    }

    if (param == 6)  // registre 6 : détail log
    {
      if (valeur <= 4) {
        res = 0;
        log_detail = valeur;
        preferences_nvs.putUChar("LogD", log_detail);
      }
    }

    if (param == 7)  // registre 7 : delai ecoute websocket
    {
      if ((valeur) && (valeur <= 30)) {
        res = 0;
        DelaiWebsocket = valeur;
        preferences_nvs.putUChar("DelWS", DelaiWebsocket);
      }
    }

    if (param == 8)  // registre 8 : skip_graph
    {
      if ((valeur) && (valeur <= 50))  // entre 1 et 50
      {
        res = 0;
        skip_graph = (uint8_t)valeur;  // 1 valeur sur x
        //Serial.printf("skip:%i\n\r", skip_graph);
        preferences_nvs.putUChar("Skip", skip_graph);
      }
    }


    if (param == 11)  // registre 11 : réglage date numérique ddmmyy
    {
      if (setRtcDateFromNumericValue(valeur))
      {
        init_time = 4;
        res = 0;
      }
    }

    if (param == 12)  // registre 12 : réglage heure numérique hhmmss/hhmm/hh
    {
      if (setRtcTimeFromNumericValue(valeur))
      {
        init_time = 4;
        res = 0;
      /*  uint8_t hh;
        uint8_t mm;
        uint8_t ss;
        if (valeur <= 99) {
          hh = valeur;
          mm = 0;
          ss = 0;
        } else if (valeur <= 9999) {
          hh = valeur / 100;
          mm = valeur % 100;
          ss = 0;
        } else {
          hh = valeur / 10000;
          mm = (valeur / 100) % 100;
          ss = valeur % 100;
        }
        heure = hh + (float)mm / 60.0 + (float)ss / 3600.0;
        //preferences_nvs.putUChar("Heure", hh);*/
      }
    }

    if (param == 13)  // registre 13 : 
    {
      if (valeur==0)
      {
        res = 0;
        otaEnabled = false;
      }
      if (valeur==1)
      {
        res = 0;
        otaEnabled = true;
        otaStartTime = millis();
      }
    }
    if (param == 43)  // registre 43 : puissance d'emission wifi
    {
      if ((valeur >= -12) && (valeur <= 20))  // entre -12dBm et +20dBm
      {
        res = 0;
        wifi_power_t power = (wifi_power_t)(valeur * 4);  // Convertir en unité de 0.25 dBm
        esp_wifi_set_max_tx_power(power);
      }
    }
    if (param == 44)  // registre 44 : wifi_sleep :0(none), 1(light), 2(max)
    {
      if ((valeur>=0) && (valeur <=3))
      {
        res = 0;
        esp_wifi_set_ps((wifi_ps_type_t)valeur);  //  mode de sommeil Wi-Fi
      }
    }


    res2 = requete_SetReg_appli(param, valeurf);

  }
  //return (res+res2-1);
  return (res == 0 || res2 == 0) ? 0 : 1;
}



// activation de 2 PIn pour allumage et extinction
/*void systeme_activ(uint8_t ordre) {
  if (ordre == 1) {
    digitalWrite(PIN_on, HIGH);  // Allume contact : appuie 0,5 seconde
    delay(500);
    digitalWrite(PIN_on, LOW);
    systeme_marche = 1;
  } else {
    digitalWrite(PIN_off, HIGH);  // Eteint contact  :appui 0,5 seconde
    delay(500);
    digitalWrite(PIN_off, LOW);
    systeme_marche = 0;
  }
}*/

// type=0:tout  type=1:maj uniquement (sans tableaux)
void requete_status(char *json_response, uint8_t socket, uint8_t type)
{

  force_stay_awake = true;
  wake_up_time = millis() + 60000;  // prolonger si demande page web

  // Vérification de sécurité du pointeur
  if (json_response == nullptr) {
    Serial.println("ERREUR: json_response est NULL");
    return;
  }
  
  // Protection contre les lectures simultanées du DHT22
  static bool lecture_en_cours = false;
  static unsigned long derniere_lecture = 0;
  
  unsigned long mill = millis();

  #ifdef ESP_VEILLE
    // Attendre au moins 2 secondes
   /* if (mill - derniere_lecture < DHT22_MIN_INTERVAL_MS) {
      // Utiliser la dernière valeur lue si trop fréquent
      // Tint reste inchangée
    } else {
      // Éviter les lectures simultanées
      if (!lecture_en_cours)
      {
        lecture_en_cours = true;
        derniere_lecture = mill;
        
        uint8_t Tint_erreur=0;
        Tint_erreur = lecture_Tint(&Tint, &Humid);
        
        if (Tint_erreur) {
          log_erreur(Code_erreur_Tint, Tint_erreur,0);
          if (Tint_erreur < 10)
            Tint = 20;
        }
        lecture_en_cours = false;
      }
    }*/
  #endif

  uint8_t Tint_erreur=0;
  Tint_erreur = lecture_Tint(&Tint, &Humid);

  //uint8_t Text_erreur=0;
  //Text_erreur = lecture_Text(&Text);
  //if (Text_erreur) Text = 15;


  unsigned long sec = mill / 1000;  // 65000
  int min = (sec / 60) % 60;  //
  int hour = (sec / 3600) % 24;
  int day = (sec / 3600 / 24);
  snprintf(St_Uptime, 30, "%d jours %d heures %d min", day, hour, min);


  TickType_t now = xTaskGetTickCount();
  TickType_t expir = xTimerGetExpiryTime(xTimer_Cycle);
  uint32_t Proch_periode = (expir - now) / configTICK_RATE_HZ;  // nb de secondes avant prochaine mesure temp piscine DSB
  //TickType_t Proch_periode = (expir - now) * portTICK_PERIOD_MS/1000;    // nb de secondes avant prochaine mesure temp piscine DSB


  if (init_time) lectureHeure();

  char *p = json_response;
  *p++ = '{';
  if (socket)   p += sprintf(p, "\"action\":\"status\",");  // ajout de action=status si c'est un message socket
  p += sprintf(p, "\"version\":\"%s\",", Version);
  p += sprintf(p, "\"reset0\":\"%s\",", resetREASON0);
  p += sprintf(p, "\"heure_string\":%s,", heure_string);
  p += sprintf(p, "\"uptime\":\"%s\",", St_Uptime);
  p += sprintf(p, "\"nb_reset\":%i,", nb_reset);
  p += sprintf(p, "\"duree_reset\":\"%s\",", duree_RESET);
  uint8_t modif=0;
  if (cpt_securite) modif=1;
  p += sprintf(p, "\"codeR_secu\":%i,", modif);
  p += sprintf(p, "\"coche_secu\":%i,", modif);

  p += sprintf(p, "\"periode_cycle\":%d,", periode_cycle);
  p += sprintf(p, "\"PPE\":%i,", Proch_periode);

  p += sprintf(p, "\"Text\":%.1f,", Text);
  p += sprintf(p, "\"Tint\":%.2f,", Tint);
  p += sprintf(p, "\"Humid\":%.2f,", Humid);


  uint8_t batSI = 0;
  float vbatt = readBatteryVoltage();

  if (((vbatt*1000) < Seuil_batt_sonde)  && (vbatt)) batSI=1;
  p += sprintf(p, "\"batSI\":%i,", batSI);
  p += sprintf(p, "\"batS\":%.2f,", vbatt);

  p += sprintf(p, "\"Hum_V\":%i,", HumV); // veille
  p += sprintf(p, "\"TextV\":%i,", TextV);  // Temp ext de la veille
  p += sprintf(p, "\"TintV\":%i,", TintV);  // Temp ext de la veille
  
  p += sprintf(p, "\"PIR_D\":%i,", compteur_detection); // derniere 15min
  p += sprintf(p, "\"PIR_V\":%i,", PIRV); // veille


  // Tableaux : E(erreurs) T(temp)
  if (!type)  // pas d'envoi des graphiques si type=1(maj)
  {
    // Nota: les 0 sont sautés
    uint8_t i,j;  // 10 car par valeur => 1000 car par graphique
    for (j = 0; j < NB_Graphique; j++) {
      // valeurs de temperature
      for (i = 0; i < NB_Val_Graph; i++) {
        //printf("val %d : %u\n",i, temp_pisc_hist[i]);
        if (graphique[i][j]) {
          int remaining = MAX_DUMP - (p - json_response) -2;
          int n = snprintf(p, remaining, "\"T%d%d\":%i,", j, i, graphique[i][j]);
          if (n >= remaining || n < 0) {
          // Plus assez de place dans le buffer ou erreur
            break;
          }
          p+=n;
        }
      }
    }
  }

  // Tableaux : log erreurs
  uint8_t i;
  for (i = 0; i < Nb_erreurs; i++) {
    if (erreur_code[i]) {
      int remaining = MAX_DUMP - (p - json_response) -2;
      int n = snprintf(p, remaining, "\"E%d\":\"%d %d %d %d %d %d\",", i, erreur_code[i], erreur_valeur[i], erreur_val2[i], erreur_jour[i], erreur_heure[i], erreur_minute[i]);
      if (n >= remaining || n < 0) {
      // Plus assez de place dans le buffer ou erreur
        break;
      }
      p+=n;
    }
  }

  uint32_t nb_car;
  nb_car = (uint32_t)p - (uint32_t)json_response;
  //nb_car = *p - nb_car;
  Serial.printf("nb car status:%lu  max:%i\n\r", (uint32_t)nb_car, MAX_DUMP);
  if ((nb_car) >= MAX_DUMP-2)  // erreur dépassement de tableau
  {
    uint8_t depass;
    depass = ((nb_car) >> 6);  // division par 64
    log_erreur(Code_erreur_depass_tab_status, depass, 0);
  }

  p--;
  *p++ = '}';
  *p++ = 0;

}

void printMemoryStatus()
{
  #ifdef DEBUG
    size_t internalFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    size_t totalFree = esp_get_free_heap_size();

    snprintf(buffer_dmp, MAX_DUMP,
              "Memoire interne:%u   contigu:%u  total:%u\r\n",
              (unsigned int)internalFree,
              (unsigned int)largestBlock,
              (unsigned int)totalFree);
    Serial.printf("Memoire interne:%d   contigu:%d  total:%d \n\r", internalFree, largestBlock, totalFree );
    delay(10);
  #endif
}

void activation_writelog()
{
  if ((log_err) || (!logPartition))
  {
    logPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x99, "log_flash");

    if (!logPartition) {
      Serial.println("Partition 'log_flash' non trouvée !");
      log_err=1;
    }
    else
    {
      log_err=0; // ok
      Serial.println("Partition 'log_flash' trouvée.");
    }
  }

  if ((log_errG) || (!logPartitionG))
  {
    logPartitionG = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x98, "log_flashG");

    if (!logPartitionG) {
      Serial.println("Partition 'log_flashG' non trouvée !");
      log_errG=1;
    }
    else
    {
      log_errG=0; // ok
      Serial.println("Partition 'log_flashG' trouvée.");
    }
  }

}

// Fonction pour calculer le checksum Contact ID
/*    Les codes de message à 15 chiffres (numéro de compte au code de zone) du système d'alarme sont accumulés en nombres binaires. Si un bit de ces données est nul, il est ajouté sous forme de nombre binaire 0AH.
    Cette somme accumulée est divisée par 15.
    Le chiffre entier du quotient est ajouté par 1 puis multiplié par 15.
    La différence après soustraction de la somme accumulée de ce produit est le code de contrôle.
*/
char calculateChecksum(String data)
{
  int sum = 0;

    // 1. Accumuler la somme, en traitant les '0' comme 0x0A
    for (int i = 0; i < data.length(); i++) {
        char c = data[i];
        int value = (c == '0') ? 10 : (c - '0');
        sum += value;
    }

    // 2. Division entière
    int quotient = sum / 15;

    // 3. Calcul du produit
    int produit = (quotient + 1) * 15;

    // 4. Calcul du code de contrôle
    int checksum = produit - sum;

    // 5. Retourner le checksum sous forme de caractère ASCII ('0' à '9')
    return (char)(checksum + '0');
}


uint16_t crc16_arc(const uint8_t *data, size_t length)
{
    uint16_t crc = 0x0000;  // Init à 0x0000 pour CRC16-ARC
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;  // 0xA001 est 0x8005 inversé
            else
                crc >>= 1;
        }
    }
    return crc;
}

uint8_t validIp(const char *stringIP, IPAddress *ip) 
{
  uint8_t err=0;
  Serial.printf("IP2: %s %s\n\r", stringIP, ip->toString().c_str());
  if (stringIP == NULL) err=1;  // Vérifier si NULL
  else
  {
    int segments = 0;  // Nombre d'octets (0-255)
    int dots = 0;      // Nombre de points trouvés
    int segmentIndex = 0;  // Pour savoir quel octet on est en train de remplir

    while ((*stringIP) && (!err)) {
        if (*stringIP == '.') {
            if (segments == 0 || segments > 255) err=2;  // Valeur hors limite
            (*ip)[segmentIndex++] = segments;
            dots++;
            segments = 0;  // Réinitialiser pour le prochain octet
        } 
        else if (isdigit(*stringIP)) {
            segments = segments * 10 + (*stringIP - '0');  // Construire le nombre
            if (segments > 255) err=3;  // Vérifier la plage 0-255
        } 
        else {
            err=4;  // Caractère invalide
        }
        stringIP++;
    }

  // Assigner le dernier segment après la boucle
    if (segments >= 0 && segments <= 255 && !err) {
        (*ip)[segmentIndex] = segments;
        segmentIndex++;
    }

    // Vérifier si l'IP a exactement 3 points et 4 segments valides
    if (dots != 3 || segmentIndex != 4) err=5;
  }
  Serial.printf("err:%i\n\r", err);
  Serial.printf("IP3:  %s\n\r", ip->toString().c_str());
  delay(200);
  return err;
}

// type : 1, 2, 3, 4 ou A    :   1-12 -> get   1-12:34 -> set   A12:45 -> action
// type 1 : variable        => requete_Get
// type 2 : registre        => requete_GetReg   & requete_GetReg_appli
// type 3 : registre modbus => read_modbus
// type 4 : texte           =>  requete_Get_String  & requete_Get_String_appli
// type 5(A) : action
void recep_message(char *messa) // recept_uart
{
  uint8_t type;
  uint8_t res;
  char reg[11];
  char data[200];  // Variable pour stocker ce qu'il y a après ":"
  data[0]='\0';
  uint8_t err=1;
  uint8_t len = strlen(messa);
  if (len >= sizeof(data)) len = sizeof(data) - 1;  // Évite un dépassement
  //uint8_t a_param=0;  // 0 ou 1 : paramètre ou non
  //uint16_t param=0;
  
  if ((messa[0]=='A') || ((messa[0]>='1') && (messa[0]<='4')))
  {
    #ifndef Sans_securite
      if (xTimer_Securite != NULL) 
      {
          xTimerStop(xTimer_Securite,100);
          xTimerChangePeriod(xTimer_Securite,(uint32_t)15*60*(1000/portTICK_PERIOD_MS),100);
          xTimerStart(xTimer_Securite,100);  // permet de couper la securite au bout de 2 minutes
      }
    #endif
    cpt_securite = 1;
  }

  if (messa[0]=='A')  // AREICV:23
  {
      char *ptr = strchr(messa, ':');  
      if (ptr != NULL)
      {  // Action avec paramètre
        //a_param=1;
        size_t len_data = strlen(ptr);
        if (len-len_data-1 >= sizeof(reg)) err=1;
        else
        {
          memcpy(reg, messa+1, len-len_data-1); //9-3-1=5
          reg[len-len_data-1] = '\0';
          if ((len_data>1) && (len_data < sizeof(data)))
          {
            memcpy(data, ptr+1, len_data-1);
            data[len_data-1] = '\0';
            Serial.printf("lg_tot:%i lg_data:%i\n\r", len, len_data-1);
            err=0;
          }
        }
      }
      else // Action sans paramètre
      {
        if ((len < sizeof(reg)) && (len))
        {
          memcpy(reg, messa+1, len-1);
          reg[len-1] = '\0';
          err=0;
        }
      }
      if (!err)  // pas d'erreur
      {
        Serial.printf("Action:%s %s\n\r", reg, data);
        res = requete_Set_Action(reg, data);     
        Serial.printf(" %s\n\r", buffer_dmp);
      }
  }
  else  // message type 1 à 4
  {
    err=0;
    if (messa[1] != '-') err=1;
    type = messa[0]-'0';
    if (!type || type>4) err=1;
    if (!err)
    {
      char *ptr = strchr(messa, ':');  
      /*uint8_t i;
      for (i=0; i<10; i++)
        Serial.printf("%02X ", messa[i]);
      Serial.println();*/
      if (ptr != NULL) {  // ------------ Set  2-4:60
          Serial.println("SET");
          ptr++;  // Avance après le ":"
          size_t len_data = strlen(ptr);  // Taille réelle du texte à copier =>2
          if (len_data >= sizeof(data)) len_data = sizeof(data) - 1;  // Évite un dépassement
          if (len-len_data-2 > sizeof(reg)) 
            err=1;
          else
          {
            memcpy(reg, messa+2, len-len_data-3);
            reg[len-len_data-3] = '\0';
            //uint8_t lg = len-len_data-2;
            //Serial.println(lg);
            memcpy(data, ptr, len_data);
            data[len_data] = '\0';
            res = requete_Set(type, reg, data);
            Serial.printf("Set: res:%i type:%i %s=%s \n\r", res, type, reg, data);
          }
      }
      else  // ------------ get
      {
        if (len<12)   // lg 9 max pour le registre
        {
          memcpy(reg, messa+2, len-2);
          reg[len-2] = '\0';
          if (type<4) {
            float valeur;
            res = requete_Get(type, reg, &valeur);
            Serial.printf("lecture %i-%s : %f\n\r", type, reg, valeur);
          }
          else  {  // type=4
            char valeur_S[50];
            res = requete_Get_String(type, reg, valeur_S);
            Serial.printf("lecture %i-%s : %s\n\r", type, reg, valeur_S);
          }
        }
      }
    }
  }
}

void recep_message1(UartMessage_t * messa) // recept_uart1
{
    Serial.print("recept uart1:");
    Serial.println(messa->msg);
    delay(1000);
    traitement_rx(messa);
}

void traitement_rx(UartMessage_t * mess)
{
  uint8_t longueur_m = mess->longueur;
  uint8_t * message_in = (uint8_t *) mess->msg;

		if ((message_in[2] == 'C') && (message_in[3] == 'H'))  // Chaudiere
		{
			if (message_in[4] == 'E')  // Ecriture
			{
        if ((message_in[5] == 'P') && (longueur_m == 15))   // CHEPnxxyyzztt  Planning  %cCHEP%i%02X%02X%i%02X%02X  i, ch_debut[i], ch_fin[i], ch_type[i], ch_consigne[i], ch_cons_apres[i])
        {
        }
      }
    }
}

void debounceCallback(TimerHandle_t xTimer)
{
    bool needRestart = false;
    //debounceFlag = true;

    for (int i = 0; i < BTN_COUNT; i++) {
        int buttonState = digitalRead(BTN_PIN[i]);  // Lire la pin actuelle

        // Incrémenter/décrémenter le compteur en fonction de l'état lu
        if (buttonState == LOW) {
            if (pressCounter[i] < VALIDATION_COUNT) pressCounter[i] ++;
        } else {
            if (pressCounter[i] > 0) pressCounter[i] --;
        }

        // Vérification si l'état a changé et atteint un seuil
        if (pressCounter[i] == VALIDATION_COUNT && stableButtonState[i] != LOW) {
            stableButtonState[i] = LOW;
            //Serial.printf("Btn %d On !\n", i); // peut planter
            if (!init_masquage) { // masquage les 30 premières secondes
              systeme_eve_t evt = { EVENT_GPIO_ON, (uint32_t)i};
              if (xQueueSend(eventQueue, &evt, 0) != pdTRUE) 
              {
                if (erreur_queue<5) num_err_queue[erreur_queue]=10;
                erreur_queue++;
              }
            }
        } 
        else if (pressCounter[i] == 0 && stableButtonState[i] != HIGH)
        {
            stableButtonState[i] = HIGH;
            //Serial.printf("Btn %d off00 !\n", i);
            if (!init_masquage) { // masquage les 30 premières secondes
              systeme_eve_t evt = { EVENT_GPIO_OFF, (uint32_t)i };
              if (xQueueSend(eventQueue, &evt, 0) != pdTRUE) 
              {
                if (erreur_queue<5) num_err_queue[erreur_queue]=11;
                erreur_queue++;
              }
            }
        }

        // Si l'état n'est pas encore stabilisé, on doit relancer le timer
        if (pressCounter[i] > 0 && pressCounter[i] < VALIDATION_COUNT) {
            needRestart = true;
        }
    }

    // Relancer le timer si au moins un bouton est en phase de stabilisation
    if (needRestart) {
        xTimerStart(debounceTimer, 0);
    }
}

void checkPartitions()
{
  #ifdef DEBUG
    Serial.println("Table des partitions:");

    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it != NULL) {
        const esp_partition_t *p = esp_partition_get(it);
        Serial.printf("Type: %02X, Sous-type: %02X, Nom: %s, Taille: %lu octets, Adresse: 0x%lX\n", p->type, p->subtype, p->label, p->size, p->address);
        it = esp_partition_next(it);
    }
  #endif
}

// trouve page flash en cours et index dans la page 
void getActiveIndex(uint8_t * page, uint16_t *index) 
{
  uint8_t flag1, flag2;  // FF:vierge  0:plein 1:en cours
  uint8_t data;
  esp_partition_read(logPartition, 0, &flag1, 1);
  esp_partition_read(logPartition, PAGE_SIZE, &flag2, 1);

  if (flag1 == 0xFF && flag2 == 0xFF)  // Page flash vierge
  {
    // on active la premiere page
    data=1;
    esp_partition_write(logPartition, 0, &data, 1);
    *page=1;
    *index=1;
  }
  else
  {
    if (flag1 == 0x01) // en cours
    {
      *page = 1;
      *index = trouve_index(0);
    }
    else
    {
      if (flag2 == 0x01) // en cours
      {
        *page = 2;
        *index = trouve_index(1);
      }
      else
      {
        *page=1;  // efface page 1
        *index=1;
        esp_partition_erase_range(logPartition, 0, PAGE_SIZE);
        Serial.println("page 1 effacee");
        data = 1;
        esp_partition_write(logPartition, 0, &data, 1);  
      }
    }
  }
  Serial.printf("get index : page:%i index:%i\n\r", *page, *index);
}

// trouve index dans page 0 ou 1
uint16_t trouve_index (uint8_t page)
{
  //esp_err_t err;
  uint16_t index=0;

    char buffer[4];
    uint16_t i;
    for (i=1; i<=NB_ENTRY; i++)
    {
      int addr = (page * PAGE_SIZE) + (i * LOG_ENTRY_SIZE);
      esp_partition_read(logPartition, addr, buffer, 4);
      if ((buffer[0] == 0xFF) && (buffer[1] == 0xFF) && (buffer[2] == 0xFF) && (buffer[3] == 0xFF)) {  break; }
    }
    index = i;
    return index;
}

void chgtPage(void)
{
  // La page est pleine, on bascule sur l'autre 1er octet : :0:pleine FF:vierge 01:en cours

  uint8_t data=0;  // page pleine
  esp_partition_write(logPartition, (activePage-1)*PAGE_SIZE, &data, 1);  // Désactiver l'ancienne page => Ecriture flag 0 = pleine
  activePage = 3 - activePage;  // 1->2   2->1
  esp_partition_erase_range(logPartition, (activePage-1)*PAGE_SIZE, PAGE_SIZE);
  Serial.printf("page effacee:%i\n\r", activePage);
  data = 1;
  esp_partition_write(logPartition, (activePage-1)*PAGE_SIZE, &data, 1);  // Indiquer qu'elle est en cours flag=1
  activeIndex = 1;
}

// ecriture d'un log de 16 caractères : Timestamp(4), code, C1, C2, C3, 8 caractères
void writeLog(uint8_t code, uint8_t c1, uint8_t c2, uint8_t c3, const char* message)
{
  if ((!log_err) && (logPartition))
  {
    if ((!activePage) || (!activeIndex))
        getActiveIndex(&activePage, &activeIndex);

    int addr = (activePage-1)*PAGE_SIZE + (activeIndex * LOG_ENTRY_SIZE);
    if (addr + LOG_ENTRY_SIZE >= activePage*PAGE_SIZE) {
      // page pleine 
      chgtPage();
      addr = (activePage-1)*PAGE_SIZE + (activeIndex * LOG_ENTRY_SIZE);
    }

    LogEntry log = {0};  // initialise toute la structure à 0, y compris message
    getLocalTime(&timeinfo,100);
    time_t timestamp = mktime(&timeinfo);   // Convertit struct tm en timestamp
    log.timestamp = static_cast<uint32_t>(timestamp);  // cast explicite sur 4 octets
    //log.timestamp = millis();  // ou un timestamp réel si tu as l'heure
    log.code = code;
    log.c1 = c1;
    log.c2 = c2;  
    log.c3 = c3;
    uint8_t size = sizeof(log.message);
    //uint8_t s1 = size;
    if (strlen(message) < sizeof(log.message)) size = strlen(message)+1;
    if (code=='B')  // Si binaire => longueur en c1
    {
        if (c1 < sizeof(log.message))  size = c1;  // tableau binaire
    }

    //Serial.printf("taille:%d %d\n\r", s1, size);
    memcpy(log.message, message, size);

    // nota : la fonction écrit par bloc de 4 octets => il faut que ce soit un multiple de 4

    esp_err_t err = esp_partition_write(logPartition, addr, &log, LOG_ENTRY_SIZE);
    //esp_err_t err = ESP_OK;

    if (err == ESP_OK) {
      Serial.printf("Log écrit !, size:%d\n\r", size);
      activeIndex++;  // Avance pour le prochain log
    } else {
      Serial.printf("Erreur écriture log: %s\n", esp_err_to_name(err));
    }
  }
}

// return : nb de log lus
// buffer:0 dernier log, ....
int readLastLogsBinary(uint8_t *buffer, int nombre)
{
  int i=0;
  if (!log_err)
  {
    if ((!activePage) || (!activeIndex))
        getActiveIndex(&activePage, &activeIndex);
    Serial.printf("Lecture de %i logs depuis la page %i  index:%i\n\r", nombre, activePage, activeIndex);
    delay(50);
    esp_err_t err;
    uint8_t page = activePage;
    uint16_t index = activeIndex;

    int addr =  index * LOG_ENTRY_SIZE;
    int addrlect;

    
    for (i = 0; i < nombre; i++)
    {
      addr -= LOG_ENTRY_SIZE;
      if (addr < LOG_ENTRY_SIZE) // chgt page
      {
        page = 3 - activePage;
        uint8_t flag1;
        esp_partition_read(logPartition, (page-1)*PAGE_SIZE, &flag1, 1);
        //Serial.printf("chgt page: page:%d flag:%d\n\r", (page-1), flag1); 
        if (!flag1) // page pleine
        {
          addr = (NB_ENTRY-1)*LOG_ENTRY_SIZE; // dernier index
          //Serial.printf("chgt page: page:%d addr:%d\n\r", (page-1), addr); 
        } 
        else // page vierge ou non formattée
        {
          Serial.println("chgt page: break"); 
          break; // fin des logs dispo
        }
      }
      addrlect = (page-1)*PAGE_SIZE + addr;
      LogEntry log;
      err = esp_partition_read(logPartition, addrlect, &log, LOG_ENTRY_SIZE);
      if (err != ESP_OK) 
        { Serial.println("Erreur de lecture.");  }
      if (log.timestamp == 0xFFFFFFFF) 
        break;  // Fin des logs
      memcpy(buffer + i * LOG_ENTRY_SIZE, &log, LOG_ENTRY_SIZE);
      // Conversion timestamp vers date lisible
      time_t ts = log.timestamp;
      struct tm *tm_info = localtime(&ts);

      char dateBuf[21];
      strftime(dateBuf, sizeof(dateBuf), "%d/%m/%Y %H:%M:%S", tm_info);

      Serial.printf("Log %d-%d  %d: %s :   %c%d, val = %d %d",
                    page, addr/LOG_ENTRY_SIZE, i, dateBuf, log.code, log.c1, log.c2, log.c3);

      if (log.code=='B')  // données binaires
      {
        uint8_t count = log.c1;
        for (int j = 0; j < count; j++)
        {
          int base = j ;
          if (base >= sizeof(log.message)) break;
          uint8_t pc = ((uint8_t)log.message[base]);
          Serial.printf("0x%02x ", pc);
          delay(50);
        }
        Serial.printf("\n\r");
      }
      else
      {
        Serial.printf("%.*s\n\r", LOG_ENTRY_SIZE-8, log.message);
        //Serial.printf("Log %d-%d  %d: %s :   %c%d, val = %d %d  %.*s\n\r",
                  //  page, addr/LOG_ENTRY_SIZE, i, dateBuf, log.code, log.c1, log.c2, log.c3, 8, log.message);
        vTaskDelay(pdMS_TO_TICKS(10));  // Délai de 30 ms
        Serial.flush();
        //delay(100);
      }
    }
    Serial.printf("nb:%i\n\r",i);
  }
  return i;
}

// trouve page flash en cours et index dans la page 
void getActiveIndexG(uint8_t * page, uint16_t *index) 
{
  uint8_t flag1, flag2;  // FF:vierge  0:plein 1:en cours
  uint8_t data;
  esp_partition_read(logPartitionG, 0, &flag1, 1);
  esp_partition_read(logPartitionG, PAGE_SIZEG, &flag2, 1);

  if (flag1 == 0xFF && flag2 == 0xFF)  // Page flash vierge
  {
    // on active la premiere page
    data=1;
    esp_partition_write(logPartitionG, 0, &data, 1);
    *page=1;
    *index=1;
  }
  else
  {
    if (flag1 == 0x01) // en cours
    {
      *page = 1;
      *index = trouve_indexG(0);
    }
    else
    {
      if (flag2 == 0x01) // en cours
      {
        *page = 2;
        *index = trouve_indexG(1);
      }
      else
      {
        *page=1;  // efface page 1
        *index=1;
        esp_partition_erase_range(logPartitionG, 0, PAGE_SIZEG);
        Serial.println("page 1 G effacee");
        data = 1;
        esp_partition_write(logPartitionG, 0, &data, 1);  
      }
    }
  }
  Serial.printf("get index G : page:%i index:%i\n\r", *page, *index);
}

// trouve index dans page 0 ou 1
uint16_t trouve_indexG (uint8_t page)
{
  //esp_err_t err;
  uint16_t index=0;

    char buffer[4];
    uint16_t i;
    for (i=1; i<=NB_ENTRY; i++)
    {
      int addr = (page * PAGE_SIZEG) + (i * LOG_ENTRY_SIZEG);
      esp_partition_read(logPartitionG, addr, buffer, 4);
      if ((buffer[0] == 0xFF) && (buffer[1] == 0xFF) && (buffer[2] == 0xFF) && (buffer[3] == 0xFF)) {  break; }
    }
    index = i;
    return index;
}


void chgtPageG(void)
{
  // La page est pleine, on bascule sur l'autre 1er octet : :0:pleine FF:vierge 01:en cours

  uint8_t data=0;  // page pleine
  esp_partition_write(logPartitionG, (activePageG-1)*PAGE_SIZEG, &data, 1);  // Désactiver l'ancienne page => Ecriture flag 0 = pleine
  activePageG = 3 - activePageG;  // 1->2   2->1
  esp_partition_erase_range(logPartitionG, (activePageG-1)*PAGE_SIZEG, PAGE_SIZEG);
  Serial.printf("page effacee:%i\n\r", activePageG);
  data = 1;
  esp_partition_write(logPartitionG, (activePageG-1)*PAGE_SIZEG, &data, 1);  // Indiquer qu'elle est en cours flag=1
  activeIndexG = 1;
}

// ecriture d'un event  de 16 caractères : Timestamp(4), code, 3 fois uint16_t
void writeLogG(uint8_t code, uint16_t c1, uint16_t c2, uint16_t c3)
{
  if (!log_errG)
  {
    if ((!activePageG) || (!activeIndexG))
        getActiveIndexG(&activePageG, &activeIndexG);

    int addr = (activePageG-1)*PAGE_SIZEG + (activeIndexG * LOG_ENTRY_SIZEG);
    if (addr + LOG_ENTRY_SIZEG >= activePageG*PAGE_SIZEG) {
      // page pleine 
      chgtPageG();
      addr = (activePageG-1)*PAGE_SIZEG + (activeIndexG * LOG_ENTRY_SIZEG);
    }

    LogEntryG log = {0};  // initialise toute la structure à 0, y compris message
    getLocalTime(&timeinfo);
    time_t timestamp = mktime(&timeinfo);   // Convertit struct tm en timestamp
    log.timestamp = static_cast<uint32_t>(timestamp);  // cast explicite sur 4 octets
    //log.timestamp = millis();  // ou un timestamp réel si tu as l'heure
    log.code = code;
    log.c1 = c1;
    log.c2 = c2;  
    log.c3 = c3;

    // nota : la fonction écrit par bloc de 4 octets => il faut que ce soit un multiple de 4

    esp_err_t err = esp_partition_write(logPartitionG, addr, &log, LOG_ENTRY_SIZEG);
    //esp_err_t err = ESP_OK;

    if (err == ESP_OK) {
      Serial.println("Log écrit_G !");
      activeIndexG++;  // Avance pour le prochain log
    } else {
      Serial.printf("Erreur écriture log_G: %s\n", esp_err_to_name(err));
    }
  }
}

// Fonction pour lire les dernières données graphiques depuis la flash
int readLastLogsG(int nombre)
{
  int i = 0;
  if (!log_errG)
  {
    if ((!activePageG) || (!activeIndexG))
        getActiveIndexG(&activePageG, &activeIndexG);
    
    //Serial.printf("LogG:Page:%i index:%i\n\r", activePageG, activeIndexG);

    uint8_t page = activePageG;
    uint16_t index = activeIndexG;
    int addr = index * LOG_ENTRY_SIZEG;
    
    for (i = 0; i < nombre && i < NB_Val_Graph; i++)
    {
      //Serial.printf("debut:%i   ", i);
      addr -= LOG_ENTRY_SIZEG;
      if (addr < LOG_ENTRY_SIZEG) // chgt page
      {
        //Serial.printf("chgt de page %i\n\r", addr);
        //delay(100);

        page = 3 - activePageG;
        uint8_t flag1;
        esp_partition_read(logPartitionG, (page-1)*PAGE_SIZEG, &flag1, 1);
        if (!flag1) // page pleine
        {
          addr = (NB_ENTRYG-1) * LOG_ENTRY_SIZEG; // dernier index
        } 
        else // page vierge
        {
          break; // fin des logs
        }
      }
      int addrlect = (page-1)*PAGE_SIZEG + addr;
      //Serial.printf("lecture %i\n\r", addrlect);
      //delay(100);
      LogEntryG log;
      esp_partition_read(logPartitionG, addrlect, &log, LOG_ENTRY_SIZEG);
      //Serial.printf("%u c1:%i c2:%i c3:%i\r\n", log.timestamp, log.c1, log.c2, log.c3);
      if (log.timestamp == 0xFFFFFFFF) 
        break;  // Fin des logs
      
      // Assigner aux tableaux graphique
      graphique[i][3] = log.c1;
      graphique[i][4] = log.c2;
      graphique[i][5] = log.c3;
    }
    //Serial.printf("fin %i\r\n", i);
    //delay(100);
  }
  return i;
}

void init_time_ps() 
{
  if ((init_time == 2) || (init_time == 1))  // temps initialisé avec température ou 8h
  {
    // si test de connexion internet => init_time=3:
   /* #ifndef MODE_Wifi
      if (eth_connected) {
        init_time = 3;
        Serial.println("eth ok : init_time=3");
      }
    #endif  */
  }

  if (!getLocalTime(&timeinfo))
  {  // toujours pas initialisé
    Serial.println("Failed to obtain time : init=0");
    // au bout de 6 minutes : initialisation quand meme avec heure par défaut
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis_inittime >= (6*60 * 1000)) && (!init_time))
    {
      Serial.println("Failed to obtain time : initialisation quand meme à 20h");
      init_time = 1;
      timeinfo.tm_hour = 20;
      timeinfo.tm_min = 0;
      timeinfo.tm_sec = 0;
      timeinfo.tm_year = 126;
      timeinfo.tm_mon = 1;      // Février (0=Jan, 1=Fév)
      timeinfo.tm_mday = 1;     // 1er jour du mois

      const time_t sec = mktime(&timeinfo);  // make time_t
      timeval tv;
      tv.tv_sec = sec;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      init_time=1;  // initialisé a 20h 1/2/2026
      // verification de l'heure toutes les 30 minutes
      xTimerChangePeriod(xTimer_Init,30*60*(1000/portTICK_PERIOD_MS),100); 
      xTimerStart(xTimer_Init,100);
    }
  }
  else  // lecture correcte de l'heure : Nota : si l'heure/date est mise à jour manuellement, alors "ok init time"
  {
    Serial.println("ok init time");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    init_time = 3;
    //delay(2000 + random(1, 1001) );
    writeLog('S', 1, init_time, 0, "Ini-heur");
    delay(1000);

    periode_lecture_heure = 30;  // 30 secondes

    // sauvegarde en NVS
    time_t sec = mktime(&timeinfo);  // nb de secondes
    time_reset0 = preferences_nvs.getUInt("time_reset0", 0);
    int32_t duree_reset;                  // duree du precedent fct, en minutes
    duree_reset = sec / 60 - time_reset0;  // calcul duree fonctionnement précédent, en minutes
    if (duree_reset<0) duree_reset=0;
    int min = duree_reset % 60;            //
    int hour = (duree_reset / 60) % 24;
    int day = (duree_reset / 60 / 24);
    snprintf(duree_RESET, sizeof(duree_RESET), "%d jours %d heures %d min", day, hour, min);

    Serial.printf("Precedent reset(en min):%lu\n\r", (uint32_t)duree_reset);
    preferences_nvs.putUInt("time_reset0", sec / 60);
  }
}

void loop()
{
  #ifdef WatchDog
    esp_task_wdt_reset();
  #endif

  if (debounceFlag) {
      debounceFlag = false;
      //Serial.println("Debounce timer callback");
  }

  //Serial.printf("PIN_4 state = %d\n", digitalRead(PIN_REVEIL));

  //heap_caps_check_integrity_all(true);  // place ce test dans ton loop()

  if (otaEnabled)
  {
    ArduinoOTA.handle();

    if (millis() - otaStartTime > 120000) {
      otaEnabled = false;
    }
  }

  #ifdef ESP_VEILLE
    // Si on est en mode "Stay Awake" (réveil par bouton), on attend 30s
    if (force_stay_awake) {
      if (millis() > wake_up_time) {
         Serial.println("Délai de configuration de 30s expiré. Passage en Deep Sleep...");
         delay(100);

         uint64_t sleep_time = (uint64_t)periode_cycle * 60 * 1000000;
         if (mode_rapide==12)
          sleep_time = (uint64_t)periode_cycle * 1000000;
          passage_deep_sleep(sleep_time);  // 30ULL * 1000000ULL); //
      }
    }
  #endif

  //Serial.printf(".");
  //vTaskDelay(temps_boucle_loop*1000 / portTICK_PERIOD_MS); // Petite pause
  vTaskDelay(100); // Petite pause

}

void passage_deep_sleep(uint64_t temps)
{
  uint64_t sleep_us = min(temps, 60ULL * 60ULL * 1000000ULL);

  Serial.printf("PIN_REVEIL state = %d %d\n", digitalRead(PIN_REVEIL), gpio_get_level((gpio_num_t)PIN_REVEIL));
  Serial.flush();

  Serial.printf("Passage deep sleep pour %llu\n", (unsigned long long)sleep_us);

  esp_sleep_enable_timer_wakeup(temps);

  #ifdef ESP32_v1
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_REVEIL, 0); // Réveil par bouton (0 = bas)
  #endif
  #ifdef ESP32_uPesy
    //rtc_gpio_pullup_en((gpio_num_t)(PIN_REVEIL));  // pull-up actif en deep sleep : GPIO4=RTC10
    //esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_REVEIL, 0); // Réveil par bouton (0 = bas)
    //rtc_gpio_pulldown_en((gpio_num_t)(PIN_REVEIL));  // pull-up actif en deep sleep : GPIO12=RTC14
    //rtc_gpio_pullup_dis((gpio_num_t)(PIN_REVEIL));  // pull-up actif en deep sleep : GPIO12=RTC14
     //delay(10);
    /*gpio_pulldown_en((gpio_num_t)(PIN_REVEIL));  // pull-up actif en deep sleep : GPIO4=RTC10
    gpio_pulldown_en((gpio_num_t)(PIN_REVEIL2));  // pull-up actif en deep sleep : GPIO12=RTC14
    delay(10);*/
    //uint64_t mask = (1ULL << PIN_REVEIL); // | (1ULL << PIN_REVEIL2);
    //esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    //esp_sleep_enable_ext1_wakeup_io(mask, ESP_EXT1_WAKEUP_ANY_HIGH); // Réveil par bouton (niveau bas)
    gpio_config_t config;
      config.pin_bit_mask = ((1ULL << PIN_REVEIL));// | (1ULL << PIN_REVEIL2));
      #ifdef PIR_ACTIF
        config.pin_bit_mask = ((1ULL << PIN_REVEIL) | (1ULL << PIN_REVEIL2));
      #endif
      config.mode = GPIO_MODE_INPUT;
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      config.intr_type = GPIO_INTR_HIGH_LEVEL; // niveau haut pour réveil
      gpio_config(&config);

      // Activer le réveil GPIO
      //esp_sleep_enable_gpio_wakeup();
      esp_sleep_enable_ext1_wakeup(config.pin_bit_mask, ESP_EXT1_WAKEUP_ANY_HIGH);

      Serial.println("Going to sleep...");
  #endif

  #ifdef ESP32_S3   // S3
    rtc_gpio_deinit((gpio_num_t)PIN_REVEIL);
    gpio_config_t config = {
      .pin_bit_mask = (1ULL << PIN_REVEIL),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,   // ACTIVATION PULLUP CRITIQUE
      .pull_down_en = GPIO_PULLDOWN_ENABLE,
      .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config);


    // config RTC GPIO
    rtc_gpio_set_direction((gpio_num_t)PIN_REVEIL, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis((gpio_num_t)PIN_REVEIL);
    rtc_gpio_pullup_en((gpio_num_t)PIN_REVEIL);

    esp_sleep_enable_ext1_wakeup( (1ULL << PIN_REVEIL),
    ESP_EXT1_WAKEUP_ANY_HIGH);   // réveil si pin = High

    //gpio_wakeup_disable((gpio_num_t)PIN_REVEIL);
    //esp_sleep_enable_gpio_wakeup();
    //gpio_wakeup_enable((gpio_num_t)PIN_REVEIL, GPIO_INTR_LOW_LEVEL);
    delay(50);
  #endif


  #ifdef ESP32_Fire2  // Firebeetle
    // 2. Configurer le réveil par GPIO pour ESP32-C6
    // Sur C6, on active le wake-up sur le niveau BAS (LOW)
    gpio_config_t config = {
      .pin_bit_mask = (1ULL << PIN_REVEIL),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,   // ACTIVATION PULLUP CRITIQUE
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_LOW_LEVEL
    };
    gpio_config(&config);

    esp_deep_sleep_enable_gpio_wakeup( 1ULL << PIN_REVEIL, ESP_GPIO_WAKEUP_GPIO_LOW);
  #endif
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.printf("GPIO state: %d\n", gpio_get_level((gpio_num_t)PIN_REVEIL));
  Serial.flush();
  delay(500);
  yield();

  esp_deep_sleep_start();
}


// permt de test que mon serveur répond bien , en boucle locale
// err:0:ok  1:non connecté, 2:erreur serveur
uint8_t testHttpServerLocal() {
  uint8_t err=1;
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + WiFi.localIP().toString() + "/verif";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200)
    {
      String payload = http.getString();
      Serial.println("[Test HTTP] Serveur OK: " + payload);
      err=0;
    } 
    else
    {
      Serial.printf("[Test HTTP] Erreur serveur: Code %d\n", httpCode);
      err=2;
    }
    http.end();
  } 
  else
  {
    Serial.println("[Test HTTP] WiFi non connecté !");
  }
  return err;
}

uint8_t testConnexionGoogle() {
  uint8_t err=1;
  if (WiFi.status() == WL_CONNECTED)
  {
    err=0;
    HTTPClient http;

    // ⚠️ Utilise http:// si tu veux éviter le SSL (HTTPS)
    http.begin("http://www.google.com");

    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("[HTTP] Réponse code : %d\n", httpCode);
      String payload = http.getString();
      Serial.println(payload.substring(0, 200)); // Affiche les 200 premiers caractères
    } else {
      Serial.printf("[HTTP] Échec, erreur : %s\n", http.errorToString(httpCode).c_str());
      err=2;
    }

    http.end();
  } 
  else
  {
    Serial.println("Wi-Fi non connecté !");
  }
  return err;
}


// 0:ok, 1:err
uint8_t reConnectWifi()
{
  uint8_t err=0;
  Serial.println("deconnexion wifi. Tentative de reconnexion...");

  WiFi.disconnect();  // optionnel, pour forcer la reconnexion propre
  delay(1000);
  WiFi.begin(nom_routeur, mdp_routeur);

  unsigned long start = millis();
  const unsigned long timeout = 15000; // 15 secondes max

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout)
  {
    delay(500);
    Serial.print(".");
    
    // Reset du watchdog pendant la reconnexion WiFi
    #ifdef WatchDog
      esp_task_wdt_reset();
    #endif
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Reconnecté !");
    Serial.print("[WiFi] Adresse IP : ");
    Serial.println(WiFi.localIP());
    // Optimisation consommation : activation du Modem Sleep après reconnexion
    WiFi.setSleep(false);
  } else
  {
    Serial.println("\n[WiFi] Échec de reconnexion.");
    err=1;
    // Optionnel : gérer redémarrage ou nouvelle tentative plus tard
  }
  return err;
}

void startWebServer() {
  setupRoutes();
  server.begin();
  Serial.println("[HTTP] Serveur démarré.");
}

void setupRoutes()
{
    /* url
    /  : index
    /Get : lecture de la valeur d'un registre
    /Set : Ecriture de la valeur d'un registre
    /status : requet Get-status pour récuperer toutes les valeurs
  */

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);  // processor);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Favicon envoye");
    request->send(200, "image/x-icon", favicon, sizeof(favicon));
  });

server.on("/verif", HTTP_GET, [](AsyncWebServerRequest *request){
  Serial.printf("/verif callback sur le cœur %d\n", xPortGetCoreID());
  request->send(200, "text/plain", "OK");
});

  // Attention : comme il n'y a pas de fractionnement de l'envoi, le max est de 2000 caractères
  server.on("/GetLogs", HTTP_GET, [](AsyncWebServerRequest *request){
    int count = 10;  // Valeur par défaut

    if (request->hasParam("count")) {
        count = request->getParam("count")->value().toInt();
        // Limitation pour éviter le buffer overflow : MAX_DUMP / LOG_ENTRY_SIZE = 82 logs max
        int maxLogs = MAX_DUMP / LOG_ENTRY_SIZE -3;
        if (count <= 0 || count > maxLogs) count = maxLogs; // borne de sécurité
      }

    int nbLogs = readLastLogsBinary((uint8_t*)buffer_dmp, count);

    request->send(200, "application/octet-stream", (const uint8_t*)buffer_dmp, nbLogs * LOG_ENTRY_SIZE);
  });


  // requete get pour mettre à jour la page web
  // get?type=1&reg=consigne  type:1:champs, 2:reg 3:registre modbus  
  // renvoie : "reg"="xxx","val"="111.1"
  server.on("/Get", HTTP_GET, [](AsyncWebServerRequest *request) {
    static char json_response[80];
    uint8_t res = 1;
    float valeur = 17;
    char valeur_S[50]="";
    String reg;
    uint8_t type=0;

    #ifdef ESP_VEILLE
      // Si une commande /get arrive, on force le réveil si ce n'est pas déjà fait
      force_stay_awake = true;
      wake_up_time = millis() + 40000;  // prolongation si requete get
      Serial.println("Activité /get détectée : prolongation du délai de 30s.");
    #endif

    if ((request->hasParam("type")) && (request->hasParam("reg")))
    {
      type = request->getParam("type")->value().toInt();
      reg = request->getParam("reg")->value();
      const char* reg_cstr = reg.c_str();
      if (type<4) {
        res=requete_Get(type, reg_cstr, &valeur);
        Serial.printf("res:%i type:%i reg=%s val=%f\n\r", res, type, reg.c_str(), valeur);
      }
      if (type==4) {
        res=requete_Get_String(type, reg_cstr, valeur_S);
        Serial.printf("res:%i type:%i reg=%s val=%s\n\r", res, type, reg.c_str(), valeur_S);
      }

    }

    if (!res) {
      char *p = json_response;
      *p++ = '{';
      if (type==1)  p += snprintf(p, json_response + sizeof(json_response) - p -2, "\"reg\":\"%s\",", reg.c_str());
      if (type==2)  p += sprintf(p, "\"reg\":\"get-reg-value\",");
      if (type==3)  p += sprintf(p, "\"reg\":\"get-regM-value\",");
      if (type==4)  p += sprintf(p, "\"reg\":\"get-regT-value\",");

      if (type<4)   p += sprintf(p, "\"val\":%f,", valeur);
      if (type==4)
          p += snprintf(p, sizeof(json_response) - (p - json_response) -2, "\"val\":\"%s\",", valeur_S);
      p--;
      *p++ = '}';
      *p++ = 0;
      //#ifdef DEBUG
      Serial.println(json_response);
      //delay(200);
      //#endif
      request->send(200, "application/json", json_response);
    } else {
      request->send(400, "text/plain", "erreur");
    }
  });



  // SET - suite à mise à jour de la valeur sur page web => mise à jour dans le code principal
  // set?type=1&reg=consigne&val=13  type:1:champs, 2:reg 3:registre modbus 4:texte 5:action
  server.on("/Set", HTTP_GET, [](AsyncWebServerRequest *request) {
    //int paramsNr = request->params();  // Nombre de paramètres = 2
    //AsyncWebParameter* p = request->getParam(0); premier parametre

    int res = 1;  // 0:ok  1:erreur
    uint8_t res2=1;
    uint8_t type=0;
    String reg;
    #define JSON_BUF_SIZE (MAX_DUMP + 50)  // marge de sécurité
    static char json_response[JSON_BUF_SIZE];

    #ifdef ESP_VEILLE
      // Si une commande /set arrive, on force le réveil si ce n'est pas déjà fait
      force_stay_awake = true;
      wake_up_time = millis() + 40000;  // prolongation si requete set
      Serial.println("Activité /set détectée : prolongation du délai de 30s.");
    #endif

    buffer_dmp[0]=0;
      char *p = json_response;

    // Vérification origine
    IPAddress clientIP = request->client()->remoteIP();
    if (clientIP[0] == 192 && clientIP[1] == 168  && clientIP[3] != 1)
    {
        #ifndef Sans_securite
          xTimerStop(xTimer_Securite,100);
          xTimerChangePeriod(xTimer_Securite,(uint32_t)2*60*(1000/portTICK_PERIOD_MS),100);
          xTimerStart(xTimer_Securite,100);  // permet de couper la securite au bout de 2 minutes
        #endif
        cpt_securite = 1;
        //Serial.println("Requête depuis LAN");
    }

    if ((request->hasParam("type")) && (request->hasParam("reg")) && (request->hasParam("val")))
    {
      type = request->getParam("type")->value().toInt();
      reg = request->getParam("reg")->value();
      String valStr = request->getParam("val")->value();
      res = requete_Set(type, reg.c_str(), valStr.c_str());
      Serial.printf("Set: res:%i type:%i %s=%s \n\r", res, type, reg.c_str(), valStr.c_str());

      float valeur=0;
      char valeur_S[50]="";

      if (type<4) res2 = requete_Get(type, reg.c_str(), &valeur);
      else 
        if (type==4) res2 = requete_Get_String(type, reg.c_str(), valeur_S);

      *p++ = '{';
      size_t espace = JSON_BUF_SIZE - (p - json_response) -2;
      if (type==1)  p += snprintf(p, espace, "\"reg\":\"%s\",", reg.c_str());
      if (type==2)  p += sprintf(p, "\"reg\":\"get-reg-value\",");
      if (type==3)  p += sprintf(p, "\"reg\":\"get-regM-value\",");
      if (type==4)  p += sprintf(p, "\"reg\":\"get-regT-value\",");

      espace = JSON_BUF_SIZE - (p - json_response);  // recalcul
      if (type<4)   p += snprintf(p, espace, "\"val\":%f,", valeur);
      else  if (type==4)  p += snprintf(p, espace, "\"val\":\"%s\",", valeur_S);
      p--;
      *p++ = '}';
      *p++ = 0;
      if (type==5) 
      { 
        snprintf(json_response, sizeof(json_response), "{\"val\":\"%s\"}", buffer_dmp);
        res2=0;
      }
      Serial.printf("Retour Get: res2:%i %s=%f type:%i %s\n", res2, reg.c_str(), valeur, type, json_response);
    }

    if (!res2)  // 0:ok  1:erreur
    {
      if (type<5)
          request->send(200, "application/json", json_response);
      else
        request->send(200, "text/plain", buffer_dmp);
    }
    else
      request->send(400, "text/plain", "erreur");
  });

  // requette Status pour récuperer toutes les valeurs
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    //static char json_response[MAX_DUMP];
    uint8_t type=1;

    if ( request->hasParam("type") )
      type = request->getParam("type")->value().toInt();


    /*IPAddress clientIP = request->client()->remoteIP();

    #ifdef DEBUG
        Serial.print("Client IP : ");
        Serial.println(clientIP);
    #endif

    // Vérification origine
    uint8_t orig=1;  // 0:local
    if (clientIP[0] == 192 && clientIP[1] == 168  && clientIP[3] != 1) {
        Serial.println("Requête depuis LAN");
        orig=0;
    }

    if (clientIP == IPAddress(192,168,251,1)) {
        Serial.println("Requête via routeur (probablement externe)");
    }

    if (!orig) // suppression securite
    {
        #ifndef Sans_securite
          xTimerStop(xTimer_Securite,100);
          xTimerChangePeriod(xTimer_Securite,(uint32_t)2*60*(1000/portTICK_PERIOD_MS),100);
          xTimerStart(xTimer_Securite,100);  // permet de couper la securite au bout de 2 minutes
          cpt_securite = 1;
        #endif
    }*/

    requete_status(buffer_dmp, 0, type);

    #ifdef DEBUG
        Serial.println(buffer_dmp);
    #endif
    request->send(200, "application/json", buffer_dmp);
  });


  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
}

const char* dumpTasksInfo() {

/*  TaskStatus_t taskStatusArray[24];  // 24 tâches max
  UBaseType_t taskCount;
  uint32_t totalRunTime;

  size_t pos = 0;

  taskCount = uxTaskGetSystemState(taskStatusArray, 24, &totalRunTime);

  if (taskCount == 0) {
    snprintf(buffer_dmp, MAX_DUMP, "Erreur : aucune tâche trouvée\n");
    return buffer_dmp;
  }

  // En-tête
  const char* header = "Name             State Prio Stack Core\n"
                       "----------------------------------------\n";
  size_t headerLen = strlen(header);
  memcpy(buffer_dmp, header, headerLen);
  pos = headerLen;

  for (UBaseType_t i = 0; i < taskCount; i++) {
    const TaskStatus_t& ts = taskStatusArray[i];

    const char* stateChar = "?";
    switch (ts.eCurrentState) {
      case eRunning:    stateChar = "R"; break;
      case eReady:      stateChar = "r"; break;
      case eBlocked:    stateChar = "B"; break;
      case eSuspended:  stateChar = "S"; break;
      case eDeleted:    stateChar = "D"; break;
      case eInvalid:    stateChar = "I"; break;
    }

    int written = snprintf(buffer_dmp + pos, MAX_DUMP - pos,
                           "%-17s %s     %2u   %5u   %d\n",
                           ts.pcTaskName,
                           stateChar,
                           (unsigned int)ts.uxCurrentPriority,
                           (unsigned int)ts.usStackHighWaterMark,
                           ts.xCoreID);

    if (written <= 0 || (pos + written >= MAX_DUMP - 2)) {
      snprintf(buffer_dmp+ pos, MAX_DUMP - pos, "\n[Tronqué...]\n");
      break;
    }

    pos += written;
  }

  if (taskCount == 24) {
    snprintf(buffer_dmp + pos, MAX_DUMP - pos, "[Attention: 24 taches atteint]\n");
  }*/

  return buffer_dmp;
}

// enregistre les log dans la flash
int myLogPrinter(const char *fmt, va_list args) {
  static char buffer[256];  // Attention à ne pas allouer dynamiquement dans les logs
  vsnprintf(buffer, sizeof(buffer), fmt, args);

  Serial.print("LOG ");
  Serial.print(buffer);  // Affiche toujours sur port série

  // Enregistre aussi dans le système de logs custom
  char logBuffer[9]; // 8 caractères + '\0'
  strncpy(logBuffer, buffer, 8);
  logBuffer[8] = '\0';  // Toujours terminer par un nul

  writeLog('E', 0, 0, 0, logBuffer);

  return strlen(buffer);  // Retour attendu
}

void modif_timer_cycle(void)
{
    xTimerStop(xTimer_Cycle,100);
    uint16_t perio = periode_cycle*60;  // minutes -> secondes
    if (mode_rapide==12) perio = periode_cycle;   // en secondes
    xTimerChangePeriod(xTimer_Cycle,(uint32_t)perio*(1000/portTICK_PERIOD_MS),100);
    xTimerStart(xTimer_Cycle,100);

}

void print_task_states() {
  const UBaseType_t maxTasks = 20; // Nombre maximum de tâches à afficher
  TaskStatus_t taskStatusArray[maxTasks];

  // Récupère l'état des tâches (sans temps CPU)
  UBaseType_t taskCount = uxTaskGetSystemState(taskStatusArray, maxTasks, NULL);

  if (taskCount == 0) {
    Serial.println("Aucune tâche détectée.");
    return;
  }

  // En-tête
  Serial.println("Nom               Etat   Prio Stack");
  Serial.println("************************************");

  for (UBaseType_t i = 0; i < taskCount; i++) {
    char state;

    switch (taskStatusArray[i].eCurrentState) {
      case eRunning:   state = 'R'; break;
      case eReady:     state = 'Y'; break;
      case eBlocked:   state = 'B'; break;
      case eSuspended: state = 'S'; break;
      case eDeleted:   state = 'D'; break;
      default:         state = '?'; break;
    }

    Serial.printf("%-18s %c     %2u   %5u\n",
      taskStatusArray[i].pcTaskName,
      state,
      (unsigned int)taskStatusArray[i].uxCurrentPriority,
      (unsigned int)taskStatusArray[i].usStackHighWaterMark
    );
  }
}

// Fonction de diagnostic WiFi améliorée
void diagnoseWiFiError() {
  Serial.println("\n=== DIAGNOSTIC WiFi DÉTAILLÉ ===");
  
  // 1. Informations de configuration
  Serial.println("1. Configuration WiFi :");
  Serial.printf("   - SSID: %s\n", nom_routeur);
  Serial.printf("   - Mot de passe: %s\n", (strlen(mdp_routeur) > 0) ? "***configuré***" : "NON CONFIGURÉ");
  Serial.printf("   - Mode réseau: %d\n", mode_reseau);
  
  // 2. Configuration IP
  Serial.println("2. Configuration IP :");
  Serial.printf("   - IP statique: %s\n", local_ip.toString().c_str());
  Serial.printf("   - Gateway: %s\n", gateway.toString().c_str());
  Serial.printf("   - Subnet: %s\n", subnet.toString().c_str());
  Serial.printf("   - DNS1: %s\n", primaryDNS.toString().c_str());
  Serial.printf("   - DNS2: %s\n", secondaryDNS.toString().c_str());
  
  // 3. État actuel du WiFi
  Serial.println("3. État WiFi actuel :");
  wl_status_t status = WiFi.status();
  Serial.printf("   - Status: %d (", status);
  switch (status) {
    case WL_IDLE_STATUS: Serial.print("IDLE"); break;
    case WL_NO_SSID_AVAIL: Serial.print("NO_SSID_AVAIL"); break;
    case WL_SCAN_COMPLETED: Serial.print("SCAN_COMPLETED"); break;
    case WL_CONNECTED: Serial.print("CONNECTED"); break;
    case WL_CONNECT_FAILED: Serial.print("CONNECT_FAILED"); break;
    case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
    case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
    case WL_NO_SHIELD: Serial.print("NO_SHIELD"); break;
    default: Serial.print("UNKNOWN"); break;
  }
  Serial.println(")");
  
  // 4. Informations sur les réseaux disponibles
  Serial.println("4. Réseaux WiFi disponibles :");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("   Aucun réseau trouvé");
  } else {
    Serial.printf("   %d réseaux trouvés:\n", n);
    for (int i = 0; i < min(n, 5); ++i) { // Affiche seulement les 5 premiers
      Serial.printf("   - %s (RSSI: %ld, Ch: %ld, %s)\n", 
        WiFi.SSID(i).c_str(), 
        WiFi.RSSI(i), 
        WiFi.channel(i),
        (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Ouvert" : "Sécurisé");
    }
  }
  
  // 5. Diagnostic des erreurs courantes
  Serial.println("5. Diagnostic des erreurs :");
  if (strlen(nom_routeur) == 0) {
    Serial.println("   ❌ ERREUR: SSID non configuré");
  }
  if (strlen(mdp_routeur) == 0) {
    Serial.println("   ❌ ERREUR: Mot de passe non configuré");
  }
  if (status == WL_NO_SSID_AVAIL) {
    Serial.println("   ❌ ERREUR: SSID non trouvé dans la zone");
  }
  if (status == WL_CONNECT_FAILED) {
    Serial.println("   ❌ ERREUR: Échec de connexion (mot de passe incorrect ?)");
  }
  if (status == WL_CONNECTION_LOST) {
    Serial.println("   ❌ ERREUR: Connexion perdue");
  }
  if (local_ip[0] == 0) {
    Serial.println("   ⚠️  ATTENTION: Configuration IP non définie");
  }
  
  // 6. Recommandations
  Serial.println("6. Recommandations :");
  if (status == WL_NO_SSID_AVAIL) {
    Serial.println("   - Vérifiez que le SSID est correct");
    Serial.println("   - Vérifiez que le routeur WiFi est allumé");
    Serial.println("   - Vérifiez la portée du signal WiFi");
  }
  if (status == WL_CONNECT_FAILED) {
    Serial.println("   - Vérifiez le mot de passe WiFi");
    Serial.println("   - Vérifiez le type de sécurité (WPA/WPA2/WEP)");
  }
  if (local_ip[0] == 0) {
    Serial.println("   - Configurez une adresse IP statique");
  }
  
  Serial.println("=== FIN DIAGNOSTIC ===\n");
}

// Fonction de connexion WiFi avec diagnostic amélioré
uint8_t connectWiFiWithDiagnostic() {
  Serial.println("\n=== TENTATIVE DE CONNEXION WiFi ===");
  
  // Vérifications préliminaires
  if (strlen(nom_routeur) == 0) {
    Serial.println("❌ ERREUR: SSID non configuré");
    return 1;
  }
  
  Serial.printf("Connexion au réseau: %s\n", nom_routeur);
    // Configuration WiFi en mode Station pour ESP-NOW
    WiFi.mode(WIFI_STA); 
    
    // 🔍 DIAGNOSTIC: Forcer le canal WiFique si définie
  if (local_ip[0] != 0) {
    Serial.printf("Configuration IP statique: %s\n", local_ip.toString().c_str());
    if (!WiFi.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("❌ ERREUR: Échec de configuration IP statique");
      return 2;
    }
  } else {
    Serial.println("Configuration IP automatique (DHCP)");
  }
  
  // Tentative de connexion
  Serial.println("Tentative de connexion...");
  WiFi.begin(nom_routeur, mdp_routeur);
  
  // Attente de connexion avec feedback
  unsigned long startTime = millis();
  const unsigned long timeout = 15000; // 15 secondes max
  int dots = 0;
  
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
    delay(500);
    Serial.print(".");
    dots++;
    if (dots % 20 == 0) Serial.println(); // Retour à la ligne tous les 10 secondes
    
    // Reset du watchdog pendant la connexion WiFi
    #ifdef WatchDog
      esp_task_wdt_reset();
    #endif
  }
  
  Serial.println(); // Retour à la ligne final
  
  // Vérification du résultat
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi connecté avec succès !");
    Serial.printf("   - Adresse IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("   - RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("   - Canal: %ld\n", WiFi.channel());
    Serial.printf("   - Masque: %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("   - Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("   - DNS: %s\n", WiFi.dnsIP().toString().c_str());
    // Optimisation consommation : activation du Modem Sleep
    WiFi.setSleep(false);
    return 0;
  } else {
    Serial.println("❌ ÉCHEC de connexion WiFi");
    diagnoseWiFiError();
    return 3;
  }
}

// Fonction de protection UART pendant la connexion WiFi
void protectUARTDuringWiFi() {
  // Vider le buffer UART avant la connexion WiFi
  while (Serial.available()) {
    Serial.read();
  }
  
  // Pause pour laisser le temps au WiFi de s'initialiser
  delay(100);
  
  // Reset du watchdog pendant cette opération critique
  #ifdef WatchDog
    esp_task_wdt_reset();
  #endif
}

uint8_t decod_asc8 (const uint8_t * index)
{
	uint8_t val=0;
	for (uint8_t i=0; i<2; i++)
	{
		uint8_t car = *(index+1-i);
		if (( car >='0') && ( car <= '9'))
				val |= ((car-'0')<<(i*4));
		else if (( car >='A') && ( car <= 'F'))
				val |= ((car-'A'+10)<<(i*4));
	}
	return val;
}

uint16_t decod_asc16 (uint8_t * index)
{
	uint16_t val=0;
	for (uint8_t i=0; i<4; i++)
	{
		uint8_t car = *(index+3-i);
		if (( car >='0') && ( car <= '9'))
				val |= ((car-'0')<<(i*4));
		else if (( car >='A') && ( car <= 'F'))
				val |= ((car-'A'+10)<<(i*4));
	}
	return val;
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
//#if defined(ARDUINO_ARCH_ESP32) && defined(WIFI_TX_INFO_T)
void OnDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status)
#else
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status)
#endif
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ackReceived = true;
        ackChannel = WiFi.channel();  // canal courant
    }
}