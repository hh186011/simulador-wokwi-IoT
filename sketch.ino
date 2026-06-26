/*
  Estacion Meteorologica IoT - Simulacion Wokwi (BIU CSE6011)
  Hector Hernandez

  Replica la logica real del firmware (lectura BME280 por I2C, pantalla
  OLED de estado, construccion de payload JSON) sobre el BME280 simulado
  (chip personalizado bme280_custom.chip.c/.json incluido en este
  proyecto).

  IMPORTANTE: a diferencia del firmware real, esta version NO envia el
  HTTP POST al endpoint real de AWS API Gateway. Eso es intencional:
  el objetivo de esta simulacion es validar la logica de lectura del
  sensor y construccion del JSON, no inyectar datos sinteticos en la
  base de datos de produccion (Supabase) que alimenta el dashboard real
  de Grafana. El intento de POST queda registrado por consola (Serial)
  para que se pueda verificar el payload exacto que se hubiera enviado.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define I2C_SDA 8
#define I2C_SCL 9

Adafruit_BME280 bme;                 // I2C, direccion 0x76
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid     = "Wokwi-GUEST"; // red WiFi virtual de Wokwi (con salida a internet)
const char* password = "";
const char* deviceId  = "ESP32S3_SENSOR_01_SIM";
const float TEMP_OFFSET = 0.0; // sin auto-calentamiento real que compensar en simulacion

const unsigned long INTERVALO_MS = 6000; // 6s en sim ~ equivalente a un ciclo de muestra
unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n=== Estacion Meteorologica IoT - Simulacion Wokwi ==="));

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!bme.begin(0x76)) {
    Serial.println(F("ERROR: no se encontro el BME280 (revisar diagram.json / direccion 0x76)"));
  } else {
    Serial.println(F("BME280 (simulado) detectado correctamente en 0x76"));
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("ERROR: no se encontro el OLED SSD1306 en 0x3C"));
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Estacion IoT (SIM)"));
    display.println(F("Conectando WiFi..."));
    display.display();
  }

  WiFi.begin(ssid, password);
  Serial.print(F("Conectando a WiFi "));
  Serial.print(ssid);
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi conectado. IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("No se logro conectar a Wokwi-GUEST (continua solo con lectura de sensor)"));
  }
}

void loop() {
  if (millis() - ultimoEnvio >= INTERVALO_MS) {
    ultimoEnvio = millis();

    float temperatura = bme.readTemperature() - TEMP_OFFSET;
    float humedad      = bme.readHumidity();
    float presion       = bme.readPressure() / 100.0F; // Pa -> hPa

    Serial.println(F("---------------------------------------------"));
    Serial.print(F("Temperatura: ")); Serial.print(temperatura, 2); Serial.println(F(" C"));
    Serial.print(F("Humedad:     "));  Serial.print(humedad, 2);     Serial.println(F(" %"));
    Serial.print(F("Presion:     "));  Serial.print(presion, 2);     Serial.println(F(" hPa"));

    // Construccion del payload JSON, igual que el firmware real
    StaticJsonDocument<200> doc;
    doc["device_id"]   = deviceId;
    doc["temperature"] = temperatura;
    doc["humidity"]    = humedad;
    doc["pressure"]    = presion;
    String payload;
    serializeJson(doc, payload);

    Serial.print(F("Payload JSON: "));
    Serial.println(payload);
    Serial.println(F("[SIMULACION] No se envia HTTP POST real (se evita escribir"));
    Serial.println(F("datos sinteticos en la base de datos de produccion Supabase)."));

    // Actualizar OLED con el ultimo estado, igual que el dispositivo real
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Estacion IoT (SIM)"));
    display.print(F("T: ")); display.print(temperatura, 1); display.println(F(" C"));
    display.print(F("H: ")); display.print(humedad, 1);     display.println(F(" %"));
    display.print(F("P: ")); display.print(presion, 1);     display.println(F(" hPa"));
    display.println(WiFi.status() == WL_CONNECTED ? F("WiFi: OK") : F("WiFi: --"));
    display.display();
  }
}
