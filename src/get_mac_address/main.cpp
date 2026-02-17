// RallyRack Receiver MAC Address Discovery Utility
// Flash this to your receiver ESP32 first.
// The device will print its MAC address on startup.

#include <WiFi.h>

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // Initialize WiFi (required for MAC address)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Get and display MAC address
  String mac = WiFi.macAddress();

  Serial.println("\n========================================");
  Serial.println("  RallyRack Receiver MAC Discovery");
  Serial.println("========================================\n");

  Serial.println("Your receiver's MAC address is:\n");
  Serial.print("  Colon format:  ");
  Serial.println(mac);

  // Convert to hex array format for code
  uint8_t baseMac[6];
  WiFi.macAddress(baseMac);

  Serial.print("  Hex array:     {");
  for (int i = 0; i < 6; i++)
  {
    Serial.print("0x");
    if (baseMac[i] < 0x10)
      Serial.print("0");
    Serial.print(baseMac[i], HEX);
    if (i < 5)
      Serial.print(", ");
  }
  Serial.println("}");

  Serial.println("\n========================================");
  Serial.println("Next steps:");
  Serial.println("1. Copy the hex array above");
  Serial.println("2. Open include/rallyrack_config.h");
  Serial.println("3. Find the RECEIVER_MAC line");
  Serial.println("4. Replace {0xFF, ...} with the hex array");
  Serial.println("5. Save and flash all devices");
  Serial.println("========================================\n");

  Serial.println("Press any key to continue...");
}

void loop()
{
  // Wait for serial input
  if (Serial.available() > 0)
  {
    Serial.read();
    Serial.println("Restarting...\n");
    delay(1000);
    setup();
  }

  delay(100);
}
