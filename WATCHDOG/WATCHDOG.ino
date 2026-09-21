#include <Arduino.h>
#include "esp_task_wdt.h"
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("WATCHDOG TEST START");

  if (esp_task_wdt_add(NULL) == ESP_OK)
  {
    Serial.println("WATCHDOG ENABLED");
  }
  else
  {
    Serial.println("WATCHDOG ENABLED failed");
  }
}

void loop(){
  Serial.println("Running...");
  //故意不餵狗
  esp_task_wdt_reset();

  delay(1000);
}
