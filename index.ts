/*
 * Supabase Edge Function: mqtt-ingest
 * Recibe las lecturas del suscriptor MQTT y las guarda en lecturas_sensor.
 * Acepta dos formatos:
 *   1) JSON directo:  {"temperatura":24.5,"humedad":61,...}
 *   2) Envuelto:      {"topic":"...","payload":"<json | base64 | objeto>"}
 *
 * Secrets (Dashboard -> Edge Functions -> Secrets):
 *   WEBHOOK_SECRET   (SUPABASE_URL y SUPABASE_SERVICE_ROLE_KEY son automaticas)
 */
import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const supabase = createClient(
  Deno.env.get("SUPABASE_URL")!,
  Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!,
);
const WEBHOOK_SECRET = Deno.env.get("WEBHOOK_SECRET") ?? "";

function json(status: number, body: unknown) {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

// Devuelve el objeto con los datos del sensor, sea cual sea el formato
function extraerLectura(body: Record<string, unknown>): Record<string, unknown> | null {
  if (body.temperatura !== undefined) return body;

  const p = body.payload;
  if (p && typeof p === "object") return p as Record<string, unknown>;
  if (typeof p !== "string") return null;

  try { return JSON.parse(p); } catch { /* quizas base64 */ }
  try { return JSON.parse(atob(p)); } catch { return null; }
}

Deno.serve(async (req: Request) => {
  if (req.method !== "POST") return json(405, { error: "Metodo no permitido" });

  if (WEBHOOK_SECRET && req.headers.get("x-webhook-secret") !== WEBHOOK_SECRET) {
    return json(401, { error: "No autorizado" });
  }

  let body: Record<string, unknown>;
  try { body = await req.json(); } catch { return json(400, { error: "JSON invalido" }); }

  const l = extraerLectura(body);
  if (!l) return json(400, { error: "Payload no parseable" });

  const temperatura = Number(l.temperatura);
  const humedad = Number(l.humedad);
  if (!Number.isFinite(temperatura) || !Number.isFinite(humedad)) {
    return json(400, { error: "temperatura/humedad invalidas" });
  }

  const hi = Number(l.indice_calor);
  const { error } = await supabase.from("lecturas_sensor").insert({
    temperatura,
    humedad,
    indice_calor: Number.isFinite(hi) ? hi : null,
    sensor: l.sensor ?? "DHT11",
    dispositivo: l.dispositivo ?? "esp32",
    topic: body.topic ?? "sensores/dht11",
  });

  if (error) {
    console.error("Error Supabase:", error);
    return json(500, { error: error.message });
  }
  return json(200, { ok: true });
});
