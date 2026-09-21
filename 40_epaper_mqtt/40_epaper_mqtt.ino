#include <Arduino.h>
#include <SPI.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "epd2in9b_V4.h"

#define DHT_PIN   14
#define LIGHT_PIN 33

// Wi-Fi / MQTT settings
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_DATA_TOPIC = "alex/class301/data";
const char *MQTT_GREEN_TOPIC = "alex/class301/ctrl/gled";
const char *MQTT_YELLOW_TOPIC = "alex/class301/ctrl/yled";
const char *MQTT_RED_TOPIC = "alex/class301/ctrl/led4";

// External LED control outputs used by the MQTT commands.
#define GREEN_LED_PIN  15
#define YELLOW_LED_PIN 2
#define RED_LED_PIN    4

const unsigned long SENSOR_INTERVAL_MS = 60000UL;
const unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000UL;

Epd epd;
SimpleDHT11 dht11;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
static uint8_t blackImage[128 * 296 / 8];
static uint8_t redImage[128 * 296 / 8];

unsigned long lastSensorMillis = 0;
unsigned long lastMqttReconnectMillis = 0;

enum Ink { BLACK, RED, WHITE };

const uint8_t g0[] = {0x3E,0x51,0x49,0x45,0x3E}; const uint8_t g1[] = {0x00,0x42,0x7F,0x40,0x00};
const uint8_t g2[] = {0x62,0x51,0x49,0x49,0x46}; const uint8_t g3[] = {0x22,0x49,0x49,0x49,0x36};
const uint8_t g4[] = {0x18,0x14,0x12,0x7F,0x10}; const uint8_t g5[] = {0x2F,0x49,0x49,0x49,0x31};
const uint8_t g6[] = {0x3E,0x49,0x49,0x49,0x32}; const uint8_t g7[] = {0x01,0x71,0x09,0x05,0x03};
const uint8_t g8[] = {0x36,0x49,0x49,0x49,0x36}; const uint8_t g9[] = {0x26,0x49,0x49,0x49,0x3E};
const uint8_t gA[] = {0x7E,0x09,0x09,0x09,0x7E}; const uint8_t gC[] = {0x3E,0x41,0x41,0x41,0x22};
const uint8_t gE[] = {0x7F,0x49,0x49,0x49,0x41}; const uint8_t gG[] = {0x3E,0x41,0x49,0x49,0x7A};
const uint8_t gH[] = {0x7F,0x08,0x08,0x08,0x7F}; const uint8_t gI[] = {0x00,0x41,0x7F,0x41,0x00};
const uint8_t gL[] = {0x7F,0x40,0x40,0x40,0x40}; const uint8_t gM[] = {0x7F,0x02,0x0C,0x02,0x7F};
const uint8_t gP[] = {0x7F,0x09,0x09,0x09,0x06}; const uint8_t gT[] = {0x01,0x01,0x7F,0x01,0x01};
const uint8_t gN[] = {0x7F,0x04,0x08,0x10,0x7F}; const uint8_t gO[] = {0x3E,0x41,0x41,0x41,0x3E};
const uint8_t gR[] = {0x7F,0x09,0x19,0x29,0x46}; const uint8_t gS[] = {0x26,0x49,0x49,0x49,0x32};
const uint8_t gU[] = {0x3F,0x40,0x40,0x40,0x3F}; const uint8_t gPercent[] = {0x63,0x13,0x08,0x64,0x63};
const uint8_t gBang[] = {0x00,0x00,0x5F,0x00,0x00}; const uint8_t gSpace[] = {0,0,0,0,0};

const uint8_t* glyph(char c)
{
  switch (c) {
    case '0': return g0; case '1': return g1; case '2': return g2; case '3': return g3;
    case '4': return g4; case '5': return g5; case '6': return g6; case '7': return g7;
    case '8': return g8; case '9': return g9; case 'A': return gA; case 'C': return gC;
    case 'E': return gE; case 'G': return gG; case 'H': return gH; case 'I': return gI;
    case 'L': return gL; case 'M': return gM; case 'P': return gP; case 'T': return gT;
    case 'N': return gN; case 'O': return gO; case 'R': return gR; case 'S': return gS;
    case 'U': return gU; case '%': return gPercent; case '!': return gBang;
    default: return gSpace;
  }
}

void setPixel(int16_t x, int16_t y, Ink ink)
{
  if (x < 0 || x >= 296 || y < 0 || y >= 128) return;
  uint16_t rawX = y, rawY = 295 - x;
  uint16_t index = rawY * 16 + rawX / 8;
  uint8_t bit = 0x80 >> (rawX % 8);
  if (ink == BLACK) { blackImage[index] &= ~bit; redImage[index] |= bit; }
  else if (ink == RED) { redImage[index] &= ~bit; blackImage[index] |= bit; }
  else { blackImage[index] |= bit; redImage[index] |= bit; }
}

