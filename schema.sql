-- ─────────────────────────────────────────────────────────────────
-- Ejecutar en Supabase → SQL Editor
-- ─────────────────────────────────────────────────────────────────

-- Tabla principal de lecturas
CREATE TABLE IF NOT EXISTS lecturas_sensor (
  id            BIGSERIAL PRIMARY KEY,
  temperatura   NUMERIC(5,2) NOT NULL,
  humedad       NUMERIC(5,2) NOT NULL,
  indice_calor  NUMERIC(5,2),
  sensor        TEXT DEFAULT 'DHT11',
  dispositivo   TEXT DEFAULT 'esp32-dht11-001',
  topic         TEXT DEFAULT 'sensores/dht11',
  created_at    TIMESTAMPTZ DEFAULT now()
);

-- Índice para consultas por fecha
CREATE INDEX IF NOT EXISTS idx_lecturas_created_at
  ON lecturas_sensor (created_at DESC);

-- Habilitar Row Level Security
ALTER TABLE lecturas_sensor ENABLE ROW LEVEL SECURITY;

-- Política: lectura pública (para la web en GitHub Pages)
CREATE POLICY "Lectura pública"
  ON lecturas_sensor FOR SELECT
  USING (true);

-- Política: solo el service role puede insertar (via Edge Function)
CREATE POLICY "Solo service role inserta"
  ON lecturas_sensor FOR INSERT
  WITH CHECK (auth.role() = 'service_role');

-- Vista útil: últimas 24 horas con promedios por hora
CREATE OR REPLACE VIEW resumen_24h AS
SELECT
  date_trunc('hour', created_at) AS hora,
  ROUND(AVG(temperatura)::numeric, 1)  AS temp_promedio,
  ROUND(MAX(temperatura)::numeric, 1)  AS temp_max,
  ROUND(MIN(temperatura)::numeric, 1)  AS temp_min,
  ROUND(AVG(humedad)::numeric, 1)      AS hum_promedio,
  COUNT(*)                              AS total_lecturas
FROM lecturas_sensor
WHERE created_at >= now() - INTERVAL '24 hours'
GROUP BY date_trunc('hour', created_at)
ORDER BY hora DESC;
