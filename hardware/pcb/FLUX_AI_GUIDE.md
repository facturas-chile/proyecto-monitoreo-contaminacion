# Guía: Importar Esquemático a Flux.ai y Continuar Diseño PCB

## Contexto

El archivo `pollution-monitor-v1.kicad_sch` contiene el esquemático completo en formato KiCad S-expression. Flux.ai acepta netlists en formato KiCad para continuar con el layout de la PCB.

---

## Opción A: Flujo Recomendado — EasyEDA + JLCPCB (más rápido para prototipo)

### Paso 1: Crear proyecto en EasyEDA

1. Ir a [easyeda.com](https://easyeda.com) → Registrarse (gratis)
2. `New Project` → nombre: "Monitor Contaminacion v1"
3. `New Schematic` dentro del proyecto

### Paso 2: Colocar componentes desde biblioteca LCSC

Para cada componente del BOM, buscar por número LCSC:

```
Buscar en EasyEDA: Place → Search Parts → ingresar "C2838502" → ESP32-C3-MINI-1
```

| Componente | LCSC ID | Búsqueda EasyEDA |
|---|---|---|
| ESP32-C3-MINI-1 | C2838502 | "ESP32-C3-MINI" |
| SHT31-DIS | C2943854 | "SHT31" |
| BH1750FVI | C78960 | "BH1750" |
| ICS-43434 | C2069979 | "ICS-43434" |
| AMS1117-3.3 | C6186 | "AMS1117 3.3" |
| USB-C GCT | C2765186 | "USB4105" |
| JST PH 5-pin | C157926 | "B5B-PH-K" |

### Paso 3: Conectar según el esquemático KiCad

Referirse a `pollution-monitor-v1.kicad_sch` para las conexiones. El mapa de pines está en el bloque de notas del esquemático:

```
GPIO2  → PPD42 Vout
GPIO3  → MQ-2 AOUT (via divisor R9/R10)
GPIO4  → I2S WS (ICS-43434)
GPIO5  → I2S SCK
GPIO6  → I2S SD
GPIO8  → I2C SDA (SHT31 + BH1750)
GPIO9  → I2C SCL
GPIO20 → UART RXD0
GPIO21 → UART TXD0
EN     → Reset (SW1)
GPIO9  → Boot (SW2)
```

### Paso 4: Convertir a PCB

1. `Design` → `Convert Schematic to PCB`
2. EasyEDA importa todos los footprints automáticamente desde LCSC
3. Ajustar dimensiones: `Board Outline` → dibujar rectángulo 70x55mm

### Paso 5: Layout de la PCB

Orden sugerido de placement:

1. **ESP32-C3-MINI-1** → centro de la placa (es el componente más grande)
2. **USB-C connector** → borde inferior (para acceso con cable)
3. **AMS1117-3.3V** → cerca del USB-C (minimizar traza de potencia)
4. **Capacitores bypass LDO** → pegados al AMS1117
5. **SHT31** → alejado de fuentes de calor, preferentemente borde exterior
6. **BH1750** → cerca de una ventana/apertura en el case para luz ambiental
7. **ICS-43434** → borde con apertura acústica
8. **JST PPD42** → borde lateral (el cable PPD42 sale 50cm+)
9. **Header MQ-2** → área con ventilación
10. **Botones RESET/BOOT** → borde accesible
11. **LED power** → visible desde exterior

### Paso 6: Reglas de ruteo

```
Ancho mínimo traza señal:  0.2mm
Ancho traza potencia 3.3V: 0.5mm
Ancho traza potencia 5V:   0.8mm (MQ-2 heater consume ~120mA)
Clearance mínimo:          0.2mm
Via mínima:                0.3mm drill / 0.6mm pad
```

### Paso 7: Exportar y pedir en JLCPCB

1. `Fabrication` → `One-click Order at JLCPCB`
2. EasyEDA genera Gerber + BOM + CPL automáticamente
3. En JLCPCB seleccionar:
   - Quantity: 10
   - PCB Thickness: 1.6mm
   - Surface Finish: ENIG (mejor para pads SMD)
   - PCBA: Yes (ensamblado con componentes LCSC)
4. Revisar BOM → agregar los componentes LCSC del BOM.csv
5. Precio estimado 10 unidades PCBA: ~$80-120 USD total (~$8-12/unidad)

---

## Opción B: Flujo Flux.ai (si se prefiere IA para routing)

### Prerrequisito: Exportar netlist desde KiCad

Si se tiene KiCad instalado:

```bash
# Abrir el .kicad_sch en KiCad
# File → Export → Netlist → formato KiCad
# Guardar como pollution-monitor-v1.net
```

### Paso 1: Crear proyecto en Flux.ai

1. Ir a [flux.ai](https://www.flux.ai) → `New Project`
2. Nombre: "Monitor Contaminacion v1"

### Paso 2: Importar netlist

1. En Flux.ai: `Import` → `KiCad Netlist (.net)`
2. Seleccionar el archivo `pollution-monitor-v1.net`
3. Flux.ai importa los componentes y conexiones del esquemático

**Nota:** Si Flux.ai no encuentra footprints para todos los componentes, buscarlos manualmente:
- ESP32-C3-MINI-1 → buscar "ESP32-C3-MINI" en biblioteca Flux
- SHT31 → "Sensirion SHT31"
- BH1750 → "Rohm BH1750"

### Paso 3: Usar IA de Flux para routing

1. Definir board outline: 70x55mm
2. Placement manual de componentes críticos (USB-C, ESP32, sensores)
3. `AI Route` → seleccionar todas las redes
4. Revisar y ajustar manualmente las trazas de potencia

### Paso 4: DRC y export

1. `Design Rule Check` → resolver errores
2. `Export` → `Gerber Files` (formato Gerber RS-274X)
3. Subir a JLCPCB manualmente junto con BOM.csv

---

## Alternativa: Usar KiCad directamente (recomendado para producción)

### Instalar KiCad

```bash
# Ubuntu/Debian
sudo add-apt-repository ppa:kicad/kicad-8.0-releases
sudo apt install kicad

# macOS
brew install --cask kicad
```

### Abrir el proyecto

```bash
cd hardware/pcb/
kicad pollution-monitor-v1.kicad_pro
```

### Agregar las bibliotecas necesarias

En KiCad Symbol Editor, los símbolos usados provienen de:
- `Device:C`, `Device:R`, `Device:LED` → KiCad standard library (incluida)
- `RF_Module:ESP32-C3-MINI-1` → KiCad standard library (desde v7.0)
- `Sensor:SHT31-D` → KiCad standard library
- `Sensor:BH1750FVI` → agregar manualmente si no está
- `Audio:ICS-43434` → agregar manualmente si no está

Para los componentes faltantes, usar KiCad Symbol Editor:
1. `Tools` → `Symbol Editor` → `File` → `New Symbol`
2. Dibujar el símbolo con los pines del datasheet

### Continuar con PCB Editor

1. `Tools` → `Update PCB from Schematic`
2. KiCad transfiere todos los componentes con sus footprints
3. Activar Freerouting: `Tools` → `External Plugins` → `Freerouting`
4. Exportar Gerbers: `File` → `Fabrication Outputs` → `Gerbers`

---

## Notas de fabricación para JLCPCB

### Checklist pre-pedido

- [ ] Gerbers generados (capa Top, Bottom, F.Cu, B.Cu, Edge.Cuts, F.SilkS, B.SilkS, F.Mask, B.Mask)
- [ ] BOM con Part Number LCSC de cada componente SMD
- [ ] CPL (Component Placement List) con coordenadas X/Y y rotación
- [ ] Verificar que MQ-2 bare chip y PPD42 están marcados como "No Mount" en el BOM (se compran por separado)
- [ ] Mínimo de pedido JLCPCB: 5 placas ($2/placa para 2 capas)
- [ ] Tiempo de entrega estándar: 7-15 días + 3-5 días envío DHL

### Componentes LCSC para PCBA en JLCPCB

Los siguientes componentes tienen stock confirmado en LCSC y son compatibles con el servicio PCBA de JLCPCB:

| LCSC | Componente | Stock típico |
|---|---|---|
| C2838502 | ESP32-C3-MINI-1 | Alto |
| C2943854 | SHT31-DIS | Medio |
| C78960 | BH1750FVI | Alto |
| C2069979 | ICS-43434 | Bajo (verificar) |
| C6186 | AMS1117-3.3 | Alto |
| C2765186 | USB-C GCT | Medio |
| C1525 | 100nF 0402 | Alto |
| C15850 | 10uF 0805 | Alto |

**Para ICS-43434 (C2069979):** Verificar stock en LCSC antes de pedir. Alternativa si no hay stock: SPH0645LM4H-B (similar, I2S, C2684592).
