#include <SPI.h>
#include <math.h>
#include <WiFi.h>
#include <time.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <TFT_22_ILI9225.h>

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *MQTT_SERVER = "mqttgo.io";
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "alex/class301/data";
const char *GREEN_CONTROL_TOPIC = "alex/class301/ctrl/gled";
const char *YELLOW_CONTROL_TOPIC = "alex/class301/ctrl/yled";
const char *RED_CONTROL_TOPIC = "alex/class301/ctrl/led4";

const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;

const int DHT_PIN = 14;
const int DHT_TYPE = DHT11;
const int LIGHT_PIN = 33;
const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;
const int PAGE_BUTTON_PIN = 0;

const unsigned long DATA_UPDATE_INTERVAL = 10000;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;
const unsigned long CLOCK_UPDATE_INTERVAL = 10000;

const uint16_t UI_BACKGROUND = 0x0008;
const uint16_t UI_CARD = 0x10A2;
const uint16_t UI_CYAN = 0x07FF;
const uint16_t UI_YELLOW = 0xFFE0;
const uint16_t UI_GREEN = 0x07E0;
const uint16_t UI_RED = 0xF800;
const uint16_t UI_WHITE = 0xFFFF;
const uint16_t UI_GRAY = 0xBDF7;
const uint16_t UI_TRACK = 0x4208;

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
unsigned long lastClockUpdate = 0;
unsigned long lastButtonChange = 0;
bool lastButtonState = HIGH;
int currentPage = 0;
float temperatureHistory[60];
int temperatureHistoryCount = 0;
float humidityHistory[60];
int humidityHistoryCount = 0;
float lightHistory[60];
int lightHistoryCount = 0;

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
  tft.setFont(Terminal6x8);
  tft.drawText(14, 206, "WIFI:", UI_WHITE);
  tft.drawText(50, 206, WiFi.status() == WL_CONNECTED ? "O" : "X", WiFi.status() == WL_CONNECTED ? UI_GREEN : UI_RED);
  tft.drawText(78, 206, "MQTT:", UI_WHITE);
  tft.drawText(120, 206, mqttClient.connected() ? "O" : "X", mqttClient.connected() ? UI_GREEN : UI_RED);
}

void drawDateTime() {
  struct tm timeInfo;
  char timeText[24];
  if (getLocalTime(&timeInfo, 10)) {
    strftime(timeText, sizeof(timeText), "%m/%d %a %H:%M", &timeInfo);
  } else {
    snprintf(timeText, sizeof(timeText), "TIME SYNC ERROR");
  }
  tft.fillRectangle(48, 18, 170, 29, UI_BACKGROUND);
  tft.setFont(Terminal6x8);
  tft.drawText(50, 20, timeText, UI_GRAY);
}

void syncNtpTime() {
  showConnectionMessage("TIME SYNC", UI_YELLOW);
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeInfo;
  if (getLocalTime(&timeInfo, 10000)) {
    Serial.println("NTP time synchronized: Taiwan UTC+8");
  } else {
    Serial.println("NTP time synchronization failed");
  }
}

void drawSensorPage() {
  clearScreen();
  tft.setFont(Terminal6x8);
  tft.drawText(18, 8, "ESP32 SENSOR DISPLAY", UI_CYAN);
  drawDateTime();

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

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  JsonDocument document;
  DeserializationError error = deserializeJson(document, payload, length);
  if (error) {
    Serial.printf("Control JSON parse failed: %s\n", error.c_str());
    return;
  }

  const char *topicText = topic;
  const char *state = document["state"] | "";
  if (strlen(state) == 0) {
    if (String(topicText) == GREEN_CONTROL_TOPIC) state = document["gled"] | "";
    else if (String(topicText) == YELLOW_CONTROL_TOPIC) state = document["yled"] | "";
    else if (String(topicText) == RED_CONTROL_TOPIC) {
      state = document["led4"] | "";
      if (strlen(state) == 0) state = document["rled"] | "";
    }
  }
  if (strcmp(state, "on") != 0 && strcmp(state, "off") != 0) return;
  bool turnOn = strcmp(state, "on") == 0;

  if (String(topicText) == GREEN_CONTROL_TOPIC) {
    digitalWrite(GREEN_LED_PIN, turnOn ? HIGH : LOW);
    Serial.printf("Green LED: %s\n", state);
  } else if (String(topicText) == YELLOW_CONTROL_TOPIC) {
    digitalWrite(YELLOW_LED_PIN, turnOn ? HIGH : LOW);
    Serial.printf("Yellow LED: %s\n", state);
  } else if (String(topicText) == RED_CONTROL_TOPIC) {
    digitalWrite(RED_LED_PIN, turnOn ? HIGH : LOW);
    Serial.printf("Red LED: %s\n", state);
  }
}

