#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <TFT_22_ILI9225.h>

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *WEATHER_URL =
  "https://api.open-meteo.com/v1/forecast?latitude=22.6273&longitude=120.3014"
  "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max"
  "&timezone=Asia%2FTaipei&forecast_days=7";

const int DHT_PIN = 14;
const int DHT_TYPE = DHT11;
const int LIGHT_PIN = 33;
const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const unsigned long SENSOR_UPDATE_INTERVAL = 10000;
const unsigned long WEATHER_UPDATE_INTERVAL = 60000;
const unsigned long PAGE_INTERVAL = 8000;

const uint16_t UI_BACKGROUND = 0x0008;
const uint16_t UI_CARD = 0x10A2;
const uint16_t UI_CYAN = 0x07FF;
const uint16_t UI_YELLOW = 0xFFE0;
const uint16_t UI_GREEN = 0x07E0;
const uint16_t UI_RED = 0xF800;
const uint16_t UI_WHITE = 0xFFFF;
const uint16_t UI_GRAY = 0xBDF7;
const uint16_t UI_TRACK = 0x4208;

struct ForecastDay {
  String date;
  int weatherCode;
  int maxTemperature;
  int minTemperature;
  int rainProbability;
};

DHT dht(DHT_PIN, DHT_TYPE);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0);
SPIClass vspi(VSPI);
ForecastDay forecast[7];

float temperature = 0;
float humidity = 0;
int lightPercent = 0;
bool sensorValid = false;
int forecastCount = 0;
int currentPage = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastPageChange = 0;

const char *weatherLabel(int weatherCode) {
  if (weatherCode == 0) return "SUN";
  if (weatherCode <= 3) return "CLOUD";
  if (weatherCode <= 48) return "FOG";
  if (weatherCode <= 57) return "DRIZZ";
  if (weatherCode <= 67) return "RAIN";
  if (weatherCode <= 77) return "SNOW";
  if (weatherCode <= 82) return "SHOWER";
  if (weatherCode <= 86) return "SNOW";
  return "STORM";
}

uint16_t weatherColor(int weatherCode) {
  if (weatherCode == 0) return UI_YELLOW;
  if (weatherCode <= 3) return UI_GRAY;
  if (weatherCode <= 67 || weatherCode <= 82) return UI_CYAN;
  if (weatherCode <= 86) return UI_WHITE;
  return UI_RED;
}

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

