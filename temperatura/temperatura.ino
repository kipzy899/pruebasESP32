/*
 * ─────────────────────────────────────────────────────────────────
 *  ESP32 + DHT11  →  MQTT (HiveMQ Cloud)  →  Supabase
 * ─────────────────────────────────────────────────────────────────
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// ── WiFi ──────────────────────────────────────────────────────────
const char* WIFI_SSID     = "ISET57-CLARO";
const char* WIFI_PASSWORD = "GONZALO1981";

// ── MQTT HiveMQ Cloud ─────────────────────────────────────────────
const char* MQTT_BROKER   = "g13a1066.ala.us-east-1.emqxsl.com";   
const int   MQTT_PORT     = 8883;                            
const char* MQTT_USER     = "javiercarlucci@proton.me";                  
const char* MQTT_PASSWORD = "iset572026";                 
const char* MQTT_CLIENT_ID = "esp32-dht11-001";
const char* MQTT_TOPIC    = "sensores/dht11";

// ── DHT11 ─────────────────────────────────────────────────────────
#define DHTPIN  4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ── Intervalo de publicación (ms) ─────────────────────────────────
const unsigned long PUBLISH_INTERVAL = 10000;   // 10 segundos

static const char* ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoBggIBAK3oJHP0FDfzm54rVygc
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

// ── Variables globales ────────────────────────────────────────────
WiFiClientSecure espClient;
PubSubClient     mqttClient(espClient);
unsigned long    lastPublish = 0;

// ─────────────────────────────────────────────────────────────────
void connectWifi() {
  Serial.printf("\nConectando a WiFi: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n✓ WiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println(" ✓ OK");
    } else {
      Serial.printf(" ✗ Error rc=%d — reintentando en 5s\n", mqttClient.state());
      delay(5000);
    }
  }
}

void publishSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("⚠ Error leyendo DHT11");
    return;
  }

  // Calcular índice de calor
  float hi = dht.computeHeatIndex(t, h, false);

  // Construir JSON
  StaticJsonDocument<200> doc;
  doc["temperatura"]   = round(t * 10.0) / 10.0;
  doc["humedad"]       = round(h * 10.0) / 10.0;
  doc["indice_calor"]  = round(hi * 10.0) / 10.0;
  doc["sensor"]        = "DHT11";
  doc["dispositivo"]   = MQTT_CLIENT_ID;

  char buffer[256];
  size_t n = serializeJson(doc, buffer);

  if (mqttClient.publish(MQTT_TOPIC, buffer, n)) {
    Serial.printf("✓ Publicado → %s | T:%.1f°C H:%.1f%% HI:%.1f°C\n", MQTT_TOPIC, t, h, hi);
  } else {
    Serial.println("✗ Error al publicar");
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  dht.begin();

  connectWifi();

  espClient.setCACert(ROOT_CA);
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setKeepAlive(60);

  connectMqtt();

  Serial.println("Sistema listo. Publicando cada 10 s.");
}

void loop() {
  if (!mqttClient.connected()) connectMqtt();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = now;
    publishSensor();
  }
}
