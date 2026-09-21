#include <SPI.h>
#include <SimpleDHT.h>
#include <TFT_22_ILI9225.h>

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const uint16_t UI_BACKGROUND = 0x0008;
const uint16_t UI_CARD = 0x10A2;
const uint16_t UI_TRACK = 0x4208;
const uint16_t UI_CYAN = 0x07FF;
const uint16_t UI_YELLOW = 0xFFE0;
const uint16_t UI_GREEN = 0x07E0;
const uint16_t UI_RED = 0xF800;
const uint16_t UI_WHITE = 0xFFFF;
const uint16_t UI_GRAY = 0xBDF7;

SimpleDHT11 dht11(DHT_PIN);
SPIClass vspi(VSPI);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0);

void clearFullScreen(uint16_t color) {
  tft.fillRectangle(0, 0, 175, 219, color);
}

void refreshDisplayMemory() {
  clearFullScreen(COLOR_BLACK);
  delay(60);
  clearFullScreen(COLOR_WHITE);
  delay(60);
  clearFullScreen(COLOR_BLACK);
  delay(60);
  clearFullScreen(UI_BACKGROUND);
}

void drawThermometer(int x, int y, uint16_t color) {
  tft.drawCircle(x + 6, y + 16, 5, color);
  tft.fillCircle(x + 6, y + 16, 3, color);
  tft.drawRectangle(x + 3, y + 3, x + 9, y + 16, color);
  tft.fillRectangle(x + 5, y + 7, x + 7, y + 16, color);
}

void drawDroplet(int x, int y, uint16_t color) {
  tft.fillCircle(x + 7, y + 13, 5, color);
  tft.fillTriangle(x + 2, y + 13, x + 12, y + 13, x + 7, y + 3, color);
}

void drawSun(int x, int y, uint16_t color) {
  tft.drawCircle(x + 7, y + 10, 5, color);
  tft.drawLine(x + 7, y, x + 7, y + 3, color);
  tft.drawLine(x + 7, y + 17, x + 7, y + 20, color);
  tft.drawLine(x, y + 10, x + 3, y + 10, color);
  tft.drawLine(x + 11, y + 10, x + 14, y + 10, color);
  tft.drawLine(x + 2, y + 5, x + 4, y + 7, color);
  tft.drawLine(x + 10, y + 13, x + 12, y + 15, color);
  tft.drawLine(x + 2, y + 15, x + 4, y + 13, color);
  tft.drawLine(x + 10, y + 7, x + 12, y + 5, color);
}

void drawProgressBar(int x, int y, int width, int percent, uint16_t color) {
  int filled = (width - 2) * constrain(percent, 0, 100) / 100;
  tft.fillRectangle(x, y, x + width, y + 8, UI_TRACK);
  if (filled > 0) {
    tft.fillRectangle(x + 1, y + 1, x + filled, y + 7, color);
  }
}

void drawCard(int y, uint16_t accent) {
  tft.fillRectangle(6, y, 169, y + 43, UI_CARD);
  tft.fillRectangle(6, y, 9, y + 43, accent);
}

void drawDashboard(uint8_t temperature, uint8_t humidity, int lightPercent, bool sensorValid) {
  clearFullScreen(UI_BACKGROUND);

  tft.setFont(Terminal12x16);
  tft.drawText(28, 8, "ENV MONITOR", UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(12, 30, "LIVE SENSOR STATUS", UI_GRAY);

  drawCard(44, UI_YELLOW);
  drawThermometer(16, 53, UI_YELLOW);
  tft.setFont(Terminal6x8);
  tft.drawText(36, 50, "TEMPERATURE", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(36, 64, sensorValid ? String(temperature) + " C" : "ERR", sensorValid ? UI_YELLOW : UI_RED);
  drawProgressBar(103, 62, 57, sensorValid ? map(temperature, 0, 50, 0, 100) : 0, UI_YELLOW);

  drawCard(94, UI_CYAN);
  drawDroplet(16, 103, UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(36, 100, "HUMIDITY", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(36, 114, sensorValid ? String(humidity) + " %" : "ERR", sensorValid ? UI_CYAN : UI_RED);
  drawProgressBar(103, 112, 57, sensorValid ? humidity : 0, UI_CYAN);

  drawCard(144, lightPercent < 25 ? UI_RED : UI_GREEN);
  drawSun(16, 153, lightPercent < 25 ? UI_RED : UI_GREEN);
  tft.setFont(Terminal6x8);
  tft.drawText(36, 150, "LIGHT", UI_GRAY);
  tft.setFont(Terminal12x16);
  tft.drawText(36, 164, String(lightPercent) + " %", lightPercent < 25 ? UI_RED : UI_GREEN);
  drawProgressBar(103, 162, 57, lightPercent, lightPercent < 25 ? UI_RED : UI_GREEN);

  tft.setFont(Terminal6x8);
  tft.drawText(12, 202, sensorValid ? "DHT11 ONLINE" : "DHT11 OFFLINE", sensorValid ? UI_GREEN : UI_RED);
  tft.drawText(112, 202, "GPIO14 / 33", UI_GRAY);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  analogReadResolution(12);

  vspi.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.begin(vspi);
  tft.setOrientation(0);
  tft.setBackgroundColor(UI_BACKGROUND);
  refreshDisplayMemory();
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;
  int error = dht11.read(&temperature, &humidity, NULL);
  bool sensorValid = error == SimpleDHTErrSuccess;
  int lightValue = analogRead(LIGHT_PIN);
  int lightPercent = constrain(map(lightValue, 0, 4095, 0, 100), 0, 100);

  drawDashboard(temperature, humidity, lightPercent, sensorValid);

  Serial.printf("Temp: %u C, Humi: %u %%, Light: %d %%, DHT: %s\n",
                temperature, humidity, lightPercent, sensorValid ? "OK" : "ERROR");
  delay(2000);
}
