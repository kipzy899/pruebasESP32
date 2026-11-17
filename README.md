# ESP32 + DHT11 — Estación IoT

Un ESP32 lee temperatura y humedad con un sensor DHT11 y envía los datos de dos formas:

- **MQTT** (HiveMQ Cloud, TLS) cada 10 s, para ver datos en vivo.
- **HTTPS** a una Supabase Edge Function cada 60 s, que guarda el historial en PostgreSQL.

La web (`index.html`) lee el historial de Supabase y se publica en Vercel:
https://pruebas-esp-32.vercel.app

## Archivos

| Archivo | Para qué sirve |
|---|---|
| `temperatura/temperatura.ino` | Firmware del ESP32 |
| `temperatura/secrets.h.example` | Plantilla de credenciales (copiar a `secrets.h`, que NO se sube) |
| `schema.sql` | Crea la tabla `lecturas_sensor` en Supabase |
| `agregacion.sql` | Crea la función `lecturas_agrupadas` que usan los gráficos |
| `index.ts` | Edge Function `mqtt-ingest` (se despliega en Supabase, no en Vercel) |
| `index.html` | Dashboard web |

## Puesta en marcha

1. Supabase → SQL Editor: ejecutar `schema.sql` y después `agregacion.sql`.
2. Desplegar `index.ts` como Edge Function `mqtt-ingest` y definir el secret `WEBHOOK_SECRET`.
3. En `index.html` completar `SUPABASE_URL` y `SUPABASE_ANON` (la clave anon es pública por diseño).
4. En `temperatura/`: copiar `secrets.h.example` a `secrets.h`, completarlo, compilar y subir al ESP32.

## Seguridad

No subir nunca `secrets.h`, la clave `service_role` de Supabase ni contraseñas. El `.gitignore` ya excluye `secrets.h`.
