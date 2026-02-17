// Flash this to your receiver ESP32 first.
// Open Serial Monitor at 115200 baud and copy the MAC address.
// Paste it into transmitter/config.h

#include <WiFi.h>

void setup()
{
  Serial.begin(115200);
  delay(1000);
  WiFi.mode(WIFI_STA);
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
