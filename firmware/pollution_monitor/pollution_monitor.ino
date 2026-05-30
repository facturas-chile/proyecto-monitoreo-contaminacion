/*
 * ============================================================
 * SISTEMA DE MONITOREO DE CONTAMINACIÓN COMUNAL
 * ============================================================
 * Placa    : Arduino Uno WiFi Rev2
 * Versión  : 2.0.0
 *
 * Al iniciar pregunta si deseas calibrar sensores.
 * Los valores de calibración se guardan en EEPROM.
 * ============================================================
 */

#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "config.h"   // Credenciales WiFi y Supabase (ver config.example.h)

// ── Pines ──────────────────────────────────────────────────
#define PIN_PPD42_P1      2
#define PIN_AM2302        4
#define PIN_SOUND_ANALOG  A0
#define PIN_SOUND_GATE    5
#define PIN_MQ2_ANALOG    A1
#define PIN_LDR           A2

// ── Constantes fijas ───────────────────────────────────────
#define DHT_TYPE          DHT22
#define SAMPLE_INTERVAL   30000UL
#define PPD42_SAMPLE_MS   30000UL
#define VREF              3.3
#define ADC_MAX           1023.0
#define R_FIXED_LDR       10000.0
#define TIMEOUT_CAL       30000UL  // 30s timeout en calibración

// ── API ────────────────────────────────────────────────────
const int   API_PORT  = 443;
const char* API_PATH  = "/rest/v1/mediciones";

// ── EEPROM: direcciones de almacenamiento ──────────────────
// Guardamos un "magic number" para saber si la EEPROM
// tiene datos válidos de calibración.
#define EEPROM_MAGIC      0xCA  // byte de validación
#define ADDR_MAGIC        0     // 1 byte
#define ADDR_V_REF_SOUND  1     // 4 bytes (float)
#define ADDR_OFFSET_DB    5     // 4 bytes (float)
#define ADDR_LDR_A        9     // 4 bytes (float)
#define ADDR_LDR_B        13    // 4 bytes (float)
#define ADDR_MQ2_RO       17    // 4 bytes (float)

// ── Parámetros de calibración (con defaults) ───────────────
float CAL_V_REF_SOUND = 0.006;   // voltaje en silencio (Sound)
float CAL_OFFSET_DB   = 48.0;    // offset dB
float CAL_LDR_A       = 12518.0; // coef. curva LDR
float CAL_LDR_B       = -1.405;  // exp. curva LDR
float CAL_MQ2_RO      = 0.0;     // Ro del MQ-2 (0 = no calibrado)

// ── Objetos globales ───────────────────────────────────────
DHT dht(PIN_AM2302, DHT_TYPE);
WiFiSSLClient wifiClient;
HttpClient    httpClient(wifiClient, API_HOST, API_PORT);

unsigned long pulsosP1            = 0;
unsigned long tiempoInicioMuestra = 0;

// ── Prototipos ────────────────────────────────────────────
void  menuCalibracion();
void  calibrarSonido();
void  calibrarLDR();
void  calibrarMQ2();
void  guardarEEPROM();
void  cargarEEPROM();
void  mostrarCalActual();
float leerTemperatura();
float leerHumedad();
float leerPolvo();
float leerSonidoDB();
float leerGasMQ2();
float leerLuz();
bool  enviarDatos(float temp, float hum, float polvo,
                  float db, float gas, float luz);
void  conectarWiFi();
void  imprimirEstado(float t, float h, float polvo,
                     float db, float g, float l);
String leerLineaSerial(unsigned long timeout);
void  esperarEnter();

// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println(F("\n╔══════════════════════════════════════════╗"));
  Serial.println(F("║   MONITOR DE CONTAMINACIÓN  v2.0         ║"));
  Serial.println(F("╚══════════════════════════════════════════╝"));

  dht.begin();
  pinMode(PIN_PPD42_P1, INPUT);
  pinMode(PIN_SOUND_GATE, INPUT);

  // Cargar calibración guardada en EEPROM
  cargarEEPROM();

  // ── Menú de calibración al inicio ─────────────────────
  Serial.println(F("\n¿Deseas calibrar los sensores ahora?"));
  Serial.println(F("  S = Sí    N = No (usar calibración guardada)"));
  Serial.print(F("  Tienes 10 segundos → "));

  String resp = leerLineaSerial(10000UL);
  resp.toUpperCase();
  resp.trim();

  if (resp == "S") {
    menuCalibracion();
  } else {
    Serial.println(F("\nUsando calibración guardada."));
    mostrarCalActual();
  }

  // ── Conectar WiFi ──────────────────────────────────────
  conectarWiFi();

  tiempoInicioMuestra = millis();
  Serial.println(F("\nIniciando monitoreo...\n"));
}

