// Intro: This is the code used create a website for a controlling the boat that is hosted on a local network. 
// During future tasks, this code will serve as the base and be added upon, this will allow for a command station we can connect to wirelessly on shore

// Task 1
// This code specifically only achieves task 1, "Shut off power to motors via relay from on shore command station"
// What we are essentially doing is developing an estop system that utilizes circuit components like a relay to enable a an emergency stop system. 
// Controlling motors with purely logic like a provided RC signal can be risky and having the capability to make motor movement and thus boat movement 
// Impossible is very valuable

// In simpler terms, this esp32 connects to our router, hosts a webpage, and allows the user to press a button from a computer or phone.
// That button then changes the digital output at the relay pin, which changes whether the relay allows current flow or not.
// So this is not just a webpage project, it is directly tied to a real safety function on the boat.





#include <WiFi.h> // To use the esp32's built in wifi, we include this library

// Now we create char variables to store the network name and password. These are given constant values as this should never change and would break the code if done so
const char* ssid = "ilovemechatronics"; 
const char* password = "Spring26";

// Instantiates a web server on port 80
// Port 80 is the standard port used for normal HTTP webpages
// So when we type the esp32 IP address into a browser, the browser knows to talk to this server
WiFiServer server(80);

// Creates a variable to represent the pin which will send either a HIGH or LOW digital signal to the relay which controls the switch state
const int relayPin = 4;

// We now create a variable which will store the relay state value, either on or off, but default it to off for now as it should never be on unless we say so. Otherwise we risk lossing control 
// during events like a disconnection to server which is very dangerous
// This is also useful because it lets the webpage display the current known state to the user
String relayState = "off";

// This String stores the full incoming HTTP request sent by the client browser
// For example, when a button is pressed, the browser sends text like GET /relay/on HTTP/1.1
// We store all of that text in this variable so we can search through it and figure out what the user is asking the esp32 to do
String header;

// This stores the current time in milliseconds since startup
// millis() is very useful because it lets us time things without needing a real clock
unsigned long currentTime = millis();

// This stores the time at which the current client session began or was last active
// It is used together with currentTime to decide when to stop waiting on a client
unsigned long previousTime = 0;

// This is the max amount of time, in milliseconds, that we will allow a client connection to sit open without finishing its request
// 2000 ms = 2 seconds
// This helps prevent the esp32 from hanging forever if a browser connects badly or stops responding halfway through
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200); // Declare baudrate of serial monitor

  delay(2000); // Gives the esp32 a short moment to fully boot before the rest of the code begins
               // This also gives the serial monitor time to open and begin displaying text cleanly

  Serial.println();
  Serial.println("ESP32 booted."); // Indicates esp32 on and serial monitor is working if seen

  pinMode(relayPin, OUTPUT); // Sets our earlier variable as an output which will allow us to send digital signals out

  // Start relay OFF
  digitalWrite(relayPin, LOW); // When the relay is given a LOW voltage (OV), the COM and NO ports on the other side create an open circuit, preventing flow to motors
                               // This is a safer startup condition because the system should default to not allowing motion until commanded

  Serial.print("Connecting to "); 
  Serial.println(ssid); 

  // This tells the esp32 to begin connecting to the router using the SSID and password provided above
  WiFi.begin(ssid, password);

  // This loop keeps checking connection status until the esp32 successfully joins the wifi network
  // While it is trying, we print dots so the user can visually see that the board is still working on connecting
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected."); // Indicates to user a successful connection

  Serial.print("IP address: "); // Gives the user the IP Address of the esp32 which was assigned by the router. This is very important as it allows the user to visit the esp32s webpage and control the boat
  Serial.println(WiFi.localIP());

  // This starts the web server so the esp32 begins listening for incoming browser connections
  server.begin();
}