void drawLine(int x0, int y0, int x1, int y1, Ink ink)
{
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1, dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1, err = dx+dy;
  while (true) {
    setPixel(x0,y0,ink); if (x0==x1 && y0==y1) break;
    int e2=2*err; if(e2>=dy){err+=dy;x0+=sx;} if(e2<=dx){err+=dx;y0+=sy;}
  }
}

void fillRect(int x,int y,int w,int h,Ink ink)
{ for(int yy=y;yy<y+h;yy++) for(int xx=x;xx<x+w;xx++) setPixel(xx,yy,ink); }

void fillCircle(int cx,int cy,int r,Ink ink)
{ for(int y=-r;y<=r;y++) for(int x=-r;x<=r;x++) if(x*x+y*y<=r*r)setPixel(cx+x,cy+y,ink); }

void drawCircle(int cx,int cy,int r,Ink ink)
{ for(int a=0;a<360;a+=8){float q=a*0.0174533f;setPixel(cx+(int)(r*cos(q)),cy+(int)(r*sin(q)),ink);} }

void drawChar(int x,int y,char c,uint8_t scale,Ink ink)
{
  const uint8_t* data=glyph(c);
  for(uint8_t col=0;col<5;col++) for(uint8_t row=0;row<7;row++)
    if(data[col]&(1<<row)) fillRect(x+col*scale,y+row*scale,scale,scale,ink);
}

void drawText(int x,int y,const char* text,uint8_t scale,Ink ink)
{ while(*text){drawChar(x,y,*text++,scale,ink);x+=6*scale;} }

void drawTextCentered(int cx,int y,const char* text,uint8_t scale,Ink ink)
{ drawText(cx-(strlen(text)*6*scale-scale)/2,y,text,scale,ink); }

void drawThermometer(int cx,int cy)
{
  drawCircle(cx,cy+13,9,BLACK); fillCircle(cx,cy+13,6,RED); fillRect(cx-4,cy-20,8,35,BLACK); fillRect(cx-2,cy-17,4,29,RED);
  drawLine(cx+8,cy-14,cx+13,cy-14,BLACK); drawLine(cx+8,cy-5,cx+13,cy-5,BLACK);
}

void drawDroplet(int cx,int cy)
{
  fillCircle(cx,cy+9,13,RED); drawLine(cx,cy-22,cx-13,cy+5,BLACK); drawLine(cx,cy-22,cx+13,cy+5,BLACK);
  drawLine(cx-13,cy+5,cx-8,cy+17,BLACK); drawLine(cx+13,cy+5,cx+8,cy+17,BLACK); drawLine(cx-8,cy+17,cx,cy+22,BLACK); drawLine(cx+8,cy+17,cx,cy+22,BLACK);
}

void drawSun(int cx,int cy)
{
  fillCircle(cx,cy,12,RED); drawCircle(cx,cy,13,BLACK);
  for(int a=0;a<360;a+=45){float q=a*0.0174533f;drawLine(cx+(int)(17*cos(q)),cy+(int)(17*sin(q)),cx+(int)(24*cos(q)),cy+(int)(24*sin(q)),BLACK);}
}

void drawDashboard(bool dhtValid,int temp,int humi,bool lightValid,int light)
{
  memset(blackImage,0xFF,sizeof(blackImage)); memset(redImage,0xFF,sizeof(redImage));
  fillRect(0,0,296,20,RED);
  drawTextCentered(148,10,"ESP32 SENSOR",1,WHITE);
  drawLine(8,20,288,20,RED);
  // Reserve the top row for the title, then move the complete dashboard down.
  drawThermometer(49,60); drawDroplet(148,60); drawSun(247,60);
  drawTextCentered(49,84,"TEMP",1,BLACK); drawTextCentered(148,84,"HUMI",1,BLACK); drawTextCentered(247,84,"LIGHT",1,BLACK);
  if(dhtValid){char value[8]; snprintf(value,sizeof(value),"%d",temp); drawTextCentered(42,102,value,3,BLACK); drawChar(67,108,'C',2,RED); snprintf(value,sizeof(value),"%d",humi); drawTextCentered(141,102,value,3,BLACK); drawChar(168,108,'%',2,RED);}
  else {drawTextCentered(49,102,"!",4,RED); drawTextCentered(148,102,"!",4,RED);}
  if(lightValid){char value[8]; snprintf(value,sizeof(value),"%d",light); drawTextCentered(240,102,value,3,BLACK); drawChar(267,108,'%',2,RED);}
  else drawTextCentered(247,102,"!",4,RED);
}

