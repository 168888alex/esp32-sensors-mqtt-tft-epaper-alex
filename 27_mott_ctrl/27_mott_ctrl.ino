#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int OLED_SDA = 21;
const int OLED_SCL = 22;
const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int YELLOW_LIGHT_PIN = 4;

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_TOPIC = "alex/class301/data";
const char *GLED_CTRL_TOPIC = "alex/class301/ctrl/gled";
const char *YLED_CTRL_TOPIC = "alex/class301/ctrl/yled";
const char *LED4_CTRL_TOPIC = "alex/class301/ctrl/led4";
const char *CTRL_ROOT_TOPIC = "alex/class301/ctrl";
const char *CTRL_WILDCARD_TOPIC = "alex/class301/ctrl/#";

const unsigned long SENSOR_INTERVAL_MS = 1000;
const unsigned long MQTT_INTERVAL_MS = 10000;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
SimpleDHT11 dht11(DHT_PIN);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastSensorMillis = 0;
unsigned long lastMqttMillis = 0;
byte temperature = 0;
byte humidity = 0;
int lightPercent = 0;
bool sensorValid = false;
bool greenLedOn = false;
bool yellowLedOn = false;
bool yellowLightOn = false;
String commandDisplay = "";
unsigned long commandDisplayUntil = 0;

void setLedState(bool green, bool yellow, bool red) {
  digitalWrite(GREEN_LED_PIN, green ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, yellow ? HIGH : LOW);
  digitalWrite(YELLOW_LIGHT_PIN, red ? HIGH : LOW);
}



void drawDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("TEMP");
  display.setCursor(64, 0);
  display.print("HUMI");
  display.setCursor(0, 36);
  display.print("LIGHT");
  display.setCursor(76, 36);
  display.print("MQTT");

  display.setTextSize(2);
  display.setCursor(0, 12);
  if (sensorValid) {
    display.print(temperature);
    display.print(" C");
  } else {
    display.print("ERR");
  }

  display.setCursor(64, 12);
  if (sensorValid) {
    display.print(humidity);
    display.print("%");
  } else {
    display.print("ERR");
  }

  display.setCursor(0, 48);
  display.print(lightPercent);
  display.print("%");

  display.setCursor(76, 48);
  display.print(mqttClient.connected() ? "OK" : "--");

  if (commandDisplayUntil > millis()) {
    display.fillRect(0, 24, 128, 10, SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.print(commandDisplay);
  }
  display.display();
}

bool jsonStateIsOn(const String &json) {
  int stateIndex = json.indexOf("\"state\"");
  if (stateIndex < 0) return false;
  int colonIndex = json.indexOf(':', stateIndex + 7);
  if (colonIndex < 0) return false;
  int firstQuote = json.indexOf('"', colonIndex + 1);
  if (firstQuote < 0) return false;
  int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return false;
  String value = json.substring(firstQuote + 1, secondQuote);
  value.trim();
  return value.equalsIgnoreCase("on") || value.equalsIgnoreCase("true") || value == "1";
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  char message[128];
  unsigned int copyLength = min(length, (unsigned int)(sizeof(message) - 1));
  memcpy(message, payload, copyLength);
  message[copyLength] = '\0';
  String command(message);
  command.trim();
  bool isOn = jsonStateIsOn(command);
  bool handled = true;

  if (strcmp(topic, GLED_CTRL_TOPIC) == 0) {
    greenLedOn = isOn;
    digitalWrite(GREEN_LED_PIN, greenLedOn ? HIGH : LOW);
    commandDisplay = String("LAMP ") + (greenLedOn ? "ON" : "OFF");
  } else if (strcmp(topic, YLED_CTRL_TOPIC) == 0) {
    yellowLedOn = isOn;
    digitalWrite(YELLOW_LED_PIN, yellowLedOn ? HIGH : LOW);
    commandDisplay = String("FAN ") + (yellowLedOn ? "ON" : "OFF");
  } else if (strcmp(topic, LED4_CTRL_TOPIC) == 0) {
    yellowLightOn = isOn;
    digitalWrite(YELLOW_LIGHT_PIN, yellowLightOn ? HIGH : LOW);
    commandDisplay = String("YLIGHT ") + (yellowLightOn ? "ON" : "OFF");
  } else if (strcmp(topic, CTRL_ROOT_TOPIC) == 0) {
    if (command.indexOf("\"gled\"") >= 0) {
      greenLedOn = jsonStateIsOn(command);
      digitalWrite(GREEN_LED_PIN, greenLedOn ? HIGH : LOW);
      commandDisplay = String("LAMP ") + (greenLedOn ? "ON" : "OFF");
    } else if (command.indexOf("\"yled\"") >= 0) {
      yellowLedOn = jsonStateIsOn(command);
      digitalWrite(YELLOW_LED_PIN, yellowLedOn ? HIGH : LOW);
      commandDisplay = String("FAN ") + (yellowLedOn ? "ON" : "OFF");
    } else if (command.indexOf("\"rled\"") >= 0) {
      yellowLightOn = jsonStateIsOn(command);
      digitalWrite(YELLOW_LIGHT_PIN, yellowLightOn ? HIGH : LOW);
      commandDisplay = String("YLIGHT ") + (yellowLightOn ? "ON" : "OFF");
    } else {
      handled = false;
    }
  } else {
    handled = false;
  }

  if (!handled) commandDisplay = "CTRL ERROR";
  commandDisplayUntil = millis() + 1000;
  Serial.printf("MQTT control topic=%s payload=%s\n", topic, command.c_str());
  drawDisplay();
}
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi connection failed");
  }
}

