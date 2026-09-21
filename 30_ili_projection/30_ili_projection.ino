#include <SPI.h>
#include <TFT_22_ILI9225.h>

const int TFT_RST = 17;
const int TFT_RS = 16;
const int TFT_CS = 5;
const int TFT_SCK = 18;
const int TFT_MOSI = 23;

const uint16_t COLOR_BACKGROUND = 0x0008;
const uint16_t COLOR_GRID = 0x001F;
const uint16_t APP_CYAN = 0x07FF;
const uint16_t APP_BLUE = 0x03FF;
const uint16_t APP_MAGENTA = 0xF81F;
const uint16_t APP_RED = 0xF800;
const uint16_t APP_YELLOW = 0xFFE0;
const uint16_t APP_WHITE = 0xFFFF;

const int SCREEN_WIDTH = 176;
const int SCREEN_HEIGHT = 220;
const int CENTER_X = SCREEN_WIDTH / 2;
const int CENTER_Y = SCREEN_HEIGHT / 2;

SPIClass vspi(VSPI);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, 0);

void drawProjectionFrame(int scanX, int scanY) {
  tft.fillRectangle(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, COLOR_BACKGROUND);

  tft.drawRectangle(2, 2, SCREEN_WIDTH - 3, SCREEN_HEIGHT - 3, APP_BLUE);
  tft.drawRectangle(6, 6, SCREEN_WIDTH - 7, SCREEN_HEIGHT - 7, COLOR_GRID);

  for (int y = 20; y < SCREEN_HEIGHT - 10; y += 20) {
    tft.drawLine(8, y, SCREEN_WIDTH - 9, y, COLOR_GRID);
  }
  for (int x = 20; x < SCREEN_WIDTH - 10; x += 20) {
    tft.drawLine(x, 8, x, SCREEN_HEIGHT - 9, COLOR_GRID);
  }

  tft.drawCircle(CENTER_X, CENTER_Y, 24, APP_BLUE);
  tft.drawCircle(CENTER_X, CENTER_Y, 48, COLOR_GRID);
  tft.drawCircle(CENTER_X, CENTER_Y, 72, COLOR_GRID);
  tft.drawLine(CENTER_X, 8, CENTER_X, SCREEN_HEIGHT - 9, COLOR_GRID);
  tft.drawLine(8, CENTER_Y, SCREEN_WIDTH - 9, CENTER_Y, COLOR_GRID);

  tft.drawLine(CENTER_X, CENTER_Y, scanX, scanY, APP_RED);
  tft.drawLine(scanX, 8, scanX, SCREEN_HEIGHT - 9, APP_MAGENTA);
  tft.drawLine(8, scanY, SCREEN_WIDTH - 9, scanY, APP_CYAN);
  tft.fillCircle(scanX, scanY, 5, APP_YELLOW);
  tft.fillCircle(scanX, scanY, 2, APP_WHITE);

  tft.setFont(Terminal6x8);
  tft.drawText(12, 12, "LASER SCAN", APP_CYAN);
  tft.drawText(12, SCREEN_HEIGHT - 18, "PROJECTOR READY", APP_MAGENTA);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  vspi.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.begin(vspi);
  tft.setOrientation(0);
  tft.setBackgroundColor(COLOR_BACKGROUND);
  tft.clear();

  Serial.println("ILI9225 laser projection scan started");
}

void loop() {
  static int scanX = 8;
  static int scanY = 8;
  static int directionX = 1;
  static int directionY = 1;

  drawProjectionFrame(scanX, scanY);

  scanX += directionX * 4;
  scanY += directionY * 3;
  if (scanX >= SCREEN_WIDTH - 9 || scanX <= 8) {
    directionX = -directionX;
  }
  if (scanY >= SCREEN_HEIGHT - 9 || scanY <= 8) {
    directionY = -directionY;
  }

  delay(70);
}

