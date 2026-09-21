#include <SPI.h>
#include <DHT.h>
#include <TFT_22_ILI9225.h>

const int DHT_PIN = 14;
const int DHT_TYPE = DHT11;
const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const uint16_t UI_BACKGROUND = 0x0008;
const uint16_t UI_CARD = 0x10A2;
const uint16_t UI_CYAN = 0x07FF;
const uint16_t UI_YELLOW = 0xFFE0;
const uint16_t UI_RED = 0xF800;
const uint16_t UI_WHITE = 0xFFFF;
const uint16_t UI_GRAY = 0xBDF7;

DHT dht(DHT_PIN, DHT_TYPE);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0);
SPIClass vspi(VSPI);

float temperature = 0;
float humidity = 0;
bool sensorValid = false;
unsigned long lastSensorRead = 0;
unsigned long lastAnimation = 0;
uint8_t animationFrame = 0;

void clearScreen() {
  tft.fillRectangle(0, 0, 175, 219, UI_BACKGROUND);
}

void drawThermometer(int x, int y, uint16_t color, uint8_t frame) {
  int mercuryHeight = 8 + (frame % 6);
  tft.drawCircle(x + 9, y + 25, 8, color);
  tft.fillCircle(x + 9, y + 25, 6, color);
  tft.drawRectangle(x + 5, y + 3, x + 13, y + 25, color);
  tft.fillRectangle(x + 8, y + 7, x + 10, y + 25, UI_CARD);
  tft.fillRectangle(x + 8, y + 25 - mercuryHeight, x + 10, y + 25, color);
}

void drawDroplet(int x, int y, uint16_t color, uint8_t frame) {
  tft.fillCircle(x + 9, y + 20, 8, color);
  tft.fillTriangle(x + 1, y + 20, x + 17, y + 20, x + 9, y + 3, color);
  int highlightY = y + 10 + (frame % 7);
  tft.fillCircle(x + 6, highlightY, 2, UI_WHITE);
}

void drawStaticLayout() {
  clearScreen();
  tft.drawRectangle(3, 3, 172, 216, UI_CYAN);

  tft.setFont(Terminal12x16);
  tft.drawText(30, 12, "DHT11", UI_CYAN);
  tft.setFont(Terminal6x8);
  tft.drawText(48, 31, "ADA SENSOR", UI_GRAY);

  tft.fillRectangle(8, 48, 167, 111, UI_CARD);
  tft.drawText(42, 59, "TEMPERATURE", UI_WHITE);

  tft.fillRectangle(8, 119, 167, 182, UI_CARD);
  tft.drawText(42, 130, "HUMIDITY", UI_WHITE);

  tft.drawText(14, 198, "SENSOR ONLINE", UI_CYAN);
}

void drawSensorValues() {
  tft.fillRectangle(36, 72, 96, 94, UI_CARD);
  tft.setFont(Terminal12x16);
  tft.drawText(42, 76, sensorValid ? String(temperature, 1) + " C" : "ERROR", sensorValid ? UI_YELLOW : UI_RED);

  tft.fillRectangle(36, 143, 96, 165, UI_CARD);
  tft.drawText(42, 147, sensorValid ? String(humidity, 1) + " %" : "ERROR", sensorValid ? UI_CYAN : UI_RED);

  tft.setFont(Terminal6x8);
  tft.fillRectangle(14, 198, 105, 207, UI_BACKGROUND);
  tft.drawText(14, 198, sensorValid ? "SENSOR ONLINE" : "SENSOR ERROR", sensorValid ? UI_CYAN : UI_RED);
}

void drawAnimatedIcons() {
  tft.fillRectangle(10, 50, 36, 91, UI_CARD);
  drawThermometer(14, 55, UI_YELLOW, animationFrame);

  tft.fillRectangle(10, 123, 36, 163, UI_CARD);
  drawDroplet(13, 130, UI_CYAN, animationFrame);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  dht.begin();

  vspi.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.begin(vspi);
  tft.setOrientation(0);
  tft.setBackgroundColor(UI_BACKGROUND);
  drawStaticLayout();
}

void loop() {
  unsigned long now = millis();

  if (lastSensorRead == 0 || now - lastSensorRead >= 10000) {
    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();
    sensorValid = !isnan(newHumidity) && !isnan(newTemperature);
    if (sensorValid) {
      humidity = newHumidity;
      temperature = newTemperature;
    }
    drawSensorValues();
    lastSensorRead = now;
    Serial.printf("Temperature: %.1f C, Humidity: %.1f %%, DHT: %s\n",
                  temperature, humidity, sensorValid ? "OK" : "ERROR");
  }

  if (lastAnimation == 0 || now - lastAnimation >= 500) {
    drawAnimatedIcons();
    animationFrame++;
    lastAnimation = now;
  }
}

