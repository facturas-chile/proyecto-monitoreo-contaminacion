# Sistema de Monitoreo de Contaminación Comunal

Firmware para Arduino Uno WiFi Rev2 que lee 6 sensores ambientales y envía datos a Supabase cada 30 segundos.

**Versión:** 2.0.0

## Documentación

La documentación completa del proyecto (hardware, mapa de pines, esquema de base de datos, fórmulas de sensores, procedimiento de calibración e historial de versiones) está disponible en la [Wiki del proyecto](https://paperclip.ing/IAV/issues/IAV-2#document-wiki).

## Configuración rápida

1. Copia `firmware/pollution_monitor/config.example.h` como `config.h` en la misma carpeta.
2. Rellena `config.h` con tus credenciales WiFi y API key de Supabase.
3. Abre `firmware/pollution_monitor/pollution_monitor.ino` en el Arduino IDE.
4. Instala las librerías listadas en `firmware/libraries.md`.
5. Selecciona la placa **Arduino Uno WiFi Rev2** y sube el sketch.

> **Seguridad:** `config.h` está excluido del repositorio via `.gitignore`. Nunca lo subas a un repositorio público ni privado compartido.

## Estructura del repositorio

```
proyecto-monitoreo-contaminacion/
├── firmware/
│   ├── pollution_monitor/
│   │   ├── pollution_monitor.ino   ← Firmware principal
│   │   ├── config.example.h        ← Template de configuración (sin credenciales)
│   │   └── config.h                ← Credenciales reales (en .gitignore)
│   └── libraries.md                ← Lista de librerías necesarias
├── docs/
│   ├── esquema_conexiones.md       ← Diagrama de conexiones
│   └── supabase_schema.sql         ← Schema de la tabla en Supabase
├── .gitignore
└── README.md
```
