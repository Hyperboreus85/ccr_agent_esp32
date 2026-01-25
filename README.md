# ESP32 ZMPT101B Debug

Sketch minimale per ESP32 Dev Module per leggere il sensore ZMPT101B su ADC1 (GPIO32) e stampare valori di debug via seriale, con Wi‑Fi connesso in STA.

## Wiring (ZMPT101B → ESP32)

| ZMPT101B | ESP32 (DevKit) | Note |
| --- | --- | --- |
| VCC | 3V3 | Alimenta a 3.3V |
| GND | GND | Massa comune |
| OUT | GPIO32 (ADC1) | Evita ADC2 quando usi Wi‑Fi |

> Regola il trimmer del modulo ZMPT101B per ottenere un offset ~1.65V (circa metà scala ADC).

## Build & Flash (PlatformIO)
```
pio run
pio run -t upload
pio device monitor
```

## Output seriale
Ogni secondo stampa una riga tipo:
```
rawMin=... rawMax=... offset=... amp=... vrms_adc=... vrms_mains=... wifi=connected/disconnected ip=...
```

## Note
- Usa ADC1 su GPIO32 con `analogReadResolution(12)` e `analogSetPinAttenuation(32, ADC_11db)`.
- Wi‑Fi in modalità STA con credenziali provvisorie nel codice (`WIFI` / `wifi1234`).
