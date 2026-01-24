# CCR ESP32 Voltage Monitor

Firmware ESP32 per misurare la tensione di rete (230V) tramite modulo ZMPT101B, calcolare Vrms e inviare campioni/eventi a un server CCR via HTTP batch.

## Features
- Campionamento ADC1 (default GPIO34) con RMS su finestre da 200 ms.
- Calibrazione gain/offset via NVS + CLI seriale.
- Ring buffer 60s e rilevamento eventi SAG/SWELL/CRITICAL con isteresi (media mobile su 3 finestre per ridurre rumore).
- Upload batch HTTP con retry/backoff e persistenza su LittleFS.
- Sincronizzazione NTP e timestamp in epoch ms.

## Wiring (ZMPT101B → ESP32)
- **VCC** → 5V (o 3.3V secondo modulo)
- **GND** → GND
- **OUT** → GPIO34 (ADC1) di default

> **Sicurezza:** La misura di 230V è pericolosa. Usa sempre moduli isolati, alimenta l’ESP32 separatamente e non lavorare su circuiti in tensione.

## Configurazione Wi‑Fi e server
1. Copia `include/secrets.example.h` in `include/secrets.h`.
2. Compila con le tue credenziali:
   - `WIFI_SSID`, `WIFI_PASSWORD`
   - `CCR_BASE_URL` (es. `http://192.168.1.50:3000`)
   - `DEVICE_ID`
   - `CCR_API_KEY` (opzionale, può restare stringa vuota)

## Calibrazione
Comandi seriali (115200 baud):
- `calib show` → stampa gain/offset.
- `calib gain <valore>` → imposta gain.
- `calib offset <valore>` → imposta offset.
- `calib assist on/off` → mostra `raw_rms` e `vrms` a ogni finestra.

Formula:
```
vrms = raw_rms * gain + offset
```

## Eventi
- **SAG**: vrms < 207V per ≥1s, termina quando vrms > 210V per 10s.
- **SWELL**: vrms > 253V per ≥1s, termina quando vrms < 250V per 10s.
- **CRITICAL**: vrms < 190V o > 265V immediato, termina dopo 10s in range.

Ogni evento include 60s di pre e 60s di post campioni (200ms).

## Build & Flash (PlatformIO)
```
pio run
pio run -t upload
pio device monitor
```

## Endpoint CCR
- **Samples**: `POST /ingest/voltage/samples`
- **Events**: `POST /ingest/voltage/events`

Header:
- `Content-Type: application/json`
- `X-DEVICE-ID: <device_id>`
- `X-API-KEY: <optional>`

Payload samples (compatto):
```
{
  "device_id": "esp32-01",
  "fw_version": "0.1.0",
  "sample_period_ms": 200,
  "samples": [ [ts_ms, vrms, flags], ... ]
}
```

Payload events:
```
{
  "device_id": "esp32-01",
  "fw_version": "0.1.0",
  "type": "SAG",
  "start_ts": 123,
  "end_ts": 456,
  "min_vrms": 190.0,
  "max_vrms": 230.0,
  "samples": [ [ts_ms, vrms, flags], ... ]
}
```
