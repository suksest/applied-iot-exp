/*
 * Studi Kasus: Sistem Monitoring Suhu dan Kontrol Lampu Berbasis IoT
 * ESP32 DevKitC V4 + DHT22 + Modul Relay + MQTT (HiveMQ) — simulasi Wokwi
 *
 * - Membaca suhu & kelembapan DHT22 setiap 2 detik
 * - Mengirim data ke broker MQTT (JSON) secara berkala
 * - Relay ON saat suhu >= 30 °C, OFF saat suhu <= 27 °C (hysteresis)
 * - Status relay dikirim ke broker setiap kali berubah (retained)
 * - Tanpa delay() yang menghambat: semua penjadwalan memakai millis()
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "hysteresis.h"

// ---------- Konfigurasi WiFi Wokwi ----------
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ---------- Konfigurasi MQTT ----------
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;

// Topik MQTT (prefix unik agar tidak bentrok di broker publik)
const char* topic_suhu   = "appliediot/sukma/ruang1/suhu";    // JSON suhu + kelembapan
const char* topic_relay  = "appliediot/sukma/ruang1/relay";   // "ON" / "OFF" (retained)
const char* topic_online = "appliediot/sukma/ruang1/online";  // "online" / "offline" (LWT)

// ---------- Pin ----------
#define RELAY_PIN 23
#define DHT_PIN   15
#define DHT_TYPE  DHT22

// ---------- Interval (ms) ----------
const unsigned long SENSOR_INTERVAL     = 2000;  // DHT22 maksimal 0,5 Hz
const unsigned long WIFI_RETRY_INTERVAL = 5000;
const unsigned long MQTT_RETRY_INTERVAL = 5000;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

bool relayState = false;          // kondisi awal relay mati
bool relayStatePublished = false; // status relay sudah terkirim ke broker?
float lastTemperature = NAN;
float lastHumidity = NAN;

unsigned long lastSensorRead = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;

// Mengubah relay dan menandai bahwa status baru perlu dikirim
void setRelay(bool on) {
  if (on == relayState) return;
  relayState = on;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  relayStatePublished = false;
  Serial.printf("Relay %s (suhu %.1f C)\n", relayState ? "ON" : "OFF", lastTemperature);
}

// Mengirim status relay; jika MQTT putus, akan dikirim ulang setelah tersambung
void publishRelayState() {
  if (relayStatePublished || !client.connected()) return;
  if (client.publish(topic_relay, relayState ? "ON" : "OFF", true)) {
    relayStatePublished = true;
    Serial.printf("MQTT -> %s : %s\n", topic_relay, relayState ? "ON" : "OFF");
  }
}

void readSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Gagal membaca DHT22");
    return;
  }

  lastTemperature = t;
  lastHumidity = h;
  Serial.printf("Suhu: %.1f C | Kelembapan: %.1f %%\n", t, h);

  // Kontrol relay tetap berjalan walaupun WiFi/MQTT putus
  setRelay(nextRelayState(t, relayState));

  if (client.connected()) {
    char payload[96];
    snprintf(payload, sizeof(payload),
             "{\"suhu\":%.1f,\"kelembapan\":%.1f,\"relay\":\"%s\"}",
             t, h, relayState ? "ON" : "OFF");
    client.publish(topic_suhu, payload);
    Serial.printf("MQTT -> %s : %s\n", topic_suhu, payload);
  }
}

// Menjaga koneksi WiFi tanpa memblokir loop
void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  unsigned long now = millis();
  if (lastWifiAttempt != 0 && now - lastWifiAttempt < WIFI_RETRY_INTERVAL) return;
  lastWifiAttempt = now;
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.disconnect();
  WiFi.begin(ssid, password, 6);  // kanal 6 = kanal Wokwi-GUEST, tanpa scan sehingga cepat terhubung
}

// Menjaga koneksi MQTT tanpa memblokir loop
void maintainMqtt() {
  if (WiFi.status() != WL_CONNECTED || client.connected()) return;
  unsigned long now = millis();
  if (lastMqttAttempt != 0 && now - lastMqttAttempt < MQTT_RETRY_INTERVAL) return;
  lastMqttAttempt = now;

  Serial.print("Menghubungkan MQTT...");
  String clientId = "ESP32-Suhu-";
  clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

  // Last Will: broker mengirim "offline" jika ESP32 terputus tidak wajar
  if (client.connect(clientId.c_str(), topic_online, 1, true, "offline")) {
    Serial.println("terhubung");
    client.publish(topic_online, "online", true);
    relayStatePublished = false;  // kirim ulang status relay terkini
  } else {
    Serial.printf("gagal, rc=%d, coba lagi %lu detik\n",
                  client.state(), MQTT_RETRY_INTERVAL / 1000);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();

  WiFi.mode(WIFI_STA);
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Sistem monitoring suhu & kontrol lampu dimulai");
}

void loop() {
  static bool wifiWasConnected = false;
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (wifiConnected && !wifiWasConnected) {
    Serial.print("WiFi terhubung, IP: ");
    Serial.println(WiFi.localIP());
  }
  wifiWasConnected = wifiConnected;

  maintainWifi();
  maintainMqtt();
  client.loop();

  unsigned long now = millis();
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    readSensor();
  }

  publishRelayState();
}