void drawSensorPage() {
  clearScreen();
  tft.setFont(Terminal6x8);
  tft.drawText(18, 10, "ESP32 SENSOR DISPLAY", UI_CYAN);
  tft.drawText(56, 25, "LIVE VALUES", UI_GRAY);

  tft.fillRectangle(7, 36, 168, 89, UI_CARD);
  drawThermometer(16, 37, UI_RED);
  tft.drawText(50, 47, "TEMPERATURE", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(50, 62, sensorValid ? String((int)temperature) + " C" : "ERROR", sensorValid ? UI_YELLOW : UI_RED);

  tft.fillRectangle(7, 95, 168, 148, UI_CARD);
  drawDroplet(14, 94, UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(50, 106, "HUMIDITY", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(50, 121, sensorValid ? String((int)humidity) + " %" : "ERROR", sensorValid ? UI_CYAN : UI_RED);

  tft.fillRectangle(7, 154, 168, 207, UI_CARD);
  drawSun(12, 154, UI_YELLOW);
  tft.setFont(Terminal6x8);
  tft.drawText(50, 165, "LIGHT", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(50, 180, String(lightPercent) + " %", lightPercent < 25 ? UI_RED : UI_GREEN);

  tft.setFont(Terminal6x8);
  tft.drawText(12, 212, sensorValid ? "DHT11 OK / GPIO14" : "DHT11 ERROR", sensorValid ? UI_GREEN : UI_RED);
}

void drawSunIcon(int x, int y, uint16_t color) {
  tft.fillCircle(x + 7, y + 7, 4, color);
  tft.drawLine(x + 7, y, x + 7, y + 3, color);
  tft.drawLine(x + 7, y + 11, x + 7, y + 14, color);
  tft.drawLine(x, y + 7, x + 3, y + 7, color);
  tft.drawLine(x + 11, y + 7, x + 14, y + 7, color);
  tft.drawLine(x + 2, y + 2, x + 4, y + 4, color);
  tft.drawLine(x + 10, y + 10, x + 12, y + 12, color);
  tft.drawLine(x + 2, y + 12, x + 4, y + 10, color);
  tft.drawLine(x + 10, y + 4, x + 12, y + 2, color);
}

void drawCloudIcon(int x, int y, uint16_t color) {
  tft.fillCircle(x + 5, y + 8, 4, color);
  tft.fillCircle(x + 9, y + 6, 5, color);
  tft.fillCircle(x + 13, y + 9, 4, color);
  tft.fillRectangle(x + 4, y + 8, x + 14, y + 12, color);
}

void drawWeatherIcon(int x, int y, int weatherCode) {
  if (weatherCode == 0) {
    drawSunIcon(x, y, UI_YELLOW);
  } else if (weatherCode <= 3) {
    drawCloudIcon(x, y, UI_GRAY);
  } else if (weatherCode <= 48) {
    drawCloudIcon(x, y, UI_WHITE);
    tft.drawLine(x + 2, y + 15, x + 15, y + 15, UI_GRAY);
  } else if (weatherCode <= 67 || weatherCode <= 82) {
    drawCloudIcon(x, y, UI_CYAN);
    tft.drawLine(x + 5, y + 13, x + 3, y + 17, UI_CYAN);
    tft.drawLine(x + 10, y + 13, x + 8, y + 17, UI_CYAN);
    tft.drawLine(x + 15, y + 13, x + 13, y + 17, UI_CYAN);
  } else if (weatherCode <= 86) {
    drawCloudIcon(x, y, UI_WHITE);
    tft.fillCircle(x + 5, y + 16, 1, UI_WHITE);
    tft.fillCircle(x + 10, y + 16, 1, UI_WHITE);
    tft.fillCircle(x + 15, y + 16, 1, UI_WHITE);
  } else {
    drawCloudIcon(x, y, UI_GRAY);
    tft.fillTriangle(x + 9, y + 10, x + 5, y + 17, x + 10, y + 14, UI_YELLOW);
    tft.fillTriangle(x + 10, y + 14, x + 14, y + 8, x + 9, y + 11, UI_YELLOW);
  }
}
void drawWeatherPage() {
  clearScreen();
  tft.setFont(Terminal12x16);
  tft.drawText(10, 7, "KAOHSIUNG", UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(48, 27, "7-DAY FORECAST", UI_GRAY);

  if (forecastCount == 0) {
    tft.drawText(22, 100, "NO WEATHER DATA", UI_RED);
    tft.drawText(8, 210, "CHECK WIFI / API", UI_GRAY);
    return;
  }

  for (int index = 0; index < forecastCount; index++) {
    int y = 45 + index * 23;
    String dateText = forecast[index].date.substring(5);
    String line = dateText + " " + weatherLabel(forecast[index].weatherCode);
    line += " " + String(forecast[index].minTemperature) + "/";
    line += String(forecast[index].maxTemperature) + "C ";
    line += String(forecast[index].rainProbability) + "%";
    drawWeatherIcon(8, y - 3, forecast[index].weatherCode);
    tft.drawText(30, y, line, weatherColor(forecast[index].weatherCode));
  }
  tft.drawText(8, 210, "UPDATE: 60 SEC", UI_GRAY);
}

bool parseForecast(const String &payload) {
  JsonDocument document;
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.printf("JSON parse failed: %s\n", error.c_str());
    return false;
  }

  JsonArray dates = document["daily"]["time"].as<JsonArray>();
  JsonArray codes = document["daily"]["weather_code"].as<JsonArray>();
  JsonArray maxTemperatures = document["daily"]["temperature_2m_max"].as<JsonArray>();
  JsonArray minTemperatures = document["daily"]["temperature_2m_min"].as<JsonArray>();
  JsonArray rainProbabilities = document["daily"]["precipitation_probability_max"].as<JsonArray>();
  if (dates.isNull() || codes.isNull() || maxTemperatures.isNull() || minTemperatures.isNull()) {
    return false;
  }

  forecastCount = min(7, (int)dates.size());
  for (int index = 0; index < forecastCount; index++) {
    forecast[index].date = dates[index].as<const char *>();
    forecast[index].weatherCode = codes[index].as<int>();
    forecast[index].maxTemperature = (int)round(maxTemperatures[index].as<float>());
    forecast[index].minTemperature = (int)round(minTemperatures[index].as<float>());
    forecast[index].rainProbability = rainProbabilities[index].as<int>();
    Serial.printf("%s %s %d/%dC rain %d%%\n", forecast[index].date.c_str(), weatherLabel(forecast[index].weatherCode), forecast[index].minTemperature, forecast[index].maxTemperature, forecast[index].rainProbability);
  }
  return forecastCount == 7;
}

bool fetchForecast() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(250);
    }
  }
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(client, WEATHER_URL)) return false;
  int httpStatus = http.GET();
  if (httpStatus != HTTP_CODE_OK) {
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();
  return parseForecast(payload);
}

void updateSensors() {
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  sensorValid = !isnan(newHumidity) && !isnan(newTemperature);
  if (sensorValid) {
    humidity = newHumidity;
    temperature = newTemperature;
  }
  lightPercent = constrain(map(analogRead(LIGHT_PIN), 0, 4095, 0, 100), 0, 100);
  Serial.printf("Temperature: %.1f C, Humidity: %.1f %%, Light: %d %%, DHT: %s\n", temperature, humidity, lightPercent, sensorValid ? "OK" : "ERROR");
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
  drawSensorPage();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  fetchForecast();
  lastSensorUpdate = millis();
  lastWeatherUpdate = millis();
  lastPageChange = millis();
}

void loop() {
  unsigned long now = millis();
  if (lastSensorUpdate == 0 || now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    updateSensors();
    if (currentPage == 0) drawSensorPage();
    lastSensorUpdate = now;
  }
  if (lastWeatherUpdate == 0 || now - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
    if (fetchForecast() && currentPage == 1) drawWeatherPage();
    lastWeatherUpdate = now;
  }
  if (now - lastPageChange >= PAGE_INTERVAL) {
    currentPage = currentPage == 0 ? 1 : 0;
    if (currentPage == 0) drawSensorPage();
    else drawWeatherPage();
    lastPageChange = now;
  }
  delay(50);
}




