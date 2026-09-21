#include <WiFi.h>

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

IPAddress local_IP(10, 10, 30, 110);
IPAddress gateway(10, 10, 30, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 10, 30, 100);    //optional
IPAddress secondaryDNS(8, 8, 4, 4);  //optional

WiFiServer Server(8888);

String data=" ";

void setup() {
  //建立SSID連線
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

//啟動TCP Server
Server.begin();
Serial.println("TCP Server !!");
Serial.println("Waiting for client...");

   pinMode(15,OUTPUT);
   pinMode(2,OUTPUT);
   pinMode(4,OUTPUT);
}

void loop() {
   WiFiClient client=Server.available();

   

  if (client){
   Serial.println("Client connected!!") ;
   while(client.connected()){
    //收client資料  
    if (client.available()){
       data=client.readStringUntil('\n');
        Serial.print("Receive:  ");
        Serial.println(data);

        
    } 

  }
   client.stop();
  }
  if(data=="11")
    digitalWrite(15,HIGH);
  else if(data=="10")
    digitalWrite(15,LOW);
  else if(data=="21")
    digitalWrite(2,HIGH);
  else if(data=="20")
    digitalWrite(2,LOW);
  else if(data=="31")
    digitalWrite(4,HIGH);
  else if(data=="30")
    digitalWrite(4,LOW);
  else if(data=="41"){
    digitalWrite(15,HIGH);
    digitalWrite(2,HIGH);
    digitalWrite(4,HIGH);
    delay(300);
    digitalWrite(15,LOW);
    digitalWrite(2,LOW);
    digitalWrite(4,LOW);
    delay(300);
}
  else if(data=="40"){
    digitalWrite(15,LOW);
    digitalWrite(2,LOW);
    digitalWrite(4,LOW);
  }
  else if(data=="51"){
    digitalWrite(15,HIGH);delay(100);digitalWrite(15,LOW);delay(100);
    digitalWrite(2,HIGH);delay(100);digitalWrite(2,LOW);delay(100);
    digitalWrite(4,HIGH);delay(100);digitalWrite(4,LOW);delay(100);
}
  else if(data=="50"){
    digitalWrite(15,LOW);
    digitalWrite(2,LOW);
    digitalWrite(4,LOW);   

}



  else if(data=="41")
    digitalWrite(4,HIGH);
  else if(data=="30")
    digitalWrite(4,LOW);

  







}
