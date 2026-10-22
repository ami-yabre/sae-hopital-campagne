#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ---- À MODIFIER ----
const char* WIFI_SSID = "VOTRE_WIFI";
const char* WIFI_PASS = "VOTRE_MOT_DE_PASSE";
const char* MQTT_HOST = "IP_DU_BROKER";
const int   MQTT_PORT = 1883;
const char* SALLE     = "bloc_operatoire";   // salle où se trouve ce lecteur
// --------------------

const char* TOPIC_BADGE    = "iot/badge/access";
const char* TOPIC_COMMANDE = "securite/porte/commande";

#define PN532_IRQ   (14)
#define PN532_RESET (3)
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool nfcOk = false;
unsigned long dernierEssaiMqtt = 0;
unsigned long dernierSimule = 0;

// Réponse de Node-RED : {"badge_id":"...","salle":"...","ouvrir":true|false}
void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.println();
  Serial.println("=====================================");
  if (msg.indexOf("\"ouvrir\":true") >= 0) {
    Serial.println("  >>> LA PORTE EST OUVERTE <<<");
  } else {
    Serial.println("  >>> AUCUN ACCES <<<");
  }
  Serial.println("=====================================");
  Serial.println();
}

void connecterWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);              // évite les coupures de veille
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" OK, IP ESP32 : ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" echec");
  }
}

void connecterMqtt() {
  if (mqtt.connected() || WiFi.status() != WL_CONNECTED) return;
  if (millis() - dernierEssaiMqtt < 3000) return;   // un essai toutes les 3 s, sans bloquer
  dernierEssaiMqtt = millis();
  String id = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.print("MQTT...");
  if (mqtt.connect(id.c_str())) {
    Serial.println(" connecte");
    mqtt.subscribe(TOPIC_COMMANDE);                 // on écoute la réponse de Node-RED
  } else {
    Serial.print(" echec rc=");
    Serial.println(mqtt.state());
  }
}

void envoyerBadge(const String& badge) {
  String msg = String("{\"badge_id\":\"") + badge + "\",\"salle\":\"" + SALLE + "\"}";
  if (mqtt.connected() && mqtt.publish(TOPIC_BADGE, msg.c_str())) {
    Serial.print("Envoye : ");
  } else {
    Serial.print("NON ENVOYE (pas de connexion) : ");
  }
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  nfc.begin();
  nfcOk = (nfc.getFirmwareVersion() != 0);
  if (nfcOk) {
    nfc.SAMConfig();
    Serial.println("Lecteur NFC PN532 detecte.");
  } else {
    Serial.println("PN532 introuvable : mode SIMULATION (un badge toutes les 10 s).");
  }

  connecterWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(10);
  Serial.println("Systeme pret. En attente d'un badge...");
}

void loop() {
  connecterWifi();
  connecterMqtt();
  mqtt.loop();

  if (nfcOk) {
    uint8_t uid[7];
    uint8_t uidLength;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 300)) {
      char id[15] = "";
      for (uint8_t i = 0; i < uidLength; i++) sprintf(id + 2 * i, "%02X", uid[i]);
      Serial.println();
      Serial.print("Badge detecte : "); Serial.println(id);
      envoyerBadge(String(id));

      // On laisse 2 secondes à Node-RED pour répondre, en continuant d'écouter
      unsigned long t = millis();
      while (millis() - t < 2000) {
        mqtt.loop();
        delay(10);
      }
    }
  } else if (millis() - dernierSimule > 10000) {   // mode simulation sans lecteur
    dernierSimule = millis();
    envoyerBadge("63DF0A28");
  }
}
