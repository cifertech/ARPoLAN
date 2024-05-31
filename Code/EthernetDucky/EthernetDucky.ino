/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/ARPoLAN
   ________________________________________ */

#include <SPI.h>
#include <Ethernet.h>
#include <HID-Project.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() {
  // Initialize the CS pin
  Ethernet.init(17);  // Assuming you are using pin 10 for CS

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //while (!Serial) {
 //   ; // wait for serial port to connect. Needed for native USB port only
 // }
  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
/*
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
*/
  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // Initialize the USB HID
  Keyboard.begin();
}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    String currentLine = "";
    bool isPost = false;
    bool payloadReceived = false;
    String payload = "";

    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        currentLine += c;
        
        if (c == '\n' && currentLineIsBlank) {
          if (isPost && !payloadReceived) {
            payload = extractPayload(client);
            payload = urlDecode(payload);
            executePayload(payload);
            payloadReceived = true;
          }
          
          // send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
          client.println("<style>");
          client.println("body { font-family: Arial, sans-serif; background-color: #121212; color: #e0e0e0; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }");
          client.println(".container { text-align: center; background-color: #1e1e1e; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.5); max-width: 400px; width: 90%; }");
          client.println("input[type='text'], textarea { width: calc(100% - 22px); padding: 10px; margin: 10px 0; border: 1px solid #333; border-radius: 5px; background-color: #333; color: #e0e0e0; }");
          client.println("input[type='submit'] { padding: 10px 20px; border: none; border-radius: 5px; background-color: #6200ea; color: #ffffff; cursor: pointer; }");
          client.println("input[type='submit']:hover { background-color: #3700b3; }");
          client.println("</style>");
          client.println("</head>");
          client.println("<body>");
          client.println("<div class='container'>");
          client.println("<h1>Ethernet Ducky</h1>");
          client.println("<form method='POST' action='/execute'>");
          client.println("<label for='payload'>Payload:</label><br>");
          client.println("<textarea name='payload' rows='10'></textarea><br>");
          client.println("<input type='submit' value='Execute'>");
          client.println("</form>");
          client.println("</div>");
          client.println("</body>");
          client.println("</html>");
          break;
        }
        
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          if (currentLine.startsWith("POST /execute")) {
            isPost = true;
          }
          currentLine = "";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

String extractPayload(EthernetClient client) {
  String payload = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      payload += c;
    }
  }
  int startIndex = payload.indexOf("payload=") + 8;
  int endIndex = payload.indexOf('&', startIndex);
  if (endIndex == -1) {
    endIndex = payload.length();
  }
  payload = payload.substring(startIndex, endIndex);
  payload.trim();
  return payload;
}

// Function to decode URL encoding
String urlDecode(String input) {
  String decoded = "";
  char temp[] = "00";
  unsigned int len = input.length();
  for (unsigned int i = 0; i < len; i++) {
    if (input[i] == '%') {
      temp[0] = input[i + 1];
      temp[1] = input[i + 2];
      decoded += (char)strtol(temp, NULL, 16);
      i += 2;
    } else if (input[i] == '+') {
      decoded += ' ';
    } else {
      decoded += input[i];
    }
  }
  return decoded;
}

void executePayload(String payload) {
  Serial.print("Executing payload: ");
  Serial.println(payload);

  int length = payload.length();
  for (int i = 0; i < length; i++) {
    char c = payload.charAt(i);

    // Check for special commands
    if (c == '\\') {
      i++;
      if (i < length) {
        String command = "";
        while (i < length && isAlphaNumeric(payload.charAt(i))) {
          command += payload.charAt(i);
          i++;
        }
        i--; // adjust for the loop increment
        if (command == "n") {
          Keyboard.write(KEY_RETURN);
        } else if (command == "t") {
          Keyboard.write(KEY_TAB);
        } else if (command == "b") {
          Keyboard.write(KEY_BACKSPACE);
        } else if (command == "d") {
          Keyboard.write(KEY_DELETE);
        } else if (command == "r") {
          Keyboard.write(KEY_RIGHT_ARROW);
        } else if (command == "l") {
          Keyboard.write(KEY_LEFT_ARROW);
        } else if (command == "u") {
          Keyboard.write(KEY_UP_ARROW);
        } else if (command == "w") {
          Keyboard.write(KEY_DOWN_ARROW);
        } else if (command == "c") {
          Keyboard.press(KEY_LEFT_CTRL);
        } else if (command == "a") {
          Keyboard.press(KEY_LEFT_ALT);
        } else if (command == "s") {
          Keyboard.press(KEY_LEFT_SHIFT);
        } else if (command == "m") {
          Keyboard.press(KEY_LEFT_GUI);
        } else if (command == "f1") {
          Keyboard.write(KEY_F1);
        } else if (command == "f2") {
          Keyboard.write(KEY_F2);
        } else if (command == "f3") {
          Keyboard.write(KEY_F3);
        } else if (command == "f4") {
          Keyboard.write(KEY_F4);
        } else if (command == "f5") {
          Keyboard.write(KEY_F5);
        } else if (command == "f6") {
          Keyboard.write(KEY_F6);
        } else if (command == "f7") {
          Keyboard.write(KEY_F7);
        } else if (command == "f8") {
          Keyboard.write(KEY_F8);
        } else if (command == "f9") {
          Keyboard.write(KEY_F9);
        } else if (command == "f10") {
          Keyboard.write(KEY_F10);
        } else if (command == "f11") {
          Keyboard.write(KEY_F11);
        } else if (command == "f12") {
          Keyboard.write(KEY_F12);
        } else if (command == "h") {
          Keyboard.write(KEY_HOME);
        } else if (command == "e") {
          Keyboard.write(KEY_END);
        } else if (command == "p") {
          Keyboard.write(KEY_PAGE_UP);
        } else if (command == "g") {
          Keyboard.write(KEY_PAGE_DOWN);
        } else if (command == "i") {
          Keyboard.write(KEY_INSERT);
        } else if (command == "k") {
          Keyboard.write(KEY_ESC);
        } else if (command == "x") {
          Keyboard.releaseAll();
        }
      }
    } else {
      Keyboard.print(c);
    }
    delay(100);  // Add a small delay between keystrokes
  }
  Keyboard.releaseAll();
}

bool isAlphaNumeric(char c) {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}
