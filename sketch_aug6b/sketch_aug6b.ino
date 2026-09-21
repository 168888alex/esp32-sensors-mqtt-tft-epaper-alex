const int sensorPin = 33; // 光敏電阻AO接腳

void setup() {
  Serial.begin(115200);   // 初始化序列埠，鮑率設定為 115200
  pinMode(sensorPin, INPUT); // 設定GPIO 33為輸入模式
}

void loop() {
  int lightValue = analogRead(sensorPin); // 讀取GPIO 33的類比數值 (0-4095)
  Serial.println(lightValue);             // 顯示在序列視窗
  delay(1000);                            // 延遲1秒 (每秒讀取一次)
}
