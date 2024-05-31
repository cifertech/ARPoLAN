/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/ARPoLAN
   ________________________________________ */

#include <SPI.h>
#include <Ethernet.h>

// MAC address for your controller
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// IP address for your controller
IPAddress ip(192,168,1,0);

void setup() {
  Serial.begin(9600);

    while (!Serial) {
    ; // Wait for serial port to connect. Needed for native USB
  }
  
  Ethernet.init(17);
  

  // Initialize Ethernet
  Ethernet.begin(mac, ip);

  // Give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("Ethernet initialized");

}

void loop() {
    Serial.println("Starting network scan...");

  // Range of IP addresses to scan
  for (int i = 1; i < 255; i++) {
    IPAddress testIp = ip;
    testIp[3] = i;

    delay(10);

    if (ping(testIp)) {
      Serial.print("Device found: ");
      Serial.println(testIp);
    }
    
  }
  Serial.println("Network scan complete.");
}

bool ping(IPAddress ip) {
  EthernetClient client;
  if (client.connect(ip, 80)) {
    client.stop();
    return true;
  }
  return false;
}
