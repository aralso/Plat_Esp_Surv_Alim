
#include <Arduino.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "variables.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Preferences.h>  // pour nvs eeprom
#include <PID_v1.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "ClosedCube_HDC1080.h"
#include "Wire.h"

extern WiFiClient client;
extern Preferences preferences_nvs;  // Déclaration externe



// variables Detection PIR
RTC_DATA_ATTR uint16_t compteur_detection=0;
RTC_DATA_ATTR uint16_t compteur_detection_1h=0;

RTC_DATA_ATTR uint8_t pause_detection;
RTC_DATA_ATTR unsigned long last_detection_time=0;

RTC_DATA_ATTR uint16_t Nb_PI[NB_VAL_TAB];

RTC_DATA_ATTR uint8_t  WIFI_CHANNEL;
RTC_DATA_ATTR uint8_t etat_now;
RTC_DATA_ATTR uint16_t Seuil_batt_sonde;  // millivolt
RTC_DATA_ATTR uint16_t Seuil_batt_arret_ESP;
RTC_DATA_ATTR uint8_t Nb_jours_Batt_log;
RTC_DATA_ATTR uint16_t prolong_veille;
RTC_DATA_ATTR uint8_t action_stockage;
RTC_DATA_ATTR uint8_t action_envoi, freq_envoi, cpt_envoi;

RTC_DATA_ATTR uint8_t compteur_graph;
RTC_DATA_ATTR uint16_t compteur_24h;



RTC_DATA_ATTR uint8_t mac_gw[6];   // B0:CB:D8:E9:0C:74  adresse mac esp_dest
volatile uint8_t ackReceived = false;  // global pour indiquer que le peer a acké
volatile int ackChannel = -1;       // canal où ça a marché

/* Fonctions Appli

surveillance de la box ;
surveillance d'Internet ;
surveillance du Deco ;
temporisation avant intervention ;
coupure de 10–15 s ;
délai de redémarrage ;
compteur d'incidents ;
limitation des redémarrages ;
bouton manuel ;
petite interface Web éventuellement accessible depuis le réseau.

*/

//void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len);
//void OnDataRecv(const esp_now_peer_info_t * info, const uint8_t *incomingData, int len);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  void OnDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status);
#else
  void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status);
#endif

uint8_t parseMacString(const char* str, uint8_t mac[6]);

uint8_t envoi_now(uint8_t channel, esp_now_peer_info_t * peerInfo, Message_EspNow *message);
uint8_t envoi_data_gateway(Message_EspNow mess_esp);


#ifdef Temp_int_DS18B20
  OneWireNg_CurrentPlatform ow(PIN_DS18B20, false);
  OneWireNg_DS18B20 sensor(&ow);

  // Capteur temperature Dallas DS18B20  Temperature intérieure
  typedef uint8_t DeviceAddress[8];
  const int PIN_Tint = 13;      // Tint:Entrée onewire GPIO DS18B20
  //OneWire oneWire(PIN_Tint);
  //DallasTemperature ds(&oneWire);
  int nb_capteurs_temp = 1;  //DS18B20
  DeviceAddress Thermometer[5];
  DeviceAddress adds;
#endif

DHT dht[] = {
  { PIN_Tint22, DHT22 },
};
#ifdef Temp_int_HDC1080
  ClosedCube_HDC1080 hdc1080;
#endif

// Temperature intérieure
float Tint, Text, Humid;
RTC_DATA_ATTR uint16_t err_Tint, err_Text, err_Heure;  // compteurs d'erreurs

RTC_DATA_ATTR int16_t calib_hygro1=300, calib_hygro2=800, calib_temp=1000; // calibration hygrométrie HDC1080



// ---------FONCTIONS DEFINIES AILLEURS -----------------

int readLastLogsG(int nombre);



// ----------  FONCTIONS APPLI --------------

void init_10_secondes()
{
}

//setup au debut
void setup_0()
{

  #ifdef ESP32_uPesy
    pinMode(PIN_Vbatt, INPUT);
  #endif

}



