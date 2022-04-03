#include <Arduino.h>
#include <WiFi.h>

byte mac[6];

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("get_MAC_address started...");
    //Serial.print("Program Start...");

    WiFi.begin();
    WiFi.macAddress(mac);

}

void loop(){
  Serial.print("MAC: ");
  Serial.print(mac[0],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.println(mac[5],HEX);
  delay(1000);
}
