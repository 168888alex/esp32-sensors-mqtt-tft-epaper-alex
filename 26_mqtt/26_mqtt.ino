#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int OLED_SDA = 21;
const int OLED_SCL = 22;
const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_TOPIC = "alex/class301/data";

const unsigned long SENSOR_INTERVAL_MS = 1000;
const unsigned long MQTT_INTERVAL_MS = 10000;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
SimpleDHT11 dht11(DHT_PIN);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastSensorMillis = 0;
unsigned long lastMqttMillis = 0;
byte temperature = 0;
byte humidity = 0;
int lightPercent = 0;
bool sensorValid = false;

void setLedState(bool green, bool yellow, bool red) {
  digitalWrite(GREEN_LED_PIN, green ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, yellow ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, red ? HIGH : LOW);
}

void updateLedState() {
  if (!sensorValid) {
    setLedState(false, false, true);
    return;
  }

  bool critical = temperature >= 32 || humidity >= 80;
  bool warning = temperature >= 28 || humidity >= 70;
  if (critical) {
    setLedState(false, false, true);
  } else if (warning) {
    setLedState(false, true, false);
  } else {
    setLedState(true, false, false);
  }
}

void drawDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("TEMP");
  display.setCursor(64, 0);
  display.print("HUMI");
  display.setCursor(0, 36);
  display.print("LIGHT");
  display.setCursor(76, 36);
  display.print("MQTT");

  display.setTextSize(2);
  display.setCursor(0, 12);
  if (sensorValid) {
    display.print(temperature);
    display.print(" C");
  } else {
    display.print("ERR");
  }

  display.setCursor(64, 12);
  if (sensorValid) {
    display.print(humidity);
    display.print("%");
  } else {
    display.print("ERR");
  }

  display.setCursor(0, 48);
  display.print(lightPercent);
  display.print("%");

  display.setCursor(76, 48);
  display.print(mqttClient.connected() ? "OK" : "--");
  display.display();
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi connection failed");
  }
}

void connectMqtt() {
  if (mqttClient.connected() || WiFi.status() != WL_CONNECTED) return;
  char clientId[40];
  uint64_t chipId = ESP.getEfuseMac();
  snprintf(clientId, sizeof(clientId), "esp32-%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  Serial.printf("Connecting to MQTT as %s...\n", clientId);
  if (mqttClient.connect(clientId)) {
    Serial.println("MQTT connected");
  } else {
    Serial.printf("MQTT connection failed, state=%d\n", mqttClient.state());
  }
}

void readSensors() {
  byte nextTemperature = 0;
  byte nextHumidity = 0;
  int error = dht11.read(&nextTemperature, &nextHumidity, NULL);
  int lightRaw = analogRead(LIGHT_PIN);
  lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
  sensorValid = error == SimpleDHTErrSuccess;
  if (sensorValid) {
    temperature = nextTemperature;
    humidity = nextHumidity;
    Serial.printf("Temperature: %d C, Humidity: %d %%, Light: %d %% (raw %d)\n", temperature, humidity, lightPercent, lightRaw);
  } else {
    Serial.printf("DHT11 read failed, error=%d\n", error);
  }
  updateLedState();
  drawDisplay();
}

void publishData() {
  if (!sensorValid || !mqttClient.connected()) return;
  char payload[64];
  snprintf(payload, sizeof(payload), "{\"temp\":%d,\"humi\":%d,\"light\":%d}", temperature, humidity, lightPercent);
  if (mqttClient.publish(MQTT_TOPIC, payload)) {
    Serial.printf("MQTT published: %s\n", payload);
  } else {
    Serial.println("MQTT publish failed");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  setLedState(false, true, false);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED initialization failed");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MQTT SENSOR");
    display.println("Connecting...");
    display.display();
  }

  connectWiFi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectMqtt();
  readSensors();
}

void loop() {
  connectWiFi();
  connectMqtt();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastSensorMillis >= SENSOR_INTERVAL_MS || lastSensorMillis == 0) {
    lastSensorMillis = now;
    readSensors();
  }
  if (now - lastMqttMillis >= MQTT_INTERVAL_MS || lastMqttMillis == 0) {
    lastMqttMillis = now;
    publishData();
  }
  delay(10);
}
