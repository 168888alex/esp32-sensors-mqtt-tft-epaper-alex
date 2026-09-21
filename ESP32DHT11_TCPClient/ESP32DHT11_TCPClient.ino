/*
     Example of connection using Static IP
     by Evandro Luis Copercini
     Public domain - 2017
*/

#include <Arduino.h>
#include <WiFi.h>
#include <SimpleDHT.h> 

int pin1DHT11 = 16; //ESP32 GPIO16
int pin2DHT11 =17; //ESP32 GPIO16
SimpleDHT11 dht11;

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

IPAddress local_IP(10, 10, 30, 110);
IPAddress gateway(10, 10, 30, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 10, 30, 100);    //optional
IPAddress secondaryDNS(8, 8, 4, 4);  //optional

WiFiClient client;
const char* ip="10.10.30.15";
const uint port=8888;

void setup() {
  Serial.begin(115200);

  WiFi.setMinSecurity(WIFI_AUTH_WEP);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
}
void loop() {
 
  while(!client.connected())
  {
  client.connect(ip,port);
  Serial.print("");
  delay(50);

  }

  if(client.connected())
  {
      byte temperature1= 0;
      byte humidity1 = 0;
      byte temperature2 = 0;
      byte humidity2 = 0;

      int err = SimpleDHTErrSuccess;
      // start working...
      Serial.println("=================================");
      if ((err = dht11.read(pin1DHT11, &temperature1, &humidity1, NULL)) != SimpleDHTErrSuccess) {
        Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
        return;
      }
  if ((err = dht11.read(pin2DHT11, &temperature2, &humidity2, NULL)) != SimpleDHTErrSuccess) {
        Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
        return;
  }

  String sendstr="S1," + String(temperature1) + "," + String(humidity1);
  Serial.println(sendstr);
  client.write((const uint8_t*)sendstr.c_str(),sendstr.length());
  client.flush();

  sendstr="S2," + String(temperature2) + ","+ String(humidity2);
  Serial.println(sendstr);
  client.write((const uint8_t*)sendstr.c_str(),sendstr.length());
  client.flush();

  client.stop();
  }

delay(3000);
}
