# PCB Custom - Monitor de Contaminación

## Herramienta Recomendada: EasyEDA (prototipo) → KiCad (producción)

### Comparación de Herramientas

| Criterio | Flux.ai | EasyEDA | **KiCad** |
|---|---|---|---|
| Costo | Gratis (limitado) | Gratis | Gratis/Open Source |
| IA en el diseño | Sí (beta) | No | Plugins (Freerouting) |
| Integración JLCPCB | No directa | **Nativa** | Via plugin |
| Integración LCSC | No | **Nativa** | Via plugin |
| Biblioteca de componentes | Limitada | LCSC integrada | 8.000+ símbolos |
| Autorouter | Sí (IA) | Básico | Freerouting (externo) |
| Control de versiones (git) | No | No | **Sí (.kicad_sch = texto)** |
| Exporta Gerber | Sí | Sí | Sí |
| Exporta BOM | Sí | Sí (LCSC) | Sí |

### Veredicto

**Fase 1 (prototipo, 1-5 unidades):** EasyEDA por integración nativa con JLCPCB y LCSC. El servicio PCBA de JLCPCB permite pedir la placa ya ensamblada con ~7-14 días de entrega.

**Fase 2 (producción, 10+ unidades):** KiCad porque el archivo `.kicad_sch` es texto plano (vive bien en git), sin lock-in a plataforma privada.

**Flux.ai:** Descartado. El routing con IA está en beta, la biblioteca de componentes es limitada, y no hay integración directa con JLCPCB/LCSC.

---

## Arquitectura del Diseño

### Microcontrolador: ESP32-C3-MINI-1

- WiFi 802.11 b/g/n integrado (reemplaza WiFiNINA)
- 400 KB SRAM, 4 MB Flash integrado
- 22 GPIOs, I2C, SPI, UART, ADC, I2S
- 3.3V lógica nativa
- Precio: ~$1.80 USD (LCSC C2838502)

### Sensores integrados en PCB

| Sensor | Chip | Interfaz | Precio LCSC | Reemplaza |
|---|---|---|---|---|
| Temp/Humedad | SHT31-DIS | I2C | ~$2.00 (C2943854) | DHT22 módulo |
| Luz | BH1750FVI | I2C | ~$0.35 (C78960) | Módulo LDR |
| Micrófono | ICS-43434 (MEMS) | I2S | ~$1.50 (C2069979) | Módulo analógico |
| Gas MQ-2 | Pads bare chip | ADC | - | Módulo MQ-2 |

### Sensores con conector externo

| Sensor | Conector | Razón |
|---|---|---|
| PPD42 (polvo PM10) | JST PH 5-pin | Voluminoso, necesita cable 50cm+ |

### Alimentación

- Entrada: USB-C 5V (GCT USB4105-GF-A, C2765186)
- Regulador: AMS1117-3.3V LDO
- Capacitores de bypass: 100nF + 10µF en cada rail
- LED power: LED verde + resistencia 1kΩ

### Mapa de pines ESP32-C3-MINI-1

```
GPIO2  → PPD42 Vout (pulsos digitales)
GPIO3  → MQ-2 AOUT (ADC1_CH3, via divisor voltaje)
GPIO4  → I2S WS  (ICS-43434)
GPIO5  → I2S SCK
GPIO6  → I2S SD
GPIO8  → I2C SDA (SHT31 + BH1750)
GPIO9  → I2C SCL
GPIO20 → UART RXD0 (debug)
GPIO21 → UART TXD0
EN     → Reset (SW1, pull-up 10k)
GPIO9  → Boot (SW2, pull-up 10k)
```

---

## Archivos en este directorio

```
hardware/pcb/
├── README.md                        ← Este archivo
├── pollution-monitor-v1.kicad_sch   ← Esquemático KiCad v20230121
├── pollution-monitor-v1.kicad_pro   ← Proyecto KiCad con netclasses
├── BOM.csv                          ← BOM con precios LCSC (10/50/100 unidades)
└── FLUX_AI_GUIDE.md                 ← Guía paso a paso para EasyEDA/Flux.ai/KiCad
```

---

## Estimación de costo por nodo

| Componente | $/unidad | Fuente |
|---|---|---|
| PCB fabricada (10u) | $0.80 | JLCPCB |
| ESP32-C3-MINI-1 | $1.80 | LCSC |
| SHT31-DIS | $2.00 | LCSC |
| BH1750FVI | $0.35 | LCSC |
| ICS-43434 | $1.50 | LCSC |
| MQ-2 bare chip | $1.20 | LCSC |
| AMS1117-3.3V | $0.15 | LCSC |
| USB-C connector | $0.40 | LCSC |
| JST PH 5-pin | $0.25 | LCSC |
| PPD42 (externo) | $8.00 | Aliexpress |
| Capacitores/resistores | $0.80 | LCSC |
| **Total estimado** | **~$17.25** | vs $80-100 actual |

**Ahorro estimado: ~78% por nodo**

---

## Ajustes de firmware necesarios

```cpp
// firmware/pollution_monitor.ino

// ANTES (Arduino WiFiNINA):
#include <WiFiNINA.h>

// DESPUÉS (ESP32):
#include <WiFi.h>

// Pines I2C en ESP32-C3-MINI-1:
#define SDA_PIN 8
#define SCL_PIN 9
Wire.begin(SDA_PIN, SCL_PIN);

// I2S para ICS-43434 (reemplaza lectura analógica):
#include <driver/i2s.h>
// WS=GPIO4, SCK=GPIO5, SD=GPIO6
```

## Fabricación

- **Tamaño:** 70 x 55 mm (2 capas FR4)
- **Proveedor:** JLCPCB
- **Precio 10 unidades PCB:** ~$8 USD
- **PCBA (ensamblado):** ~$80-120 USD total para 10 unidades
