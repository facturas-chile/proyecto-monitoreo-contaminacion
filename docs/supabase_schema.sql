-- Tabla: mediciones

CREATE TABLE IF NOT EXISTS mediciones (
    id            bigserial PRIMARY KEY,
    created_at    timestamptz DEFAULT now(),
    device_id     text        NOT NULL,
    comuna        text        NOT NULL,
    temperatura   float,
    humedad       float,
    polvo_pm10    float,
    sonido_db     float,
    gas_ppm       float,
    luminosidad   float
);

-- Índice para consultas por dispositivo y tiempo
CREATE INDEX IF NOT EXISTS idx_mediciones_device_time
    ON mediciones (device_id, created_at DESC);
