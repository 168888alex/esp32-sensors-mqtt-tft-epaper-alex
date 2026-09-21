#include <SPI.h>
#include <TFT_22_ILI9225.h>

const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const uint16_t NEON_COLORS[] = {
  0xF81F,
  0x07FF,
  0xFFE0,
  0x07E0,
  0xF800
};
const uint16_t NEON_DIM_COLORS[] = {
  0x780F,
  0x03EF,
  0x7BE0,
  0x03E0,
  0x7800
};
const char HELLO[] = "HELLO";
const int TEXT_X = 58;
const int TEXT_Y = 100;
const int LETTER_WIDTH = 12;

SPIClass vspi(VSPI);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0);

void drawFlowingNeon() {
  static uint8_t colorOffset = 0;
  static bool flicker = false;

  tft.fillRectangle(48, 88, 130, 124, COLOR_BLACK);
  tft.setFont(Terminal12x16);

  for (int index = 0; index < 5; index++) {
    uint8_t colorIndex = (colorOffset + index) % 5;
    String letter = String(HELLO[index]);
    uint16_t color = flicker ? NEON_DIM_COLORS[colorIndex] : NEON_COLORS[colorIndex];
    tft.drawText(TEXT_X + index * LETTER_WIDTH, TEXT_Y, letter, color);
  }

  colorOffset = (colorOffset + 1) % 5;
  flicker = !flicker;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  vspi.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.begin(vspi);
  tft.setOrientation(0);
  tft.setBackgroundColor(COLOR_BLACK);
  tft.clear();
  drawFlowingNeon();

  Serial.println("ILI9225 flowing neon HELLO test displayed");
}

void loop() {
  drawFlowingNeon();
  delay(180);
}