void connectMqtt() {
  if (mqttClient.connected() || WiFi.status() != WL_CONNECTED) return;
  char clientId[40];
  uint64_t chipId = ESP.getEfuseMac();
  snprintf(clientId, sizeof(clientId), "esp32-%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  Serial.printf("Connecting to MQTT as %s...\n", clientId);  if (mqttClient.connect(clientId)) {
    bool controlSubscribed = mqttClient.subscribe(CTRL_WILDCARD_TOPIC);
    Serial.printf("MQTT subscribe %s: %s\n", CTRL_WILDCARD_TOPIC, controlSubscribed ? "OK" : "FAIL");
  } else {
    Serial.printf("MQTT connection failed, state=%d\n", mqttClient.state());
  }
}

void readSensors() {
  byte nextTemperature = 0;
  byte nextHumidity = 0;
  int error = dht11.read(&nextTemperature, &nextHumidity, NULL);
  int lightRaw = analogRead(LIGHT_PIN);
  lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
  sensorValid = error == SimpleDHTErrSuccess;
  if (sensorValid) {
    temperature = nextTemperature;
    humidity = nextHumidity;
    Serial.printf("Temperature: %d C, Humidity: %d %%, Light: %d %% (raw %d)\n", temperature, humidity, lightPercent, lightRaw);
  } else {
    Serial.printf("DHT11 read failed, error=%d\n", error);
  }

  drawDisplay();
}

void publishData() {
  if (!sensorValid || !mqttClient.connected()) return;
  char payload[64];
  snprintf(payload, sizeof(payload), "{\"temp\":%d,\"humi\":%d,\"light\":%d}", temperature, humidity, lightPercent);
  if (mqttClient.publish(MQTT_TOPIC, payload)) {
    Serial.printf("MQTT published: %s\n", payload);
  } else {
    Serial.println("MQTT publish failed");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(YELLOW_LIGHT_PIN, OUTPUT);
  setLedState(false, false, false);
  pinMode(LIGHT_PIN, INPUT);
  analogReadResolution(12);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED initialization failed");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MQTT SENSOR");
    display.println("Connecting...");
    display.display();
  }

  connectWiFi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(256);
  mqttClient.setCallback(mqttCallback);
  connectMqtt();
  readSensors();
}

void loop() {
  connectWiFi();
  connectMqtt();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastSensorMillis >= SENSOR_INTERVAL_MS || lastSensorMillis == 0) {
    lastSensorMillis = now;
    readSensors();
  }
  if (now - lastMqttMillis >= MQTT_INTERVAL_MS || lastMqttMillis == 0) {
    lastMqttMillis = now;
    publishData();
  }
  delay(10);
}