// setup apres la lecture nvs, avant démarrage reseau
void setup_1()
{

    Tint = 15;
    #ifdef Temp_int_HDC1080
      delay(200);
      //i2cBootRecovery();
      Wire.begin(PIN_SDA, PIN_SCL); // Forçage des pins SDA=8, SCL=9 pour ESP32 S3 DevKit V1

      hdc1080.begin(0x40);
      /*if (i2cDevicePresent(0x40)) {
        Serial.println("HDC1080 détecté");
        hdc1080.begin(0x40);
      } else {
        Serial.println("HDC1080 ABSENT");
      }*/
    #endif

  // initialisation capteur de température intérieur
    #ifdef Temp_int_DHT22
      dht[0].begin();
    #endif
 
 
    #ifdef Temp_int_DS18B20
      ds.begin();  // Startup librairie DS18B20
      nb_capteurs_temp = ds.getDeviceCount();
      Serial.print("Nb Capteurs DS18B20:");
      Serial.println(nb_capteurs_temp);
      if (nb_capteurs_temp > 1) nb_capteurs_temp = 1;
      int j;
      for (j = 0; j < nb_capteurs_temp; j++) {
        Serial.print(" Capteur :");
        ds.getAddress(Thermometer[j], j);
        printAddress(Thermometer[j]);
      }
    #endif

    // lecture initiale temperature interieure
    /*uint8_t Tint_err = lecture_Tint(&Tint);
    if ((Tint < 1) || (Tint > 45)) {
      Tint = 20.0;
      Tint_err = 7;
    }
    if (Tint_err) log_erreur(Code_erreur_Tint, Tint_err, 1);
    else
      Serial.printf("Temp int:%.2f\n\r", Tint);*/

}

// apres demarrage reseau
void setup_2()
{


  #ifdef ESP_TJ_ACTIF

    // lecture des données sauvegardées dans la partition log_flashG
    readLastLogsG(99);

    // Configuration WiFi en mode Station pour ESP-NOW

    if ((mode_reseau==13) )
      WiFi.mode(WIFI_STA);
    
    // 🔍 DIAGNOSTIC: Forcer le canal WiFi
    uint8_t current_channel;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&current_channel, &second);
    Serial.printf("Canal WiFi AVANT config ESP-NOW: %d\n", current_channel);
    
    // Forcer le canal si nécessaire (doit correspondre au routeur)
    // esp_wifi_set_promiscuous(true);
    // esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    // esp_wifi_set_promiscuous(false);
    
    if (esp_now_init() != ESP_OK) {
      Serial.println("Erreur initialisation ESP-NOW");
      return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    
    // Vérifier le canal après init
    esp_wifi_get_channel(&current_channel, &second);
    
    Serial.println("\n\n======================================");
    Serial.println("🔵 ESP-NOW Initialisé (RÉCEPTEUR)");
    Serial.print("   MAC Address module: ");
    //if ((mode_reseau==13) )
    //else
    //  Serial.println(WiFi.softAPmacAddress());
    Serial.println(WiFi.macAddress());

    // Stockage de l'adresse MAC dans le tableau mac_gw[6]
    Serial.printf("   MAC dest : %02X:%02X:%02X:%02X:%02X:%02X\n",
            mac_gw[0], mac_gw[1], mac_gw[2],
            mac_gw[3], mac_gw[4], mac_gw[5]);


    Serial.printf("   Canal WiFi: %d\n", current_channel);
    Serial.println("   En attente de messages...");
    Serial.println("======================================\n\n");
    delay(2000); // 2 secondes de pause pour lire

  #else  // Si veille
      readLastLogsG(99);

  #endif
}



void appli_event_on(systeme_eve_t evt)
{
  Serial.printf("Evenement on : %i\n\r", evt.data);

  if (evt.data == 1)  // bouton 1 appuyé
  {
      envoi_temp_hygro();
  }
}

void detection_pir()
{
    compteur_detection++;  // nb de detection des 5 dernières minutes
    compteur_detection_1h++; // nb de detection de la dernière heure
    if (compteur_detection_1h == 1) // premiere detection du cyle 1h
    {
      writeLog('D', 1, 0, 0, "PIR");
      // envoi d'un sms par http pour prevenir d'une presence
    }
}

void appli_event_off(systeme_eve_t evt)
{
  // Detecteur PIR activé
  if (evt.data == 1)
  {
    detection_pir();
  }
  Serial.printf("Evenement off : %i\n\r", evt.data);
}

// type 1
uint8_t requete_Get_appli(const char* var, float *valeur)
  //uint8_t requete_Get_appli (String var, float *valeur) 
{
  uint8_t res=1;

  if (strncmp(var, "Tint",5) == 0) {
    res = 0;
    *valeur = Tint;
  }
  if (strncmp(var, "Text",5) == 0) {
    res = 0;
    *valeur = Text;
  }
  if (strncmp(var, "codeR_pac",10) == 0) {
    res = 0;
    if (cpt_securite)  *valeur=1;
    else *valeur=0;
  }


  return res;
}