// ═══════════════════════════════════════════════════════════
void loop() {
  unsigned long ahora = millis();
  if (digitalRead(PIN_PPD42_P1) == LOW) pulsosP1++;

  if (ahora - tiempoInicioMuestra >= SAMPLE_INTERVAL) {
    float temperatura = leerTemperatura();
    float humedad     = leerHumedad();
    float polvo       = leerPolvo();
    float sonido      = leerSonidoDB();
    float gas         = leerGasMQ2();
    float luz         = leerLuz();

    imprimirEstado(temperatura, humedad, polvo, sonido, gas, luz);

    if (WiFi.status() != WL_CONNECTED) conectarWiFi();

    bool ok = enviarDatos(temperatura, humedad, polvo, sonido, gas, luz);
    Serial.println(ok ? F("[API] ✓ Enviado") : F("[API] ✗ Error"));
    Serial.println(F("──────────────────────────────────────────"));

    pulsosP1 = 0;
    tiempoInicioMuestra = millis();
  }
}

// ═══════════════════════════════════════════════════════════
//  MENÚ DE CALIBRACIÓN
// ═══════════════════════════════════════════════════════════
void menuCalibracion() {
  while (true) {
    Serial.println(F("\n╔══════════════════════════════════════════╗"));
    Serial.println(F("║         MENÚ DE CALIBRACIÓN              ║"));
    Serial.println(F("╠══════════════════════════════════════════╣"));
    Serial.println(F("║  1 → Calibrar Sonido (dB)                ║"));
    Serial.println(F("║  2 → Calibrar Luz (LDR / lux)            ║"));
    Serial.println(F("║  3 → Calibrar Gas (MQ-2 / Ro)            ║"));
    Serial.println(F("║  4 → Ver calibración actual              ║"));
    Serial.println(F("║  5 → Restaurar defaults de fábrica       ║"));
    Serial.println(F("║  0 → Salir y comenzar monitoreo          ║"));
    Serial.println(F("╚══════════════════════════════════════════╝"));
    Serial.print(F("Opción: "));

    String op = leerLineaSerial(TIMEOUT_CAL);
    op.trim();

    if      (op == "1") calibrarSonido();
    else if (op == "2") calibrarLDR();
    else if (op == "3") calibrarMQ2();
    else if (op == "4") mostrarCalActual();
    else if (op == "5") {
      CAL_V_REF_SOUND = 0.006;
      CAL_OFFSET_DB   = 48.0;
      CAL_LDR_A       = 12518.0;
      CAL_LDR_B       = -1.405;
      CAL_MQ2_RO      = 0.0;
      guardarEEPROM();
      Serial.println(F("✓ Defaults restaurados y guardados."));
    }
    else if (op == "0") {
      Serial.println(F("Saliendo del menú de calibración..."));
      break;
    }
    else {
      Serial.println(F("Opción no válida."));
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  CALIBRACIÓN SONIDO
// ═══════════════════════════════════════════════════════════
void calibrarSonido() {
  Serial.println(F("\n── CALIBRACIÓN DE SONIDO ──────────────────"));
  Serial.println(F("Vamos a medir el voltaje de tu micrófono en silencio."));
  Serial.println(F("\nPaso 1: Asegúrate de estar en SILENCIO TOTAL."));
  Serial.println(F("Presiona Enter cuando estés listo..."));
  esperarEnter();

  Serial.println(F("Midiendo silencio durante 5 segundos..."));
  long suma = 0;
  int  n    = 0;
  unsigned long inicio = millis();
  while (millis() - inicio < 5000) {
    suma += analogRead(PIN_SOUND_ANALOG);
    n++;
    delayMicroseconds(500);
  }
  float promedio  = (float)suma / n;
  float vSilencio = promedio * (VREF / ADC_MAX);

  Serial.print(F("  ADC promedio en silencio : ")); Serial.println(promedio, 1);
  Serial.print(F("  Voltaje en silencio      : ")); Serial.print(vSilencio, 5); Serial.println(F(" V"));

  Serial.println(F("\nPaso 2: ¿Tienes un sonómetro o app de dB como referencia?"));
  Serial.println(F("  S = Sí (calibración con referencia)"));
  Serial.println(F("  N = No (solo ajustar V_ref al silencio medido)"));
  Serial.print(F("  → "));

  String resp = leerLineaSerial(TIMEOUT_CAL);
  resp.toUpperCase(); resp.trim();

  float nuevoOffset = CAL_OFFSET_DB;

  if (resp == "S") {
    Serial.println(F("\nHaz un ruido conocido cerca del sensor (palmada, voz)."));
    Serial.println(F("¿Cuántos dB marca tu app de referencia? Escribe el número y Enter:"));
    Serial.print(F("  dB referencia → "));
    String sDb = leerLineaSerial(TIMEOUT_CAL);
    float dbRef = sDb.toFloat();

    // Medir voltaje pico durante 3 s con ese ruido
    Serial.println(F("Ahora haz el mismo ruido durante 3 segundos..."));
    long sumaR = 0; int nR = 0;
    unsigned long t0 = millis();
    while (millis() - t0 < 3000) {
      sumaR += analogRead(PIN_SOUND_ANALOG);
      nR++;
      delayMicroseconds(500);
    }
    float vRuido = ((float)sumaR / nR) * (VREF / ADC_MAX);

    // offset = dbRef - 20*log10(vRuido / vSilencio)
    if (vRuido > vSilencio && vSilencio > 0) {
      nuevoOffset = dbRef - 20.0 * log10(vRuido / vSilencio);
      Serial.print(F("  Offset calculado: ")); Serial.println(nuevoOffset, 2);
    } else {
      Serial.println(F("  No se detectó diferencia de nivel. Manteniendo offset actual."));
    }
  }

  // Aplicar y guardar
  CAL_V_REF_SOUND = vSilencio;
  CAL_OFFSET_DB   = nuevoOffset;
  guardarEEPROM();

  Serial.println(F("\n✓ Calibración de sonido guardada:"));
  Serial.print(F("  V_ref  = ")); Serial.println(CAL_V_REF_SOUND, 5);
  Serial.print(F("  Offset = ")); Serial.println(CAL_OFFSET_DB, 2);
}

// ═══════════════════════════════════════════════════════════
//  CALIBRACIÓN LDR
// ═══════════════════════════════════════════════════════════
void calibrarLDR() {
  Serial.println(F("\n── CALIBRACIÓN DE LUZ (LDR) ───────────────"));
  Serial.println(F("Necesitas 2 mediciones en condiciones de luz distintas."));
  Serial.println(F("Si tienes una app de luxómetro en el celular, úsala."));
  Serial.println(F("Si no, usa: oscuridad ≈ 0 lux, luz normal ≈ 300 lux.\n"));

  float adcPts[2], luxPts[2];

  for (int i = 0; i < 2; i++) {
    Serial.print(F("── Punto ")); Serial.print(i + 1); Serial.println(F(" de 2 ──────────────────────"));
    if (i == 0) Serial.println(F("Condición: POCA LUZ (tapa el sensor o ve a un cuarto oscuro)"));
    else        Serial.println(F("Condición: BUENA LUZ (luz normal de habitación o exterior)"));

    Serial.println(F("Presiona Enter cuando estés listo..."));
    esperarEnter();

    // Medir ADC promedio 3 s
    long suma = 0; int n = 0;
    unsigned long t0 = millis();
    while (millis() - t0 < 3000) {
      suma += analogRead(PIN_LDR);
      n++;
      delay(10);
    }
    adcPts[i] = (float)suma / n;
    Serial.print(F("  ADC medido: ")); Serial.println(adcPts[i], 1);

    Serial.print(F("  ¿Cuántos lux marca tu app? (si no tienes, escribe "));
    Serial.println(i == 0 ? F("5): ") : F("300): "));
    String sLux = leerLineaSerial(TIMEOUT_CAL);
    luxPts[i] = (sLux.toFloat() > 0) ? sLux.toFloat() : (i == 0 ? 5.0 : 300.0);
    Serial.print(F("  Lux usado: ")); Serial.println(luxPts[i], 1);
  }

  // Calcular a y b
  // lux = a * R^b  →  b = (log(L1)-log(L2)) / (log(R1)-log(R2))
  float v1  = adcPts[0] * (VREF / ADC_MAX);
  float v2  = adcPts[1] * (VREF / ADC_MAX);
  float R1  = (v1 > 0.01) ? (R_FIXED_LDR * (VREF / v1 - 1.0)) : 999999;
  float R2  = (v2 > 0.01) ? (R_FIXED_LDR * (VREF / v2 - 1.0)) : 1;

  if (R1 != R2 && luxPts[0] > 0 && luxPts[1] > 0) {
    float b = (log10(luxPts[0]) - log10(luxPts[1]))
            / (log10(R1)        - log10(R2));
    float a = luxPts[0] / pow(R1, b);

    CAL_LDR_A = a;
    CAL_LDR_B = b;
    guardarEEPROM();

    Serial.println(F("\n✓ Calibración de luz guardada:"));
    Serial.print(F("  a = ")); Serial.println(CAL_LDR_A, 2);
    Serial.print(F("  b = ")); Serial.println(CAL_LDR_B, 4);
  } else {
    Serial.println(F("\n✗ Los puntos son demasiado similares. Repite con más diferencia de luz."));
  }
}

// ═══════════════════════════════════════════════════════════
//  CALIBRACIÓN MQ-2
// ═══════════════════════════════════════════════════════════
void calibrarMQ2() {
  Serial.println(F("\n── CALIBRACIÓN MQ-2 (Gas) ─────────────────"));
  Serial.println(F("El MQ-2 necesita calentarse al menos 3 minutos antes de calibrar."));
  Serial.println(F("Asegúrate de estar en AIRE LIMPIO (sin humo, sin gas cerca)."));
  Serial.print(F("\n¿Ya lleva más de 3 min encendido? S/N → "));

  String resp = leerLineaSerial(TIMEOUT_CAL);
  resp.toUpperCase(); resp.trim();

  if (resp != "S") {
    Serial.println(F("Esperando 3 minutos de calentamiento..."));
    for (int i = 3; i >= 1; i--) {
      Serial.print(i); Serial.println(F(" min restantes..."));
      delay(60000);
    }
  }

  Serial.println(F("\nMidiendo resistencia en aire limpio (Ro)..."));
  Serial.println(F("Presiona Enter cuando estés listo..."));
  esperarEnter();

  // Medir Rs promedio durante 10 s
  long suma = 0; int n = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < 10000) {
    suma += analogRead(PIN_MQ2_ANALOG);
    n++;
    delay(50);
  }
  float rawProm = (float)suma / n;
  float v       = rawProm * (VREF / ADC_MAX);
  float RL      = 10000.0;
  float Rs      = (v > 0) ? ((VREF - v) / v * RL) : 0;
  // En aire limpio Rs/Ro = 9.8 para MQ-2 según datasheet
  float Ro      = Rs / 9.8;

  CAL_MQ2_RO = Ro;
  guardarEEPROM();

  Serial.println(F("\n✓ Calibración MQ-2 guardada:"));
  Serial.print(F("  Rs (aire limpio) = ")); Serial.print(Rs, 0); Serial.println(F(" Ω"));
  Serial.print(F("  Ro calculado     = ")); Serial.print(Ro, 0); Serial.println(F(" Ω"));
  Serial.println(F("  (Ro se usará en futuras lecturas de ppm)"));
}

// ═══════════════════════════════════════════════════════════
//  EEPROM
// ═══════════════════════════════════════════════════════════
void guardarEEPROM() {
  EEPROM.write(ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.put(ADDR_V_REF_SOUND, CAL_V_REF_SOUND);
  EEPROM.put(ADDR_OFFSET_DB,   CAL_OFFSET_DB);
  EEPROM.put(ADDR_LDR_A,       CAL_LDR_A);
  EEPROM.put(ADDR_LDR_B,       CAL_LDR_B);
  EEPROM.put(ADDR_MQ2_RO,      CAL_MQ2_RO);
  Serial.println(F("  [EEPROM] Calibración guardada."));
}

void cargarEEPROM() {
  if (EEPROM.read(ADDR_MAGIC) == EEPROM_MAGIC) {
    EEPROM.get(ADDR_V_REF_SOUND, CAL_V_REF_SOUND);
    EEPROM.get(ADDR_OFFSET_DB,   CAL_OFFSET_DB);
    EEPROM.get(ADDR_LDR_A,       CAL_LDR_A);
    EEPROM.get(ADDR_LDR_B,       CAL_LDR_B);
    EEPROM.get(ADDR_MQ2_RO,      CAL_MQ2_RO);
    Serial.println(F("[EEPROM] Calibración previa cargada."));
  } else {
    Serial.println(F("[EEPROM] Sin calibración guardada. Usando defaults."));
  }
}

void mostrarCalActual() {
  Serial.println(F("\n── Calibración actual ──────────────────────"));
  Serial.print(F("  Sonido V_ref  : ")); Serial.println(CAL_V_REF_SOUND, 5);
  Serial.print(F("  Sonido offset : ")); Serial.println(CAL_OFFSET_DB, 2);
  Serial.print(F("  LDR a         : ")); Serial.println(CAL_LDR_A, 2);
  Serial.print(F("  LDR b         : ")); Serial.println(CAL_LDR_B, 4);
  Serial.print(F("  MQ-2 Ro       : "));
  if (CAL_MQ2_RO > 0) { Serial.print(CAL_MQ2_RO, 0); Serial.println(F(" Ω")); }
  else                   Serial.println(F("no calibrado (usando default)"));
}

// ═══════════════════════════════════════════════════════════
//  SENSORES
// ═══════════════════════════════════════════════════════════
float leerTemperatura() {
  float t = dht.readTemperature();
  if (isnan(t)) { Serial.println(F("[DHT] Error temperatura")); return -999.0; }
  return t;
}

float leerHumedad() {
  float h = dht.readHumidity();
  if (isnan(h)) { Serial.println(F("[DHT] Error humedad")); return -999.0; }
  return h;
}

float leerPolvo() {
  // Limitar ratio a máximo 100% para evitar overflow en la fórmula cúbica
  float ratio = (float)pulsosP1 / (float)PPD42_SAMPLE_MS * 100.0;
  ratio = constrain(ratio, 0.0f, 100.0f);
  float conc  = (1.1f * pow(ratio, 3)) - (3.8f * pow(ratio, 2))
              + (520.0f * ratio) + 0.62f;
  // Límite razonable para PM10: OMS emergencia = 250 μg/m³
  conc = constrain(conc, 0.0f, 1000.0f);
  return conc;
}

float leerSonidoDB() {
  long suma = 0;
  for (int i = 0; i < 64; i++) { suma += analogRead(PIN_SOUND_ANALOG); delayMicroseconds(300); }
  float v = (suma / 64.0) * (VREF / ADC_MAX);
  if (v <= 0 || CAL_V_REF_SOUND <= 0) return 0.0;
  return max(0.0f, 20.0f * log10(v / CAL_V_REF_SOUND) + CAL_OFFSET_DB);
}

float leerGasMQ2() {
  long suma = 0;
  for (int i = 0; i < 10; i++) { suma += analogRead(PIN_MQ2_ANALOG); delay(5); }
  float v  = (suma / 10.0) * (VREF / ADC_MAX);
  float RL = 10000.0;
  float Rs = (v > 0) ? ((VREF - v) / v * RL) : 0;
  float Ro = (CAL_MQ2_RO > 0) ? CAL_MQ2_RO : (Rs / 9.8);
  if (Ro <= 0) return 0;
  return max(0.0f, (float)pow(10.0, (log10(Rs / Ro) - 1.58) / (-0.45)));
}

float leerLuz() {
  float v   = analogRead(PIN_LDR) * (VREF / ADC_MAX);
  if (v <= 0.01) return 0.0;
  float R   = R_FIXED_LDR * (VREF / v - 1.0);
  return max(0.0f, CAL_LDR_A * (float)pow(R, CAL_LDR_B));
}

// ═══════════════════════════════════════════════════════════
//  API
// ═══════════════════════════════════════════════════════════

// Serializa float seguro: si es nan/inf envía 0
String safeFloat(float v) {
  if (isnan(v) || isinf(v)) return "0.00";
  return String(v, 2);
}

bool enviarDatos(float temp, float hum, float polvo,
                 float db,   float gas, float luz) {
  // Construir JSON manualmente para evitar "ovf" e "inf"
  String body = "{";
  body += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  body += "\"comuna\":\"" + String(COMUNA) + "\",";
  body += "\"temperatura\":" + safeFloat(temp) + ",";
  body += "\"humedad\":" + safeFloat(hum) + ",";
  body += "\"polvo_pm10\":" + safeFloat(polvo) + ",";
  body += "\"sonido_db\":" + safeFloat(db) + ",";
  body += "\"gas_ppm\":" + safeFloat(gas) + ",";
  body += "\"luminosidad\":" + safeFloat(luz);
  body += "}";
  Serial.print(F("[API] JSON: ")); Serial.println(body);

  httpClient.beginRequest();
  httpClient.post(API_PATH);
  httpClient.sendHeader("Content-Type",  "application/json");
  httpClient.sendHeader("apikey",         API_KEY);
  httpClient.sendHeader("Authorization", String("Bearer ") + API_KEY);
  httpClient.sendHeader("Prefer",        "return=minimal");
  httpClient.sendHeader("Content-Length", body.length());
  httpClient.beginBody();
  httpClient.print(body);
  httpClient.endRequest();
  int code = httpClient.responseStatusCode();
  httpClient.stop();
  Serial.print(F("[API] HTTP: ")); Serial.println(code);
  return (code == 201);
}

// ═══════════════════════════════════════════════════════════
//  WIFI
// ═══════════════════════════════════════════════════════════
void conectarWiFi() {
  Serial.print(F("[WiFi] Conectando a: ")); Serial.println(WIFI_SSID);
  int intentos = 0;
  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED) {
    delay(2000); Serial.print(F("."));
    if (++intentos > 10) { Serial.println(F("\n[WiFi] Reintentando...")); delay(30000); intentos = 0; }
  }
  Serial.println(F("\n[WiFi] ¡Conectado!"));
  Serial.print(F("[WiFi] IP: ")); Serial.println(WiFi.localIP());
}

// ═══════════════════════════════════════════════════════════
//  MONITOR SERIE
// ═══════════════════════════════════════════════════════════
void imprimirEstado(float t, float h, float polvo,
                    float db, float g, float l) {
  Serial.println(F("\n┌──────────────────────────────┐"));
  Serial.println(F("│     LECTURA DE SENSORES      │"));
  Serial.println(F("├──────────────────────────────┤"));
  Serial.print(F("│ Temperatura : ")); Serial.print(t);     Serial.println(F(" °C"));
  Serial.print(F("│ Humedad     : ")); Serial.print(h);     Serial.println(F(" %"));
  Serial.print(F("│ Polvo PM10  : ")); Serial.print(polvo); Serial.println(F(" μg/m³"));
  Serial.print(F("│ Sonido      : ")); Serial.print(db);    Serial.println(F(" dB"));
  Serial.print(F("│ Gas MQ-2    : ")); Serial.print(g);     Serial.println(F(" ppm"));
  Serial.print(F("│ Luminosidad : ")); Serial.print(l);     Serial.println(F(" lux"));
  Serial.println(F("└──────────────────────────────┘"));
}

// ═══════════════════════════════════════════════════════════
//  HELPERS SERIAL
// ═══════════════════════════════════════════════════════════

// Lee una línea del Serial con timeout (ms). Retorna "" si timeout.
String leerLineaSerial(unsigned long timeout) {
  String resultado = "";
  unsigned long inicio = millis();
  while (millis() - inicio < timeout) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (resultado.length() > 0) {
          Serial.println(resultado);
          return resultado;
        }
      } else {
        resultado += c;
      }
    }
  }
  Serial.println(F("(timeout)"));
  return resultado;
}

// Espera Enter sin timeout
void esperarEnter() {
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') return;
    }
  }
}