void connectWiFi()
{
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting WiFi: %s\n", WIFI_SSID);
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000UL) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected, IP=%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi connection failed");
  }
}

void setLedFromMqtt(const char *topic, bool turnOn)
{
  int pin = -1;
  if (strcmp(topic, MQTT_GREEN_TOPIC) == 0) pin = GREEN_LED_PIN;
  else if (strcmp(topic, MQTT_YELLOW_TOPIC) == 0) pin = YELLOW_LED_PIN;
  else if (strcmp(topic, MQTT_RED_TOPIC) == 0) pin = RED_LED_PIN;
  if (pin >= 0) {
    digitalWrite(pin, turnOn ? HIGH : LOW);
    Serial.printf("MQTT LED topic=%s state=%s\n", topic, turnOn ? "on" : "off");
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  char message[48];
  unsigned int count = min(length, (unsigned int)(sizeof(message) - 1));
  memcpy(message, payload, count);
  message[count] = '\0';
  String command = String(message);
  command.trim();
  bool turnOn = command == "on" || command.indexOf("\"on\"") >= 0 || command.indexOf("ON") >= 0;
  bool turnOff = command == "off" || command.indexOf("\"off\"") >= 0 || command.indexOf("OFF") >= 0;
  if (turnOn != turnOff) setLedFromMqtt(topic, turnOn);
}

bool connectMqtt()
{
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return mqttClient.connected();
  char clientId[40];
  uint64_t chipId = ESP.getEfuseMac();
  snprintf(clientId, sizeof(clientId), "esp32-epaper-%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  Serial.printf("Connecting MQTT host=%s port=%u client=%s\n", MQTT_HOST, MQTT_PORT, clientId);
  if (!mqttClient.connect(clientId)) {
    Serial.printf("MQTT connection failed, state=%d\n", mqttClient.state());
    return false;
  }
  bool a = mqttClient.subscribe(MQTT_GREEN_TOPIC);
  bool b = mqttClient.subscribe(MQTT_YELLOW_TOPIC);
  bool c = mqttClient.subscribe(MQTT_RED_TOPIC);
  Serial.printf("MQTT connected; subscribe green=%s yellow=%s red=%s\n", a ? "OK" : "FAIL", b ? "OK" : "FAIL", c ? "OK" : "FAIL");
  return true;
}

void publishSensorData(bool dhtValid, int temp, int humi, bool lightValid, int light)
{
  if (!mqttClient.connected()) return;
  char payload[96];
  snprintf(payload, sizeof(payload), "{\"temp\":%d,\"humi\":%d,\"light\":%d,\"dhtValid\":%s,\"lightValid\":%s}",
           dhtValid ? temp : -1, dhtValid ? humi : -1, lightValid ? light : -1,
           dhtValid ? "true" : "false", lightValid ? "true" : "false");
  bool ok = mqttClient.publish(MQTT_DATA_TOPIC, payload);
  Serial.printf("MQTT publish %s: %s\n", ok ? "OK" : "FAIL", payload);
}

void updateDisplay()
{
  byte temp=0,humi=0; int dhtError=dht11.read(DHT_PIN,&temp,&humi,NULL);
  bool dhtValid=dhtError==SimpleDHTErrSuccess && temp<=80 && humi<=100;
  int rawLight=analogRead(LIGHT_PIN); bool lightValid=rawLight>=0 && rawLight<=4095;
  int light=constrain(map(rawLight,0,4095,0,100),0,100);
  Serial.printf("DHT valid=%s temp=%d humi=%d light valid=%s light=%d\n",dhtValid?"yes":"no",temp,humi,lightValid?"yes":"no",light);
  publishSensorData(dhtValid,temp,humi,lightValid,light);
  drawDashboard(dhtValid,temp,humi,lightValid,light); epd.Init(); epd.Display(blackImage,redImage); epd.Sleep();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);
  SPI.begin(18,-1,23,27);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  connectWiFi();
  connectMqtt();
  updateDisplay();
  lastSensorMillis = millis();
}

void loop()
{
  unsigned long now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && now - lastMqttReconnectMillis >= MQTT_RECONNECT_INTERVAL_MS) {
    lastMqttReconnectMillis = now;
    connectMqtt();
  }
  if (mqttClient.connected()) mqttClient.loop();
  if (now - lastSensorMillis >= SENSOR_INTERVAL_MS) {
    lastSensorMillis = now;
    updateDisplay();
  }
  delay(20);
}