// type 1
uint8_t requete_Set_appli (String param, float valf) 
{
  uint8_t res=1;
  int8_t val = round(valf);

    /*if (param == "consigne")     // Forcage consigne, rajouter duree
    {
      if ((valf >= 6.0) && (valf <= 22.0))  // 6°C à 22°C
      {
          fo_co = round(valf * 10);
          //fo_jus = 10;  // en minutes
          //preferences_nvs.putUChar("Cons", Consigne_G);
          res = 0;
      }
    }*/

    /*if (param == "vbatt")
    {
      res = 0;
      Vbatt_Th = valf;
      Vbatt_Th_I = 1;

      Serial.printf("Réception Vbatt Distante : %.2fV\n", Vbatt_Th);
    }*/


  return res;
}

// type 2
uint8_t requete_GetReg_appli(int reg, float *valeur)
{
  uint8_t res=1;

  // Most numeric parameters are now returned via the generic PARAMS table.
  // Keep here only application-specific dynamic reads that are not present
  // in the PARAMS table.

  
  // Most numeric parameters are now returned via the generic PARAMS table.
  // Keep here only application-specific dynamic reads that are not present
  // in the PARAMS table.

  if (reg == 41)  // registre 41 : canal WiFi actuel (dynamic)
  {
    res = 0;
    uint8_t current_channel;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&current_channel, &second);
    *valeur = (float)current_channel;
  }

  return res;
}

// type 2
uint8_t requete_SetReg_appli(int param, float valeurf)
{
  int16_t valeur = int16_t(round(valeurf));
  uint8_t res = 1;
  (void)valeur;

  return res;
}



// type 4
uint8_t requete_Get_String_appli(uint8_t type, String var, char *valeur)
{
  uint8_t res=1;
  int paramV = var.toInt();
  // valeur limité a 50 caractères
  
  if (paramV == 60)  // registre 60 : adresse MAC ce module
  {
    res = 0;
    strncpy(valeur, WiFi.macAddress().c_str(), 18);
  }
  if (paramV == 61)  // registre 61 : adresse MAC GW
  {
    res = 0;
    snprintf(valeur, 18,
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_gw[0], mac_gw[1], mac_gw[2],
           mac_gw[3], mac_gw[4], mac_gw[5]);
  }

  return res;
}