bool connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return false;
  showConnectionMessage("MQTT CONNECTING", UI_YELLOW);
  String clientId = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("MQTT connected");
    bool greenSubscribed = mqttClient.subscribe(GREEN_CONTROL_TOPIC);
    bool yellowSubscribed = mqttClient.subscribe(YELLOW_CONTROL_TOPIC);
    bool redSubscribed = mqttClient.subscribe(RED_CONTROL_TOPIC);
    Serial.printf("MQTT subscribe gled=%s yled=%s rled=%s\n", greenSubscribed ? "OK" : "FAILED", yellowSubscribed ? "OK" : "FAILED", redSubscribed ? "OK" : "FAILED");
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

void addHistorySample(float history[], int &historyCount, float value) {
  if (historyCount < 60) {
    history[historyCount++] = value;
    return;
  }
  for (int index = 1; index < 60; index++) {
    history[index - 1] = history[index];
  }
  history[59] = value;
}

const uint8_t TINY_DIGITS[10][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111},
  {0b010, 0b110, 0b010, 0b010, 0b111},
  {0b110, 0b001, 0b010, 0b100, 0b111},
  {0b110, 0b001, 0b010, 0b001, 0b110},
  {0b101, 0b101, 0b111, 0b001, 0b001},
  {0b111, 0b100, 0b110, 0b001, 0b110},
  {0b011, 0b100, 0b111, 0b101, 0b111},
  {0b111, 0b001, 0b010, 0b010, 0b010},
  {0b111, 0b101, 0b111, 0b101, 0b111},
  {0b111, 0b101, 0b111, 0b001, 0b110}
};

void drawTinyNumber(int value, int x, int y, uint16_t color) {
  String text = String(value);
  for (uint8_t digitIndex = 0; digitIndex < text.length(); digitIndex++) {
    uint8_t digit = text[digitIndex] - '0';
    for (uint8_t row = 0; row < 5; row++) {
      for (uint8_t column = 0; column < 3; column++) {
        if (TINY_DIGITS[digit][row] & (1 << (2 - column))) {
          tft.drawPixel(x + digitIndex * 4 + column, y + row, color);
        }
      }
    }
  }
}

void drawGaugeArc(int centerX, int centerY, int radius, int startAngle, int endAngle, uint16_t color) {
  for (int angle = startAngle; angle < endAngle; angle++) {
    float firstRadians = angle * 3.1415926 / 180.0;
    float secondRadians = (angle + 1) * 3.1415926 / 180.0;
    int x1 = centerX + cos(firstRadians) * radius;
    int y1 = centerY + sin(firstRadians) * radius;
    int x2 = centerX + cos(secondRadians) * radius;
    int y2 = centerY + sin(secondRadians) * radius;
    tft.drawLine(x1, y1, x2, y2, color);
    for (int thickness = 1; thickness <= 7; thickness++) {
      int innerRadius = radius - thickness;
      tft.drawLine(centerX + cos(firstRadians) * innerRadius, centerY + sin(firstRadians) * innerRadius, centerX + cos(secondRadians) * innerRadius, centerY + sin(secondRadians) * innerRadius, color);
    }
  }
}

void drawMetricGauge(float value, int sampleCount, float minimum, float maximum, const char *minimumLabel, const char *maximumLabel, const char *unit) {
  const int centerX = 88;
  const int centerY = 91;
  const int radius = 45;
  drawGaugeArc(centerX, centerY, radius, 180, 240, UI_GREEN);
  drawGaugeArc(centerX, centerY, radius, 240, 300, UI_YELLOW);
  drawGaugeArc(centerX, centerY, radius, 300, 360, UI_RED);
  tft.setFont(Terminal6x8);
  tft.drawText(30, 91, minimumLabel, UI_GRAY);
  tft.drawText(strcmp(maximumLabel, "100") == 0 ? 133 : 139, 91, maximumLabel, UI_GRAY);
  tft.drawText(80, 36, unit, UI_GRAY);

  if (sampleCount == 0) {
    tft.drawText(70, 82, "--", UI_WHITE);
    return;
  }
  float gaugeValue = constrain(value, minimum, maximum);
  float pointerAngle = (180.0 + (gaugeValue - minimum) * 180.0 / (maximum - minimum)) * 3.1415926 / 180.0;
  int pointerX = centerX + cos(pointerAngle) * (radius - 7);
  int pointerY = centerY + sin(pointerAngle) * (radius - 7);
  tft.drawLine(centerX, centerY, pointerX, pointerY, UI_WHITE);
  tft.fillCircle(centerX, centerY, 3, UI_WHITE);
  tft.setFont(Terminal12x16);
  tft.drawText(69, 98, String((int)value) + " " + unit, UI_WHITE);
}

