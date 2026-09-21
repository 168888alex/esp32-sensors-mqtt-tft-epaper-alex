#include <SPI.h>
#include <TFT_22_ILI9225.h>

const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const uint16_t COLORS[] = {
  0xF800,
  0xFD20,
  0xFFE0,
  0x07E0,
  0x001F,
  0x481F,
  0x780F
};

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_MOSI, TFT_SCK, 0);

void clearAndFill(uint16_t color) {
  tft.fillRectangle(0, 0, tft.maxX() - 1, tft.maxY() - 1, color);
  tft.fillRectangle(0, 0, tft.maxX() - 1, tft.maxY() - 1, color);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  tft.begin();
  tft.setOrientation(0);
  tft.setBackgroundColor(COLOR_BLACK);
  clearAndFill(COLOR_BLACK);

  Serial.println("ILI9225 seven-color software SPI test started");
}

void loop() {
  for (uint8_t index = 0; index < 7; index++) {
    clearAndFill(COLORS[index]);
    Serial.printf("Color index: %u\n", index);
    delay(3000);
  }
}