uint8_t parseMacString(const char* str, uint8_t mac[6]) {
  int v[6];
  if (sscanf(str, "%x:%x:%x:%x:%x:%x",
             &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)v[i];
  return true;
}

// type 4
uint8_t requete_Set_String_appli(int param, const char *texte)
{
  uint8_t res=1;
  IPAddress ip;

    if (param == 12)  // registre 12 : adresse Mac dest
    {
      if (!parseMacString(texte, mac_gw))
      {
          Serial.println("MAC Serveur invalide");
      }
      else
      {
        preferences_nvs.putString("MacC", texte);
        res = 0;
      }
    }

  return res;
}

// type5 : reception message ACTION par uart ou par page web
uint8_t requete_action_appli(const char *reg, const char *data)
{
  uint8_t res=1;

  if (strcmp(reg, "Test1") == 0) 
    { 
      res=0; 
      requete_status(buffer_dmp, 0, 1);
      Serial.println(buffer_dmp);
    }

  if (strcmp(reg, "Tint") == 0) 
    { 
      res=0; 
      uint8_t Tint_erreur = lecture_Tint(&Tint,&Humid);
      Serial.println(Tint_erreur);
      Serial.println(Tint);
    }
  return res;
}

float absoluteHumidity(float T, float RH)
{
    return (13.247f * RH/100 * exp((17.67f * T) / (T + 243.5f)))
           / (273.15f + T);
}

// erreur :0:ok  sinon erreur 2 à 7
uint8_t lecture_Tint(float *mesure, float*humid)
{
  uint8_t Tint_erreur = 7;
  float valeur = 20;
  float valeur2 = 50;


    #ifdef Temp_int_DHT22
      //dht[0].begin();

      if (digitalRead(PIN_Tint22) == HIGH || digitalRead(PIN_Tint22) == LOW)
      {
        valeur =  dht[0].readTemperature();
        if (isnan(valeur))
        {
          valeur = 20.0;
          Tint_erreur = 6;
          Serial.println("---DHT:non numérique");
        }
        else
        {
          Tint_erreur=0;
        }
      }
      else
        Serial.println("---DHT:signal non stable!");
    #endif

    #ifdef Temp_int_HDC1080
      valeur = hdc1080.readTemperature();
      if (isnan(valeur) || (valeur>124)) {
        Serial.println("Reset i2c)");
        resetI2C(); 
        hdc1080.begin(0x40); 
        valeur = hdc1080.readTemperature();

        if (isnan(valeur) || (valeur>124)) {
          Serial.println("Recovery i2c)");
          i2cRecovery();
          hdc1080.begin(0x40); 
          valeur = hdc1080.readTemperature();
          if (isnan(valeur) || (valeur>124)) {
            valeur = 20.0;
            Tint_erreur = 4;
          } else  Tint_erreur=0;
        } else  Tint_erreur=0;

      } else {
        Tint_erreur=0;
      }
      valeur2 = hdc1080.readHumidity();
      Serial.printf("lecture HDC1080 - Temp: %.2f Humid:%.2f\n\r", valeur, valeur2);
      valeur = valeur + (float)(calib_temp-1000)/100.0;  // calibration temperature
      valeur2 = (valeur2 - (float)calib_hygro1/10.0) * (80.0-30.0) / ((float)calib_hygro2/10.0 - (float)calib_hygro1/10.0) + 30.0;  // calibration hygrométrie
      Serial.printf("lecture HDC1080M- Temp: %.2f Humid:%.2f\n\r", valeur, valeur2);

    #endif

    #ifdef Temp_int_DS18B20
      valeur = ds.getTemperature();
      Tint_erreur=0;
    #endif


  if (valeur > 50) Tint_erreur = 2;
  if (valeur < -20) Tint_erreur = 3;
  Serial.printf("lecture Tint : %.2f Humid:%.2f Tint_erreur:%i\n\r", valeur, valeur2, Tint_erreur);
  if (Tint_erreur) {
    valeur = 20.0;
    valeur2 = 50.0;
  }
  *mesure = valeur;
  *humid = valeur2;
  return Tint_erreur;
}



//mesure temperature exterieure
uint8_t lecture_Text(float *mesure) {
  uint8_t Text_erreur = 0;
  int16_t Val_Text = 1600;
  float valeur;

  #ifdef MODBUS
    Text_erreur = read_modbus(2, &Val_Text);  // registre 1-2-3 pour temp exterieure
    valeur = (float)Val_Text / 10;
  #else
    valeur = 18.0;

    /*#ifndef DEBUG_SANS_Sonde_Ext
        Val_Text = analogRead( PIN_Text );  // 0 à 4096
        //Serial.println(Val_Text);
      #endif*/
    // calibration
    // Text1:100(10°C) Text1Val:500
    // Text2:200(20°C) Text2Val:2000
    //valeur = ((float)(Text1Val-Val_Text)/(Text1Val-Text2Val)*(Text2-Text1) + Text1)/10;

    /*float Vmesure = ((float)Val_Text / resolutionADC) * 3.66;
      float Rntc = 15000 * Vmesure / (3.3 - Vmesure);  // Calcul de la résistance de la thermistance
      float T_kelvin = 1.0 / ((1.0 / 298.15) + (1.0 / TBeta) * log(Rntc / Therm0));    // Calcul de la température en Kelvin
      valeur = T_kelvin - 273.15;    // Conversion en °C */
    //Serial.printf("val_text:%i vmesure:%.3f rntc:%.0f T_kelvin:%.1f valeur:%.1f\n", Val_Text, Vmesure, Rntc, T_kelvin, valeur);
  #endif

  if ((valeur < -30.0) || (valeur > 60.0)) Text_erreur = 1;
  if (!Val_Text) Text_erreur = 2;

  *mesure = valeur;
  return Text_erreur;
}



uint8_t fetch_internet_temp() {

  uint8_t res=1;

  // 1. Vérifier si le réseau est disponible avant de commencer
  if (WiFi.status() != WL_CONNECTED) {
    // Si vous utilisez l'Ethernet, remplacez par le test approprié
    return res; 
  }

  HTTPClient http;

  // 2. Définir un timeout court (2000ms au lieu des 5-10s par défaut)
  http.setTimeout(2000); 

  char url[150];  // assez grand pour contenir toute l'URL
  sprintf(url, "http://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m", latitude, longitude);


  if (http.begin(url)) {
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();

      // pour éviter les warnings de la librairie ArduinoJson sur les anciennes versions de l'ESP32
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      DynamicJsonDocument doc(512);
      #pragma GCC diagnostic pop

      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        float temp = doc["current"]["temperature_2m"] | NAN;
        if (!isnan(temp) && temp > -50.0 && temp < 60.0)
        {
          res = 0;
          Text = temp;
          //Serial.printf("Météo Garches : %.1f°C\n", Text);
          uint32_t mil = millis();
          if (mil - last_remote_Text_time > 35*60*1000) // le precedent message est vieux de plus de 35 minutes
            err_Text++;
          last_remote_Text_time = mil;
          cpt24_Text++;
          tempE_moy24h += Text;
        }
      } else {
        Serial.printf("Erreur parsing JSON Météo : %s\n", error.c_str());
      }
    } else {
      Serial.printf("Erreur HTTP Météo (%d) : %s\n", httpCode, http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  return res;
}

uint8_t envoi_valeur()
{
  if (!esp_now_actif || freq_envoi<1 || freq_envoi>MAX_TEMP) return 1;

  Message_EspNow message;

  message.destinataire = SERVER_ADD | 0x80;  // 0x80 = message hexa
  message.emetteur = ADDRESS;
  message.code = 'C';
  message.code2 = 'T';

  message.payload[0] = freq_envoi;
  message.payload[1] = (skip_graph* periode_cycle) & 0xFF;
  message.payload[2] = ((skip_graph* periode_cycle) >> 8) & 0xFF;
  
  uint8_t pos = 3;
  for (uint8_t cpt = 0; cpt < freq_envoi; cpt++)
  {
    payloadWrite(message.payload, pos, (uint16_t)(graphique[cpt][0] + 1000));  // Temp : Si négatif => ajouter 10°degrés
    payloadWrite(message.payload, pos, (uint16_t)(graphique[cpt][1]));
  }

  // Taille réelle du message envoyé
  message.longueur = 5 + pos - 3;

  uint8_t result = envoi_data_gateway(message);


  if (result == ESP_OK) {
      Serial.printf("✅ %d valeurs Temp-HA-HR envoyées\n",  message.payload[0]);
  } else {
      Serial.printf("❌ Erreur envoi ESP-NOW : %d\n", result);
      activation_writelog();
      writeLog('E', 8, graphique[0][0]/100, graphique[0][1]/100, "Esp_now");

      return 1;
  }
  return 0;
}

uint8_t envoi_valeur_instant(float Tint, float Humid, float HA)
{
  if (!esp_now_actif) return 0;

  Message_EspNow message;

  message.destinataire = SERVER_ADD | 0x80;  // 0x80 = message hexa
  message.emetteur = ADDRESS;
  message.code = 'C';
  message.code2 = 'I';

  uint8_t pos = 0;
  payloadWrite(message.payload, pos, (uint16_t)(Tint * 100));
  payloadWrite(message.payload, pos, (uint16_t)(Humid * 100));
  payloadWrite(message.payload, pos, (uint16_t)(HA * 100));

  // Taille réelle du message envoyé
  message.longueur = 8;

  uint8_t result = envoi_data_gateway(message);
  return result;
}

void envoi_temp_hygro()
{
  lecture_Tint(&Tint, &Humid);
  float HA = absoluteHumidity(Tint, Humid);
  Serial.printf("Temp int:%.2f Humid:%.2f HA:%.2f\n\r", Tint, Humid, HA); 
  envoi_valeur_instant(Tint, Humid, HA);
}


void event_cycle()  // toutes les 15 minutes  (Power on, Timer on, inconnu)
{

    uint8_t i;
    // chaque 5/15 minutes
    for (i = NB_VAL_TAB - 1; i; i--) {
        Nb_PI[i]= Nb_PI[i - 1];
    }    
    Nb_PI[0] = compteur_detection;
    compteur_detection=0;

  // Récupération de la température extérieure par internet
  //fetch_internet_temp();
  Text = 0;

  // lecture temp-humi
  uint8_t err_Tint = lecture_Tint(&Tint, &Humid);
  if (err_Tint)
    log_erreur(Code_erreur_Tint, err_Tint, 1);
  else
    Serial.printf("Temp int:%.2f Humid:%.2f\n\r", Tint, Humid); 

  float HA = absoluteHumidity(Tint, Humid);

  float tempI_moy15m, tempE_moy15m, HA_moy15m, Hum_moy15m, PIR_moy15m;
  uint8_t cpt15_Tint, cpt15_Text, cpt15_HA, cpt15_Hum, cpt15_PIR;

  tempI_moy15m += Tint;
  cpt15_Tint++;
  tempE_moy15m += Text;
  cpt15_Text++;
  HA_moy15m += HA;
  cpt15_HA++;
  Hum_moy15m += Humid;
  cpt15_Hum++;
  PIR_moy15m += compteur_detection_1h;
  cpt15_PIR++;

  // chaque 15 minutes
  compteur_graph++;
  if (compteur_graph >= skip_graph)  // 1 valeur sur x
  {
    int16_t tempI_arrondi = 200;
    int16_t tempE_arrondi = 150;
    int16_t HA_arrondi = 10;
    int16_t Hum_arrondi = 500;
    int16_t PIR_arrondi = 0;

     if (cpt15_Tint > 0) {  tempI_arrondi = round(tempI_moy15m / cpt15_Tint * 100);  }
     if (cpt15_Text > 0) {  tempE_arrondi = round(tempE_moy15m / cpt15_Text * 100); }
     if (cpt15_HA > 0) {    HA_arrondi = round(HA_moy15m / cpt15_HA * 100); }
     if (cpt15_Hum > 0) {   Hum_arrondi = round(Hum_moy15m / cpt15_Hum * 100); }
     if (cpt15_PIR > 0) {   PIR_arrondi = round(PIR_moy15m / cpt15_PIR * 10); }

    compteur_graph = 0;
    for (i = NB_Val_Graph - 1; i; i--) {
      graphique[i][0] = graphique[i - 1][0];
      graphique[i][1] = graphique[i - 1][1];
      graphique[i][2] = graphique[i - 1][2];
    }
    graphique[0][0] = tempI_arrondi; // 20°C => 2000
    graphique[0][1] = selec_graph(GRAPH2, HA_arrondi, Hum_arrondi, PIR_arrondi);
    graphique[0][2] = selec_graph(GRAPH3, HA_arrondi, Hum_arrondi, PIR_arrondi);

    if (compteur_detection_1h>1)  // au moins 2
    {
      uint8_t tot = (uint8_t)compteur_detection_1h;
      if (compteur_detection_1h > 255) tot = 255;
      writeLog('D', 1, tot, 0, "PIR 1h");
    }
    compteur_detection_1h = 0;

    if (action_envoi) 
    {
      cpt_envoi++;
      if (cpt_envoi >= freq_envoi)  // envoi toutes les x valeurs (x*skip_graph*15 minutes)
      {
        cpt_envoi=0;
        envoi_valeur();  // envoi  par ESP-NOW
      }
    }
    tempI_moy15m = 0;
    tempE_moy15m = 0;
    HA_moy15m = 0;
    Hum_moy15m = 0;
    PIR_moy15m = 0;
    cpt15_Tint = 0;
    cpt15_Text = 0;
    cpt15_HA = 0; 
    cpt15_Hum = 0;
    cpt15_PIR = 0;
  }
  tempI_moy24h += Tint;
  cpt24_Tint++;
  tempE_moy24h += Text;
  cpt24_Text++;
  HA_moy24h += HA;
  cpt24_HA++;
  Hum_24h += Humid;
  cpt24_Hum++;
  PIR_24h += compteur_detection_1h;
  cpt24_PIR++;

  //Serial.printf("fin cycle :reveil:%i cpt:%i %i tint:%i 24h:%i\n\r", type_reveil, compteur_graph, skip_graph, graphique[0][0], compteur_24h);
  
  if (type_reveil != 10)      // si diff de toujours actif => compteur 24h
  {
    compteur_24h++;
    if (compteur_24h >= 24*60/periode_cycle) // )  // toutes les 24h
    {
      Serial.println("24h");
      activation_writelog();
      enreg_24h(1);  // et envoi si actif
      compteur_24h=0;
    }
  }
}



float readBatteryVoltage() {
  // Lecture ADC (0-4095) sur PIN_Vbatt
  // Sur ESP32 DevKit V1, l'ADC est calibré par défaut
  int raw = analogRead(PIN_Vbatt);
  float voltage = 0.0;  
  // Conversion:
  // raw / 4095.0 * 3.3V (tension ref approx) * 2 (pont diviseur) * 1.1 (facteur corection empirique souvent nécessaire sur ESP32)
  // On commence sans facteur 1.1 pour tester
  #ifdef ESP32_Fire2
     voltage = (raw / 4095.0) * 3.3 * 2.5; 
  #endif

  #ifdef ESP32_uPesy
     voltage = (raw / 4095.0) * 3.3 * 1.411 ;
  #endif

  return voltage;
}

#ifdef ESP_TJ_ACTIF
// Callback reception ESP-NOW
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
//void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
//void OnDataRecv(const esp_now_peer_info_t * info, const uint8_t *incomingData, int len) {
  // 🔍 DIAGNOSTIC: Afficher infos de réception
  Serial.println("\n📥 ========== RECEPTION ESP-NOW ==========");
  for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
    }
  /*Serial.printf("   Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info->src_addr[0], info->src_addr[1], info->src_addr[2],
                info->src_addr[3], info->src_addr[4], info->src_addr[5]);*/
  
  // Afficher le canal WiFi actuel
  uint8_t current_channel;
  wifi_second_chan_t second;
  esp_wifi_get_channel(&current_channel, &second);
  Serial.printf("   Canal WiFi actuel: %d\n", current_channel);
  Serial.printf("   Taille reçue: %d octets\n", len);
  
  Message_EspNow receivedMessage;
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

  Serial.print("   Type: "); Serial.print(receivedMessage.type);
  Serial.print(" | Valeur: "); Serial.println(receivedMessage.valuef);
  Serial.println("=========================================\n");

  if (receivedMessage.type == 1) { // Temperature
    float Trecue = receivedMessage.valuef;
    if ((Trecue > -10.0) && (Trecue < 49.99f)) 
    {
      Tint = Trecue;
      if ((Trecue < 24.99) || (Trecue > 25.01))
      {
        unsigned long mil = millis();
        if (mil - last_remote_Tint_time > 25*60*1000) // le precedent message est vieux de plus de 25 minutes
          err_Tint++;
        last_remote_Tint_time = mil;
        cpt24_Tint++;
        tempI_moy24h += Tint;
      }
    }
    Serial.printf("✅ Tint mise à jour: %.2f°C\n", Tint);
  }
  else if (receivedMessage.type == 2) { // Batterie
    Vbatt_ESP = receivedMessage.valuef;
    Serial.printf("✅ Vbatt_Th mise à jour: %.2fV\n", Vbatt_ESP);
  }
  else {
    Serial.printf("⚠️ Type de message inconnu: %d\n", receivedMessage.type);
  }
}
#endif


