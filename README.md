# Monitoring Suhu & Kontrol Lampu IoT (ESP32 + DHT22 + Relay + MQTT)

ESP32 DevKit V1 membaca suhu dan kelembapan dari DHT22, mengirimkannya ke broker MQTT HiveMQ, dan menyalakan/mematikan lampu (LED melalui modul relay) secara otomatis berdasarkan suhu.

## Logika kontrol (hysteresis)

| Suhu | Relay / lampu |
|---|---|
| ≥ 30 °C | ON |
| ≤ 27 °C | OFF |
| 27 °C – 30 °C | Status sebelumnya dipertahankan |

Kondisi awal relay OFF. Program tidak memakai `delay()` yang memblokir; pembacaan sensor dan penyambungan ulang WiFi/MQTT dijadwalkan dengan `millis()`.

## Rangkaian

| Komponen | Pin | ESP32 |
|---|---|---|
| DHT22 | VCC / SDA / GND | 3V3 / GPIO 15 / GND |
| Relay | VCC / GND / IN | VIN / GND / GPIO 23 |
| Relay → LED | COM → 3V3, NO → LED(A), LED(K) → 220 Ω → GND | |

## Topik MQTT (`broker.hivemq.com:1883`)

| Topik | Payload |
|---|---|
| `appliediot/sukma/ruang1/suhu` | `{"suhu":31.0,"kelembapan":60.0,"relay":"ON"}` tiap 2 detik |
| `appliediot/sukma/ruang1/relay` | `ON` / `OFF` saat berubah (retained) |
| `appliediot/sukma/ruang1/online` | `online` / `offline` (Last Will) |

## Cara mencoba

1. Klik ▶ untuk memulai simulasi dan tunggu "terhubung" di Serial Monitor.
2. Klik sensor DHT22, geser nilai *temperature* (mis. 25 → 30 → 32 → 29 → 27 → 26 °C).
3. Amati LED dan Serial Monitor; untuk melihat pesan MQTT, subscribe ke `appliediot/sukma/ruang1/#` di MQTTX.

## Library

- PubSubClient
- DHT sensor library
- Adafruit Unified Sensor

## Dashboard web & deploy ke Vercel

`dashboard/index.html` subscribe langsung ke HiveMQ lewat MQTT over WebSocket, jadi bisa dibuka lokal atau di-deploy sebagai situs statis.

Konfigurasi deploy:

- `vercel.json`: tanpa build, folder `dashboard/` disajikan sebagai root situs.
- `.vercelignore`: hanya `dashboard/` dan `vercel.json` yang diunggah; firmware, `.pio/`, dan laporan tidak ikut.

Deploy lewat CLI:

```bash
vercel        # preview
vercel --prod # produksi
```

Atau impor repositori ini di dashboard Vercel (Framework Preset: *Other*); pengaturan diambil dari `vercel.json`, dan setiap push ke `main` otomatis ter-deploy.
