#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <TFT_22_ILI9225.h>

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *MQTT_SERVER = "mqttgo.io";
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "alex/class301/data";

const int DHT_PIN = 14;
const int DHT_TYPE = DHT11;
const int LIGHT_PIN = 33;
const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const unsigned long DATA_UPDATE_INTERVAL = 10000;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;

const uint16_t UI_BACKGROUND = 0x0008;
const uint16_t UI_CARD = 0x10A2;
const uint16_t UI_CYAN = 0x07FF;
const uint16_t UI_YELLOW = 0xFFE0;
const uint16_t UI_GREEN = 0x07E0;
const uint16_t UI_RED = 0xF800;
const uint16_t UI_WHITE = 0xFFFF;
const uint16_t UI_GRAY = 0xBDF7;

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
SPIClass vspi(VSPI);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0);

float temperature = 0;
float humidity = 0;
int lightPercent = 0;
bool sensorValid = false;
bool mqttPublishOK = false;
unsigned long lastDataUpdate = 0;
unsigned long lastMqttReconnect = 0;

void clearScreen() {
  tft.fillRectangle(0, 0, 175, 219, UI_BACKGROUND);
}

void drawThermometer(int x, int y, uint16_t color) {
  tft.drawCircle(x + 11, y + 29, 10, color);
  tft.fillCircle(x + 11, y + 29, 8, color);
  tft.drawRectangle(x + 6, y + 2, x + 16, y + 29, color);
  tft.fillRectangle(x + 9, y + 6, x + 13, y + 29, color);
}

void drawDroplet(int x, int y, uint16_t color) {
  tft.fillCircle(x + 11, y + 23, 10, color);
  tft.fillTriangle(x + 1, y + 23, x + 21, y + 23, x + 11, y + 2, color);
}

void drawSun(int x, int y, uint16_t color) {
  tft.fillCircle(x + 12, y + 12, 9, color);
  tft.drawLine(x + 12, y, x + 12, y + 3, color);
  tft.drawLine(x + 12, y + 21, x + 12, y + 24, color);
  tft.drawLine(x, y + 12, x + 3, y + 12, color);
  tft.drawLine(x + 21, y + 12, x + 24, y + 12, color);
  tft.drawLine(x + 3, y + 3, x + 6, y + 6, color);
  tft.drawLine(x + 18, y + 18, x + 21, y + 21, color);
  tft.drawLine(x + 3, y + 21, x + 6, y + 18, color);
  tft.drawLine(x + 18, y + 6, x + 21, y + 3, color);
}

void drawStatusBar() {
  tft.fillRectangle(5, 200, 170, 219, UI_CARD);
  tft.setFont(Terminal12x16);
  tft.drawText(8, 201, "WIFI:", UI_WHITE);
  tft.drawText(68, 201, WiFi.status() == WL_CONNECTED ? "O" : "X", WiFi.status() == WL_CONNECTED ? UI_GREEN : UI_RED);
  tft.drawText(86, 201, "MQTT:", UI_WHITE);
  tft.drawText(146, 201, mqttClient.connected() && mqttPublishOK ? "O" : "X", mqttClient.connected() && mqttPublishOK ? UI_GREEN : UI_RED);
}

void drawSensorPage() {
  clearScreen();
  tft.setFont(Terminal6x8);
  tft.drawText(18, 8, "ESP32 SENSOR DISPLAY", UI_CYAN);
  tft.drawText(60, 21, "MQTT DATA", UI_GRAY);

  tft.fillRectangle(7, 29, 168, 82, UI_CARD);
  drawThermometer(16, 32, UI_RED);
  tft.setFont(Terminal6x8);
  tft.drawText(50, 42, "TEMPERATURE", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(50, 57, sensorValid ? String((int)temperature) + " C" : "ERROR", sensorValid ? UI_YELLOW : UI_RED);

  tft.fillRectangle(7, 87, 168, 140, UI_CARD);
  drawDroplet(14, 95, UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(50, 100, "HUMIDITY", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(50, 115, sensorValid ? String((int)humidity) + " %" : "ERROR", sensorValid ? UI_CYAN : UI_RED);

  tft.fillRectangle(7, 145, 168, 198, UI_CARD);
  drawSun(12, 157, UI_YELLOW);
  tft.setFont(Terminal6x8);
  tft.drawText(50, 159, "LIGHT", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(50, 174, String(lightPercent) + " %", lightPercent < 25 ? UI_RED : UI_GREEN);

  drawStatusBar();
}

void updateSensorValues() {
  tft.fillRectangle(48, 55, 103, 81, UI_CARD);
  tft.setFont(Terminal12x16);
  tft.drawText(50, 57, sensorValid ? String((int)temperature) + " C" : "ERROR", sensorValid ? UI_YELLOW : UI_RED);

  tft.fillRectangle(48, 113, 103, 139, UI_CARD);
  tft.drawText(50, 115, sensorValid ? String((int)humidity) + " %" : "ERROR", sensorValid ? UI_CYAN : UI_RED);

  tft.fillRectangle(48, 171, 103, 197, UI_CARD);
  tft.drawText(50, 174, String(lightPercent) + " %", lightPercent < 25 ? UI_RED : UI_GREEN);
}

void showConnectionMessage(const char *message, uint16_t color) {
  clearScreen();
  tft.setFont(Terminal12x16);
  tft.drawText(18, 30, "ESP32 MQTT", UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(24, 70, message, color);
  drawStatusBar();
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  showConnectionMessage("WIFI CONNECTING", UI_YELLOW);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    drawStatusBar();
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    showConnectionMessage("WIFI ERROR", UI_RED);
  } else {
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  }
}

bool connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return false;
  showConnectionMessage("MQTT CONNECTING", UI_YELLOW);
  String clientId = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("MQTT connected");
    return true;
  }
  Serial.printf("MQTT connection failed, state=%d\n", mqttClient.state());
  return false;
}

void updateSensorData() {
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  sensorValid = !isnan(newHumidity) && !isnan(newTemperature);
  if (sensorValid) {
    humidity = newHumidity;
    temperature = newTemperature;
  }
  lightPercent = 100 - constrain(map(analogRead(LIGHT_PIN), 0, 4095, 0, 100), 0, 100);
}

void publishSensorData() {
  char payload[80];
  snprintf(payload, sizeof(payload), "{\"temp\":%d,\"humi\":%d,\"light\":%d}", (int)temperature, (int)humidity, lightPercent);
  mqttPublishOK = mqttClient.publish(MQTT_TOPIC, payload);
  Serial.printf("MQTT publish %s: %s\n", mqttPublishOK ? "OK" : "FAILED", payload);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  dht.begin();
  analogReadResolution(12);
  vspi.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.begin(vspi);
  tft.setOrientation(0);
  tft.setBackgroundColor(UI_BACKGROUND);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  showConnectionMessage("STARTING", UI_CYAN);
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
  }
  updateSensorData();
  drawSensorPage();
  lastDataUpdate = millis();
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    mqttPublishOK = false;
    connectWiFi();
  }

  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && now - lastMqttReconnect >= MQTT_RECONNECT_INTERVAL) {
    lastMqttReconnect = now;
    mqttPublishOK = connectMQTT();
    drawStatusBar();
  }

  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  if (now - lastDataUpdate >= DATA_UPDATE_INTERVAL) {
    updateSensorData();
    if (mqttClient.connected()) {
      publishSensorData();
    } else {
      mqttPublishOK = false;
    }
    updateSensorValues();
    drawStatusBar();
    lastDataUpdate = now;
  }

  delay(50);
}