// envoie data à la gateway par ESP_now
uint8_t envoi_data_gateway(Message_EspNow mess_esp)
{

  if ((mac_gw[0] || mac_gw[3] || mac_gw[4]) && (esp_now_actif==1))
  {
    // Initialisation WiFi en mode Station (nécessaire pour ESP-NOW)

 // Ne change le mode que si nécessaire
    if (WiFi.getMode() != WIFI_STA &&  WiFi.getMode() != WIFI_AP_STA)
    {
        WiFi.mode(WIFI_STA);
        Serial.println("WiFi mode set to STA for ESP-NOW");
    }
    //WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      ESP.restart();
    }

    esp_now_register_send_cb(OnDataSent);

    // Préparation du Peer (Chaudière)
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo)); // Initialisation complète à zéro
    memcpy(peerInfo.peer_addr, mac_gw, 6);
    peerInfo.channel = 0; // Le canal sera défini avant l'ajout
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA; // Interface WiFi Station (OBLIGATOIRE)

    // 🚀 OPTION 1 : Forcer le canal connu (plus rapide et économe en énergie)
    // Si vous connaissez le canal de votre routeur, décommentez ces lignes :
    /*
    Serial.printf("🎯 Forçage canal %d (défini dans variables.h)\n", WIFI_CHANNEL);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    last_wifi_channel = WIFI_CHANNEL; // Pour la prochaine fois
    */

    // 🔍 OPTION 2 : Scan robuste des canaux (si le canal n'est pas connu ou change)

    Serial.printf("🔍 Scan de 13 canaux (priorité: canal %d)\n", last_wifi_channel);
    uint8_t deliverySuccess = false;
    uint8_t current_channel;
    if (!last_wifi_channel || last_wifi_channel>13) last_wifi_channel=1;  // si corrompu : channel 1

    if (etat_now==0)  // encore aucun envoi essayé
    {
      for (uint8_t k = 0; k < 13; k++)  // => channel 1 à 13
      {
        current_channel = k + last_wifi_channel;
        if (current_channel > 13) current_channel -= 13;
        deliverySuccess = envoi_now(current_channel, &peerInfo, &mess_esp);
        if (deliverySuccess) break;
      }
      if (deliverySuccess) etat_now=2;
      else etat_now=1;
    }
    else if (etat_now==2)  // essai precedent reussi
    {
      deliverySuccess = envoi_now(last_wifi_channel, &peerInfo, &mess_esp);
      if (!deliverySuccess) etat_now=4; // prochain essai sur le meme channel+ scan
    }
    else if (etat_now==1 || etat_now==3)  // essai precedent raté
    {
      deliverySuccess = envoi_now(last_wifi_channel, &peerInfo, &mess_esp);
      if (deliverySuccess) etat_now=2; 
      else 
      {
        // prochain essai sur le channel suivant
        last_wifi_channel++;
        if (last_wifi_channel > 13) last_wifi_channel=1;
      }
    }
    else if (etat_now==4)  // envoi precedent raté mais celui d'avant réussi
    {
      for (uint8_t k = 0; k < 14; k++)  // => channel 1 à 13 et 1 de plus
      {
        current_channel = k + last_wifi_channel;
        if (current_channel > 13) current_channel -= 13;
        deliverySuccess = envoi_now(current_channel, &peerInfo, &mess_esp);
        if (deliverySuccess) break;
      }
      if (deliverySuccess) etat_now=2;
      else etat_now=3;
    }
    else etat_now=0; // si etat_now corrompu, remise à 0

    Serial.printf("etat_now:%i\n\r", etat_now);
    return 1-deliverySuccess;
    
  }
  else
    Serial.println("Adresse Mac gateway nulle");

  return 2;
}

