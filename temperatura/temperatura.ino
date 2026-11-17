/*
 * ESP32 + DHT11 -> doble envio, SIN servidor intermedio
 *   1) MQTT (HiveMQ Cloud, TLS validado)  cada 10 s  -> vista en vivo
 *   2) HTTPS POST a Supabase Edge Function cada 60 s -> historial
 *
 * Librerias: DHT sensor library, Adafruit Unified Sensor, PubSubClient, ArduinoJson v7
 * Credenciales: copiar secrets.h.example a secrets.h y completar.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "secrets.h"

const int   MQTT_PORT      = 8883;
const char* MQTT_CLIENT_ID = "esp32-dht11-001";
const char* MQTT_TOPIC     = "sensores/dht11";

#define DHTPIN  4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const unsigned long MQTT_INTERVAL     = 10000;  // 10 s
const unsigned long SUPABASE_INTERVAL = 60000;  // 60 s (cuida la cuota de Supabase)

// ISRG Root X1 (Let's Encrypt) - la usa HiveMQ Cloud. Huella SHA-256 verificada:
// 96:BC:EC:06:26:49:76:F3:74:60:77:9A:CF:28:C5:A7:CF:E8:A3:C0:AA:E1:1A:8F:FC:EE:05:C0:BD:DF:08:C6
static const char ROOT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

WiFiClientSecure mqttSecure;          // MQTT, certificado validado
PubSubClient     mqttClient(mqttSecure);
unsigned long    lastMqtt = 0, lastSupabase = 0;

void connectWifi() {
  Serial.printf("\nConectando a WiFi: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\nWiFi OK - IP: %s\n", WiFi.localIP().toString().c_str());
}

void syncTime() {  // TLS valida fechas: hace falta hora real
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.print("Sincronizando hora");
  while (time(nullptr) < 1700000000) { delay(500); Serial.print("."); }
  Serial.println(" OK");
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    Serial.printf("Conectando MQTT a %s ...", MQTT_BROKER);
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println(" OK");
    } else {
      Serial.printf(" fallo rc=%d, reintento en 5 s\n", mqttClient.state());
      delay(5000);
    }
  }
}

bool leerSensor(float &t, float &h, float &hi) {
  h = dht.readHumidity();
  t = dht.readTemperature();
  if (isnan(h) || isnan(t)) { Serial.println("Error leyendo DHT11"); return false; }
  hi = dht.computeHeatIndex(t, h, false);
  return true;
}

size_t armarJson(char *buf, size_t len, float t, float h, float hi) {
  JsonDocument doc;
  doc["temperatura"]  = round(t  * 10.0) / 10.0;
  doc["humedad"]      = round(h  * 10.0) / 10.0;
  doc["indice_calor"] = round(hi * 10.0) / 10.0;
  doc["sensor"]       = "DHT11";
  doc["dispositivo"]  = MQTT_CLIENT_ID;
  return serializeJson(doc, buf, len);
}

void publicarMqtt(const char *json, size_t n) {
  if (mqttClient.publish(MQTT_TOPIC, (const uint8_t*)json, n))
    Serial.println("MQTT  -> publicado");
  else
    Serial.println("MQTT  -> error al publicar");
}

void enviarSupabase(const char *json) {
  // AVISO DE SEGURIDAD: este POST va cifrado (HTTPS) pero NO valida el certificado
  // del servidor (setInsecure). Alguien con acceso a tu red podria interceptarlo.
  // Para un proyecto de aprendizaje es aceptable; para algo serio, cargar la CA raiz.
  WiFiClientSecure https;
  https.setInsecure();

  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(https, SUPABASE_FUNC_URL)) { Serial.println("HTTP  -> begin fallo"); return; }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("x-webhook-secret", WEBHOOK_SECRET);

  int code = http.POST((uint8_t*)json, strlen(json));
  if (code == 200) Serial.println("HTTP  -> Supabase OK (200)");
  else             Serial.printf("HTTP  -> Supabase codigo %d\n", code);
  http.end();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  connectWifi();
  syncTime();

  mqttSecure.setCACert(ROOT_CA);
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(15);
  connectMqtt();
  Serial.println("Sistema listo");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWifi();
  if (!mqttClient.connected()) connectMqtt();
  mqttClient.loop();

  unsigned long now = millis();
  bool tocaMqtt = (now - lastMqtt >= MQTT_INTERVAL);
  bool tocaSupa = (now - lastSupabase >= SUPABASE_INTERVAL);
  if (!tocaMqtt && !tocaSupa) return;

  float t, h, hi;
  if (!leerSensor(t, h, hi)) { lastMqtt = lastSupabase = now; return; }

  char json[256];
  size_t n = armarJson(json, sizeof(json), t, h, hi);
  Serial.printf("T:%.1f H:%.1f HI:%.1f\n", t, h, hi);

  if (tocaMqtt) { lastMqtt = now;     publicarMqtt(json, n); }
  if (tocaSupa) { lastSupabase = now; enviarSupabase(json); }
}
