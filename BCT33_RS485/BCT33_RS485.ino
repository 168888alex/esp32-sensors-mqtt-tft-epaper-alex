#include <SoftwareSerial.h>
#include <WiFi.h>

SoftwareSerial TempSerial(18, 19);

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

IPAddress local_IP(10, 10, 30, 110);
IPAddress gateway(10, 10, 30, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 10, 30, 100);  //optional
IPAddress secondaryDNS(8, 8, 4, 4);     //optional

WiFiClient client;
const char *ip = "10.10.30.15";
const uint port = 8888;

void setup() {
  Serial.begin(9600);
  TempSerial.begin(9600);
  Serial.println("TempSerial is starting....");
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
  byte snd[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xCB };
  byte rec[snd[4] * 256 + snd[5] * 2 + 5];

  for (int i = 0; i < 8; i++)
    TempSerial.write(snd[i]);

  delay(100);

  //Receive RS485 data
  if (TempSerial.available()) {
    TempSerial.readBytes(rec, (snd[4] * 256 + snd[5]) * 2 + 5);

    // for(int i=0; i<(snd[4]*256+snd[5]*2)+5;i++)
    // {
    //     Serial.print(rec[i]);
    //     Serial.print(",");
    // }
    // Serial.println();

  int mid=rec[0];

    float temp = (rec[3] * 256 + rec[4]) / 10.0;
    Serial.print("溫度:");
    Serial.print(temp);
    Serial.println("°C");

    float hum = (rec[5] * 256 + rec[6]) / 10.0;
    Serial.print("濕度:");
    Serial.print(hum);
    Serial.println("%");

    float dew = (rec[7] * 256 + rec[8]) / 10.0;
    Serial.print("露度:");
    Serial.print(dew);
    Serial.println("d°C");

    while (!client.connected()) {
      client.connect(ip, port);
      Serial.print("");
      delay(50);
    }
    Serial.println("Connected to C# Server!!");

    if (client.connected()) {
      String str =String(mid)+","+ String(temp) + "," + String(hum) + "," + String(dew);
      client.write((const uint8_t*)str.c_str(),str.length());
      client.stop();

      Serial.println("Client sends successfully!!");
    }
  }
    delay(3000);
}