uint8_t envoi_now(uint8_t channel, esp_now_peer_info_t * peerInfo, Message_EspNow * message)
{
  uint8_t result = false;

  // Fixer le canal
  Serial.printf("\n--- Essai canal %d ---\n", channel);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  // Vérifier que le canal a bien été changé
  uint8_t actual_channel;
  wifi_second_chan_t second;
  esp_wifi_get_channel(&actual_channel, &second);
  
  if (actual_channel != channel)
  {
    Serial.printf("⚠️ Échec changement canal (demandé:%d, actuel:%d)\n", channel, actual_channel);
    delay(100); // Attendre un peu plus
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_get_channel(&actual_channel, &second);
    Serial.printf("   2ème tentative: canal actuel=%d\n", actual_channel);
  } else {
    //Serial.printf("✅ Canal changé: %d\n", actual_channel);
  }
  
  delay(50); // Délai pour stabilisation du canal

  // Ajouter le peer sur ce canal
  if (esp_now_is_peer_exist(mac_gw)) {
    esp_now_del_peer(mac_gw);
  }
  peerInfo->channel = actual_channel; // Utiliser le canal réel
  if (esp_now_add_peer(peerInfo) != ESP_OK){
    Serial.println("❌ Échec ajout peer");
  }
  //Serial.println("✅ Peer ajouté");

  // Envoi Message
  
  // 🔍 DIAGNOSTIC: Afficher les infos avant envoi
  /*Serial.printf("📤 Tentative envoi sur canal %d vers MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                actual_channel,
                mac_gw[0], mac_gw[1], mac_gw[2],
                mac_gw[3], mac_gw[4], mac_gw[5]);*/
  
  ackReceived=0;
  ackChannel = -1;
  esp_err_t resulta = esp_now_send(mac_gw, (uint8_t *) message, message->longueur+3);

  // impression du message envoyé pour diagnostic
  for (int i = 0; i < message->longueur+3; i++) {
    Serial.printf("%02X ", ((uint8_t*)message)[i]);
  }

  if (resulta == ESP_OK)
  {
    //Serial.printf("Envoye sur canal %d\n", actual_channel);

    // attendre la réponse max 100 ms
    int wait = 0;
    while (!ackReceived && wait < 10) { // 10 * 10ms = 100ms
        delay(10);
        wait++;
    }

    if (ackReceived) // canal trouvé
    {
      result = true; 
      Serial.println("✅ Ack Recu");
      if (last_wifi_channel != actual_channel)
      {
        last_wifi_channel = actual_channel;
      }
    }
  }
  else Serial.println("❌ Echec d'envoi");

  return result;
}


