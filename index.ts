/*
 * Supabase Edge Function: mqtt-ingest
 * ─────────────────────────────────────────────────────────────────
 * HiveMQ Cloud llama a este endpoint via Webhook (HTTP Action)
 * cada vez que llega un mensaje al topic "sensores/dht11".
 *
 * Variables de entorno necesarias (en Supabase Dashboard → Edge Functions):
 *   SUPABASE_URL          → automática
 *   SUPABASE_SERVICE_ROLE_KEY → automática
 *   WEBHOOK_SECRET        → string secreto que configurás en HiveMQ
 * ─────────────────────────────────────────────────────────────────
 */

import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const supabase = createClient(
  Deno.env.get("SUPABASE_URL")!,
  Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!
);

const WEBHOOK_SECRET = Deno.env.get("WEBHOOK_SECRET") ?? "";

Deno.serve(async (req: Request) => {
  // ── Validar método ──────────────────────────────────────────────
  if (req.method !== "POST") {
    return new Response("Método no permitido", { status: 405 });
  }

  // ── Validar secret header (seguridad básica) ────────────────────
  const authHeader = req.headers.get("x-webhook-secret");
  if (WEBHOOK_SECRET && authHeader !== WEBHOOK_SECRET) {
    return new Response("No autorizado", { status: 401 });
  }

  // ── Parsear body ────────────────────────────────────────────────
  let body: Record<string, unknown>;
  try {
    body = await req.json();
  } catch {
    return new Response("Body JSON inválido", { status: 400 });
  }

  /*
   * HiveMQ envía el payload así:
   * {
   *   "topic": "sensores/dht11",
   *   "payload": "<base64 o string>",
   *   "timestamp": 1234567890
   * }
   * El payload es el JSON del ESP32:
   * {"temperatura":24.5,"humedad":61.0,"indice_calor":25.1,"sensor":"DHT11","dispositivo":"esp32-dht11-001"}
   */
  let mqttPayload: Record<string, unknown>;
  try {
    const raw = body.payload as string;
    // HiveMQ puede enviar base64
    const decoded = atob(raw).startsWith("{") ? atob(raw) : raw;
    mqttPayload = JSON.parse(decoded);
  } catch {
    // Si no es base64, intentar parsear directo
    try {
      mqttPayload = JSON.parse(body.payload as string);
    } catch {
      return new Response("Payload no parseable", { status: 400 });
    }
  }

  // ── Insertar en Supabase ────────────────────────────────────────
  const { error } = await supabase.from("lecturas_sensor").insert({
    temperatura:  mqttPayload.temperatura,
    humedad:      mqttPayload.humedad,
    indice_calor: mqttPayload.indice_calor,
    sensor:       mqttPayload.sensor ?? "DHT11",
    dispositivo:  mqttPayload.dispositivo ?? "esp32",
    topic:        body.topic ?? "sensores/dht11",
    // created_at lo pone Supabase automáticamente con DEFAULT now()
  });

  if (error) {
    console.error("Error Supabase:", error);
    return new Response(JSON.stringify({ error: error.message }), {
      status: 500,
      headers: { "Content-Type": "application/json" },
    });
  }

  return new Response(JSON.stringify({ ok: true }), {
    status: 200,
    headers: { "Content-Type": "application/json" },
  });
});
