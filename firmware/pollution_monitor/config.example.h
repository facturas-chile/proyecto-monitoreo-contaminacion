/*
 * config.example.h — Template de configuración
 *
 * Copia este archivo como config.h y rellena con tus credenciales reales.
 * config.h está excluido del repositorio via .gitignore.
 *
 * NUNCA subas config.h a un repositorio público o privado compartido.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ── Credenciales WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID     "TU_RED_WIFI"
#define WIFI_PASSWORD "TU_CONTRASENA_WIFI"

// ── Configuración Supabase ─────────────────────────────────────────────────
// Host del proyecto (sin https://)
#define API_HOST  "TU_PROYECTO.supabase.co"

// API Key anon (Settings → API en el panel de Supabase)
#define API_KEY   "TU_SUPABASE_ANON_KEY"

// ── Identificación del dispositivo ────────────────────────────────────────
#define DEVICE_ID "sensor-node-01"
#define COMUNA    "Tu_Ciudad"

#endif // CONFIG_H