void loop() {
  // This checks if a browser/client is currently trying to connect to the esp32 server
  // If nobody is connecting, client will basically be empty and the loop just continues
  WiFiClient client = server.available();

  // If a client exists, that means something like a browser on a laptop or phone has connected
  if (client) {
    currentTime = millis();
    previousTime = currentTime;

    Serial.println("New Client.");

    // This stores one line of the incoming HTTP request at a time
    // HTTP requests are line based, so this helps us detect blank lines and know when the request header is finished
    String currentLine = "";

    // Stay in this loop while:
    // 1. the client is still connected
    // 2. the timeout has not been exceeded
    // This prevents the esp32 from waiting forever on a bad or incomplete request
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();

      // If there is actually data available from the client, read it one character at a time
      if (client.available()) {
        char c = client.read();

        Serial.write(c); // Writes each character to the serial monitor so we can literally watch the raw browser request come in
        header += c;     // Add that character to the full request String so we can later search it for commands

        // In HTTP, a newline means the current line ended
        if (c == '\n') {

          // If currentLine is empty, that means we just hit a blank line
          // In HTTP, a blank line means the request headers are done
          // That is our signal that it is now time to respond to the browser
          if (currentLine.length() == 0) {

            // Handle relay commands
            // If the request contains GET /relay/on, that means the user clicked the button to turn relay on
            if (header.indexOf("GET /relay/on") >= 0) {
              Serial.println("Relay ON");
              relayState = "on";
              digitalWrite(relayPin, HIGH); // Sends HIGH to relay pin, changing relay state
            } 

            // If the request contains GET /relay/off, that means the user clicked the button to turn relay off
            else if (header.indexOf("GET /relay/off") >= 0) {
              Serial.println("Relay OFF");
              relayState = "off";
              digitalWrite(relayPin, LOW); // Sends LOW to relay pin, changing relay state
            }

            // HTTP response
            // The esp32 must now send a valid HTTP response back to the browser
            // First line says status 200 OK, meaning the request was handled successfully
            client.println("HTTP/1.1 200 OK");

            // This tells the browser the content being sent is HTML
            client.println("Content-type:text/html");

            // This tells the browser the server will close the connection after sending the page
            client.println("Connection: close");

            // Blank line separates HTTP headers from actual webpage content
            client.println();

            // Webpage
            // Everything below here is the actual HTML/CSS code the browser will render
            client.println("<!DOCTYPE html><html>");
            client.println("<head>");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");

            client.println("<style>");
            client.println("html { font-family: Helvetica; text-align: center; margin-top: 50px; }");

            // This creates a CSS class called button that controls the size, color, and appearance of the button
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 10px; cursor: pointer; border-radius: 8px; }");

            // This creates a second button style for the OFF state so it visually looks different
            client.println(".buttonOff { background-color: #555555; }");
            client.println("</style>");

            client.println("<title>On Shore Control Station</title>");
            client.println("</head>");

            client.println("<body>");
            client.println("<h1>On Shore Control Station</h1>");

            // This line displays the current relay state to the user directly on the webpage
            client.println("<p>Relay State: " + relayState + "</p>");

            // If relay is currently off, show a button that sends the browser to /relay/on
            // That new path is what the esp32 later detects using header.indexOf(...)
            if (relayState == "off") {
              client.println("<p><a href=\"/relay/on\"><button class=\"button\">Relay ON</button></a></p>");
            } 

            // Otherwise if relay is already on, show a button that sends the browser to /relay/off
            else {
              client.println("<p><a href=\"/relay/off\"><button class=\"button buttonOff\">Relay OFF</button></a></p>");
            }

            client.println("</body></html>");

            // Final blank line to finish response cleanly
            client.println();

            // Break out once page has been sent
            break;
          } 
          else {
            // If the line was not blank, clear currentLine so it can begin storing the next line of the HTTP request
            currentLine = "";
          }
        } 
        else if (c != '\r') {
          // If the character is not a carriage return, add it to the current line being built
          // We ignore '\r' because HTTP lines often end in \r\n and the newline handling is enough for what we are doing
          currentLine += c;
        }
      }
    }

    // Clear the stored request so old request text does not remain the next time a client connects
    header = "";

    // Close the connection to the client
    client.stop();

    Serial.println("Client disconnected.");
    Serial.println();
  }
}