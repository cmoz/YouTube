#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SDA_PIN       21
#define SCL_PIN       22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials
const char* ssid = "YOURNETWORKSSID";
const char* password = "PASSWORD";

// Web server on port 80
WebServer server(80);

// HTML page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>CampClavie Message</title>
  <style>
    body { font-family: sans-serif; text-align: center; margin-top: 50px; }
    input[type="text"] { width: 80%; padding: 10px; font-size: 16px; }
    input[type="submit"] { padding: 10px 20px; font-size: 16px; }
  </style>
</head>
<body>
  <h2>Send a Message to the Screen</h2>
  <form action="/submit" method="get">
    <input type="text" name="msg" maxlength="30" placeholder="Type your message here" />
    <br><br>
    <input type="submit" value="Send" />
  </form>
</body>
</html>
)rawliteral";

// Handle root
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handle message submission
void handleSubmit() {
  if (server.hasArg("msg")) {
    String message = server.arg("msg");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println(message);
    display.display();
    server.send(200, "text/html", "<h2>Message sent!</h2><a href='/'>Back</a>");
  } else {
    server.send(400, "text/plain", "No message received");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Starting Wi-Fi...");
  display.display();

  // Start Wi-Fi access point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Display IP on OLED
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Connect to:");
  display.println(ssid);
  display.println("IP:");
  display.println(IP);
  display.display();

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/submit", handleSubmit);
  server.begin();
}

void loop() {
  server.handleClient();
}
