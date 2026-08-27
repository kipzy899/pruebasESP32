Proyecto IoT que lee temperatura y humedad con un sensor DHT11
conectado a un ESP32, publica los datos via MQTT (HiveMQ Cloud)
y los almacena en Supabase para visualizarlos en una web estática
hosteada en GitHub Pages.

## Arquitectura
ESP32 (DHT11) → MQTT (HiveMQ) → Supabase Edge Function → PostgreSQL → GitHub Pages

## Archivos
- `firmware/` — Código Arduino para el ESP32
- `supabase/` — Edge Function y schema SQL
- `web/` — Dashboard web (GitHub Pages)

## Configuración
Ver `GUIA_CONFIGURACION.md`