void drawMetricChart(float history[], int historyCount, int minimum, int maximum, uint16_t lineColor, const char *title, const char *unit) {
  clearScreen();
  tft.setFont(Terminal6x8);
  tft.drawText(24, 7, title, UI_CYAN);
  tft.drawText(59, 20, "RECENT TREND", UI_GRAY);
  String minimumLabel = String(minimum);
  String maximumLabel = String(maximum);
  drawMetricGauge(historyCount > 0 ? history[historyCount - 1] : 0, historyCount, minimum, maximum, minimumLabel.c_str(), maximumLabel.c_str(), unit);

  const int chartLeft = 25;
  const int chartRight = 170;
  const int chartTop = 124;
  const int chartBottom = 190;
  tft.drawLine(chartLeft, chartTop, chartLeft, chartBottom, UI_WHITE);
  tft.drawLine(chartLeft, chartBottom, chartRight, chartBottom, UI_WHITE);

  for (int value = minimum; value <= maximum; value += 10) {
    int y = map(value, minimum, maximum, chartBottom, chartTop);
    tft.drawLine(chartLeft + 1, y, chartRight, y, UI_TRACK);
    drawTinyNumber(value, 5, y - 2, UI_GRAY);
  }

  const int sampleWindow = 10;
  int firstSample = max(0, historyCount - sampleWindow);
  int displayedSamples = historyCount - firstSample;
  for (int index = firstSample + 1; index < historyCount; index++) {
    int previousX = map(index - 1 - firstSample, 0, displayedSamples - 1, chartLeft + 2, chartRight);
    int currentX = map(index - firstSample, 0, displayedSamples - 1, chartLeft + 2, chartRight);
    int previousY = map(constrain((int)history[index - 1], minimum, maximum), minimum, maximum, chartBottom - 1, chartTop + 1);
    int currentY = map(constrain((int)history[index], minimum, maximum), minimum, maximum, chartBottom - 1, chartTop + 1);
    tft.drawLine(previousX, previousY, currentX, currentY, lineColor);
    tft.fillCircle(currentX, currentY, 1, lineColor);
  }

  if (historyCount == 0) {
    tft.drawText(54, 157, "WAITING DATA", UI_YELLOW);
  }
  drawStatusBar();
}

void drawTemperatureChart() {
  drawMetricChart(temperatureHistory, temperatureHistoryCount, 10, 40, UI_YELLOW, "TEMP 10 SAMPLES", "C");
}

void drawHumidityChart() {
  drawMetricChart(humidityHistory, humidityHistoryCount, 0, 100, UI_CYAN, "HUMI 10 SAMPLES", "%");
}

void drawLightChart() {
  drawMetricChart(lightHistory, lightHistoryCount, 0, 100, UI_YELLOW, "LIGHT 10 SAMPLES", "%");
}

void drawCurrentPage() {
  if (currentPage == 0) drawSensorPage();
  else if (currentPage == 1) drawTemperatureChart();
  else if (currentPage == 2) drawHumidityChart();
  else drawLightChart();
}

void handlePageButton() {
  bool buttonState = digitalRead(PAGE_BUTTON_PIN);
  unsigned long now = millis();
  if (lastButtonState == HIGH && buttonState == LOW && now - lastButtonChange > 250) {
    currentPage = (currentPage + 1) % 4;
    drawCurrentPage();
    lastButtonChange = now;
  }
  lastButtonState = buttonState;
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
  mqttClient.setCallback(mqttCallback);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);

  showConnectionMessage("STARTING", UI_CYAN);
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
    syncNtpTime();
  }
  updateSensorData();
  drawSensorPage();
  lastDataUpdate = millis();
  lastClockUpdate = millis();
}

void loop() {
  unsigned long now = millis();
  handlePageButton();

  if (WiFi.status() != WL_CONNECTED) {
    mqttPublishOK = false;
    connectWiFi();
    if (currentPage == 0) drawStatusBar();
    else drawCurrentPage();
  }

  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && now - lastMqttReconnect >= MQTT_RECONNECT_INTERVAL) {
    lastMqttReconnect = now;
    mqttPublishOK = connectMQTT();
    if (currentPage == 0) drawStatusBar();
    else drawCurrentPage();
  }

  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  if (now - lastDataUpdate >= DATA_UPDATE_INTERVAL) {
    updateSensorData();
    if (sensorValid) {
      addHistorySample(temperatureHistory, temperatureHistoryCount, temperature);
      addHistorySample(humidityHistory, humidityHistoryCount, humidity);
    }
    addHistorySample(lightHistory, lightHistoryCount, lightPercent);
    if (mqttClient.connected()) {
      publishSensorData();
    } else {
      mqttPublishOK = false;
    }
    if (currentPage == 0) updateSensorValues();
    else drawCurrentPage();
    lastDataUpdate = now;
  }

  if (currentPage == 0 && now - lastClockUpdate >= CLOCK_UPDATE_INTERVAL) {
    drawDateTime();
    lastClockUpdate = now;
  }

  delay(20);
}








