#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_22_ILI9225.h>

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const unsigned long WEATHER_UPDATE_INTERVAL = 60000;
const char *WEATHER_URL =
  "https://api.open-meteo.com/v1/forecast?latitude=22.6273&longitude=120.3014"
  "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max"
  "&timezone=Asia%2FTaipei&forecast_days=7";

const uint16_t UI_BACKGROUND = 0x0008;
const uint16_t UI_CYAN = 0x07FF;
const uint16_t UI_BLUE = 0x03FF;
const uint16_t UI_YELLOW = 0xFFE0;
const uint16_t UI_GREEN = 0x07E0;
const uint16_t UI_RED = 0xF800;
const uint16_t UI_WHITE = 0xFFFF;
const uint16_t UI_GRAY = 0xBDF7;

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_MOSI, TFT_SCK, 0);

struct ForecastDay {
  String date;
  int weatherCode;
  int maxTemperature;
  int minTemperature;
  int rainProbability;
};

ForecastDay forecast[7];
int forecastCount = 0;
unsigned long lastWeatherUpdate = 0;

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
  if (weatherCode <= 48) return UI_WHITE;
  if (weatherCode <= 67 || weatherCode <= 82) return UI_CYAN;
  if (weatherCode <= 86) return UI_WHITE;
  return UI_RED;
}

void clearScreen() {
  tft.fillRectangle(0, 0, 175, 219, UI_BACKGROUND);
}

void showStatus(const char *message, uint16_t color) {
  clearScreen();
  tft.setFont(Terminal12x16);
  tft.drawText(20, 20, "WEATHER", UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(18, 58, message, color);
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

void drawRainIcon(int x, int y, uint16_t color) {
  drawCloudIcon(x, y, color);
  tft.drawLine(x + 5, y + 13, x + 3, y + 16, UI_CYAN);
  tft.drawLine(x + 10, y + 13, x + 8, y + 16, UI_CYAN);
  tft.drawLine(x + 15, y + 13, x + 13, y + 16, UI_CYAN);
}

void drawStormIcon(int x, int y, uint16_t color) {
  drawCloudIcon(x, y, color);
  tft.fillTriangle(x + 9, y + 10, x + 5, y + 17, x + 10, y + 14, UI_YELLOW);
  tft.fillTriangle(x + 10, y + 14, x + 14, y + 8, x + 9, y + 11, UI_YELLOW);
}

void drawSnowIcon(int x, int y, uint16_t color) {
  drawCloudIcon(x, y, color);
  tft.fillCircle(x + 5, y + 16, 1, UI_WHITE);
  tft.fillCircle(x + 10, y + 16, 1, UI_WHITE);
  tft.fillCircle(x + 15, y + 16, 1, UI_WHITE);
}

void drawWeatherIcon(int x, int y, int weatherCode) {
  if (weatherCode == 0) {
    drawSunIcon(x, y, UI_YELLOW);
  } else if (weatherCode <= 3) {
    drawCloudIcon(x, y, UI_GRAY);
  } else if (weatherCode <= 48) {
    drawCloudIcon(x, y, UI_WHITE);
    tft.drawLine(x + 3, y + 16, x + 16, y + 16, UI_GRAY);
  } else if (weatherCode <= 67 || weatherCode <= 82) {
    drawRainIcon(x, y, UI_CYAN);
  } else if (weatherCode <= 86) {
    drawSnowIcon(x, y, UI_WHITE);
  } else {
    drawStormIcon(x, y, UI_GRAY);
  }
}
void drawForecast() {
  clearScreen();
  tft.setFont(Terminal12x16);
  tft.drawText(10, 7, "KAOHSIUNG", UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(48, 27, "7-DAY FORECAST", UI_GRAY);

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
    Serial.println("Missing daily forecast fields");
    return false;
  }

  forecastCount = min(7, (int)dates.size());
  for (int index = 0; index < forecastCount; index++) {
    forecast[index].date = dates[index].as<const char *>();
    forecast[index].weatherCode = codes[index].as<int>();
    forecast[index].maxTemperature = (int)round(maxTemperatures[index].as<float>());
    forecast[index].minTemperature = (int)round(minTemperatures[index].as<float>());
    forecast[index].rainProbability = rainProbabilities[index].as<int>();
    Serial.printf("%s %s %d/%dC rain %d%%\n",
                  forecast[index].date.c_str(),
                  weatherLabel(forecast[index].weatherCode),
                  forecast[index].minTemperature,
                  forecast[index].maxTemperature,
                  forecast[index].rainProbability);
  }
  return forecastCount == 7;
}

bool fetchForecast() {
  if (WiFi.status() != WL_CONNECTED) {
    showStatus("WIFI CONNECTING", UI_YELLOW);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(250);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    showStatus("WIFI ERROR", UI_RED);
    return false;
  }

  showStatus("DOWNLOADING", UI_YELLOW);
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(client, WEATHER_URL)) {
    Serial.println("HTTP begin failed");
    showStatus("HTTP ERROR", UI_RED);
    return false;
  }

  int httpStatus = http.GET();
  Serial.printf("HTTP status: %d\n", httpStatus);
  if (httpStatus != HTTP_CODE_OK) {
    http.end();
    showStatus("SERVER ERROR", UI_RED);
    return false;
  }

  String payload = http.getString();
  http.end();
  bool parsed = parseForecast(payload);
  if (!parsed) {
    showStatus("DATA ERROR", UI_RED);
  }
  return parsed;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  tft.begin();
  tft.setOrientation(0);
  tft.setBackgroundColor(UI_BACKGROUND);
  showStatus("STARTING", UI_CYAN);

  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  }

  fetchForecast();
  if (forecastCount == 7) {
    drawForecast();
  }
  lastWeatherUpdate = millis();
}

void loop() {
  if (millis() - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
    if (fetchForecast()) {
      drawForecast();
    }
    lastWeatherUpdate = millis();
  }
  delay(100);
}




