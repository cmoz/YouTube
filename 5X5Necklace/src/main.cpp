#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "font5x5.h"  // Your custom font
Preferences prefs;

#define LED_PIN 8
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

Adafruit_NeoPixel matrix(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define BUTTON_PIN 9  // GPIO0 is the onboard button

// Web server on port 80
WebServer server(80);

// Message buffer
String message = "CMOZ  ";

uint8_t r;
uint8_t g;
uint8_t b;

uint32_t currentColor = matrix.Color(0, 128, 128);  // default color

unsigned long wifiStartTime;
bool wifiTimedOut = false;
bool sleepAnimationDone = false;

const bool heart[5][5] = {{0, 1, 0, 1, 0},
                          {1, 1, 1, 1, 1},
                          {1, 1, 1, 1, 1},
                          {0, 1, 1, 1, 0},
                          {0, 0, 1, 0, 0}};

// Font definitions and getLetterIndex() function are in font5x5.h

void showHeart(uint32_t color, int pulses = 5) {
  for (int p = 0; p < pulses; p++) {
    for (int brightness = 0; brightness <= 10; brightness++) {
      matrix.setBrightness(brightness);
      for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
          int pixel = y * 5 + x;
          matrix.setPixelColor(pixel, heart[y][x] ? color : 0);
        }
      }
      matrix.show();
      delay(100);
    }
    for (int brightness = 10; brightness >= 0; brightness--) {
      matrix.setBrightness(brightness);
      matrix.show();
      delay(100);
    }
  }
  matrix.clear();
  matrix.show();
  matrix.setBrightness(10); // Reset brightness after heart animation
}

void displayLetter(int index, uint32_t color) {
  Serial.println("Displaying letter index: " + String(index) + " with color");
  matrix.clear();
  
  // Add bounds checking
  if (index < 0) {
    Serial.println("ERROR: Negative index!");
    return;
  }
  
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (letters[index][y][x]) {
        int pixel = y * MATRIX_WIDTH + x;
        matrix.setPixelColor(pixel, color);
      }
    }
  }
  matrix.show();
}

void displayMessage(const String &msg) {
  Serial.println("Displaying message: '" + msg + "' (length: " + String(msg.length()) + ")");
  
  // Check if message is empty
  if (msg.length() == 0) {
    Serial.println("Message is empty!");
    return;
  }
  
  for (int i = 0; i < msg.length(); i++) {
    char c = toupper(msg[i]);
    int index = getLetterIndex(c);
    Serial.println("Character '" + String(msg[i]) + "' -> '" + String(c) + "' -> index: " + String(index));
    
    if (index >= 0) {
      displayLetter(index, currentColor);
      delay(700);
    } else {
      Serial.println("Invalid character index: " + String(index));
    }
  }
  Serial.println("Message display complete.");
}

void handleRoot() {
  // HTML form

  const char *htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LED Matrix Input</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #ffe6f0;
      font-family: Arial, sans-serif;
      text-align: center;
      padding: 20px;
    }
    h2 {
      color: #333;
    }
    input[type="text"] {
      width: 80%;
      padding: 12px;
      font-size: 16px;
      border: 1px solid #ccc;
      border-radius: 6px;
      margin-bottom: 10px;
    }
    input[type="submit"] {
      padding: 12px 24px;
      font-size: 16px;
      background-color: #ff80aa;
      color: white;
      border: none;
      border-radius: 6px;
      cursor: pointer;
    }
    input[type="submit"]:hover {
      background-color: #ff6699;
    }
    .slider {
      width: 80%;
      margin: 10px auto;
    }
    #colorPreview {
      width: 100px;
      height: 100px;
      margin: 20px auto;
      border: 2px solid #ccc;
      border-radius: 10px;
      background-color: rgb(0, 128, 128);
    }
  </style>
</head>
<body>
  <h2>Enter Your Message</h2>
  <h5>you can also have numbers, and '!' '.' '-'</h5>

  <form action="/set" method="POST">
    <input type="text" name="msg" maxlength="20" placeholder="Type your message here"><br>

    <label>Red:</label><br>
    <input type="range" name="r" min="0" max="255" value="0" class="slider" oninput="updateColor()"><br>
    <label>Green:</label><br>
    <input type="range" name="g" min="0" max="255" value="128" class="slider" oninput="updateColor()"><br>
    <label>Blue:</label><br>
    <input type="range" name="b" min="0" max="255" value="128" class="slider" oninput="updateColor()"><br>

    <div id="colorPreview"></div>

    <input type="submit" value="Send">
  
  </form>
<br>
  <form action="/shutdown" method="POST">
  <input type="submit" value="Turn Off Wi-Fi">
</form>

  <script>
    function updateColor() {
      const r = document.querySelector('input[name="r"]').value;
      const g = document.querySelector('input[name="g"]').value;
      const b = document.querySelector('input[name="b"]').value;
      document.getElementById('colorPreview').style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
    }
    window.onload = updateColor;
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", htmlForm);
}

void handleSet() {
  if (server.hasArg("msg")) {
    message = server.arg("msg");
    
    Serial.println("Received message from web: '" + message + "'");
    Serial.println("Message length: " + String(message.length()));
    
    // Print each character for debugging
    for (int i = 0; i < message.length(); i++) {
      Serial.println("  Char " + String(i) + ": '" + String(message[i]) + "' (ASCII: " + String((int)message[i]) + ")");
    }

    // Get RGB values
    r = server.hasArg("r") ? server.arg("r").toInt() : 0;
    g = server.hasArg("g") ? server.arg("g").toInt() : 128;
    b = server.hasArg("b") ? server.arg("b").toInt() : 128;
    currentColor = matrix.Color(r, g, b);
    
    Serial.println("Color set to R:" + String(r) + " G:" + String(g) + " B:" + String(b));

    server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Message Sent</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #ffe6f0;
      font-family: Arial, sans-serif;
      text-align: center;
      padding: 20px;
    }
    h2 {
      color: #333;
    }
    a {
      display: inline-block;
      margin-top: 20px;
      padding: 12px 24px;
      font-size: 16px;
      background-color: #ff80aa;
      color: white;
      text-decoration: none;
      border-radius: 6px;
    }
    a:hover {
      background-color: #ff6699;
    }
  </style>
</head>
<body>
  <h2>Message updated!</h2>
  <a href="/">Back to input</a>
</body>
</html>
)rawliteral");
  } else {
    Serial.println("ERROR: No message parameter received!");
    server.send(400, "text/plain", "Missing message");
  }

  prefs.begin("settings", false);  // write mode
  prefs.putString("savedMsg", message);
  prefs.putUChar("r", r);
  prefs.putUChar("g", g);
  prefs.putUChar("b", b);
  prefs.end();

  wifiStartTime = millis();  // Reset timer on message submit
  
  Serial.println("New message saved to preferences: '" + message + "'");
}

void handleShutdown() {
  server.send(200, "text/html",
              "<h2>Wi-Fi turned off</h2><a href='/'>Back</a>");
  WiFi.softAPdisconnect(true);
}

void splashScreen() {
  for (int i = 0; i < NUM_LEDS; i++) {
    matrix.setPixelColor(i, matrix.Color(255, 255, 255));  // White sparkle
    matrix.show();
    delay(50);
    matrix.setPixelColor(i, 0);  // Turn off
  }
  matrix.show();
}

void displayIP() {
  String ipMessage = WiFi.localIP().toString();
  Serial.println("Displaying IP: " + ipMessage);

  for (int i = 0; i < 2; i++) {
    displayMessage(ipMessage);
    displayMessage("     ");  // pause
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  wifiStartTime = millis();

  matrix.begin();
  matrix.setBrightness(10);
  matrix.clear();
  matrix.show();

  // Create Access Point
  const char *apSSID = "SecretMessage";
  const char *apPassword = "tinkertailor";
  WiFi.softAP(apSSID, apPassword);

  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(IP);

  // Show IP on startup
  Serial.println("Showing IP on matrix...");
  displayMessage(IP.toString());  
  delay(2000);

  server.on("/", handleRoot);
  server.on("/set", HTTP_POST, handleSet);
  server.on("/shutdown", HTTP_POST, handleShutdown);
  server.begin();

  showHeart(matrix.Color(255, 0, 0));  // Red heart
  delay(1000);

  prefs.begin("settings", true);  // read-only
  message = prefs.getString("savedMsg", "CMOZ  ");
  r = prefs.getUChar("r", 0);
  g = prefs.getUChar("g", 128);
  b = prefs.getUChar("b", 128);
  prefs.end();

  currentColor = matrix.Color(r, g, b);
  
  Serial.println("Setup complete. Current message: " + message);
}

void loop() {
  server.handleClient();

  // Check button press for IP display
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed - showing IP");
    displayIP();  // Show IP when button is pressed
    delay(1000);  // Debounce
    return; // Skip the rest of the loop iteration
  }

  // Show heart animation
  Serial.println("Showing heart...");
  showHeart(matrix.Color(255, 0, 0), 1);  // Just 1 pulse to make it faster
  delay(1000);
  
  // Display the message
  Serial.println("Displaying current message: " + message);
  displayMessage(message);
  delay(2000);
  
  // Show a space/pause
  displayMessage(" ");
  delay(1000);

  // WiFi timeout check
  if (!wifiTimedOut && millis() - wifiStartTime > 60000) {  // 1 minute
    if (WiFi.softAPgetStationNum() == 0) {
      WiFi.softAPdisconnect(true);
      wifiTimedOut = true;
      Serial.println("Wi-Fi timed out and was shut down.");
    }
  }

  // Sleep animation (only runs once after WiFi timeout)
  if (wifiTimedOut && !sleepAnimationDone) {
    Serial.println("Running sleep animation...");
    // Run fading sleep animation once
    for (int i = 0; i < 3; i++) {  // 3 gentle pulses
      for (int fade = 0; fade <= 255; fade += 5) {
        uint32_t sleepColor =
            matrix.Color(fade / 4, 0, fade);  // soft purple-blue
        for (int j = 0; j < NUM_LEDS; j++) {
          matrix.setPixelColor(j, sleepColor);
        }
        matrix.show();
        delay(10);
      }
      for (int fade = 255; fade >= 0; fade -= 5) {
        uint32_t sleepColor = matrix.Color(fade / 4, 0, fade);
        for (int j = 0; j < NUM_LEDS; j++) {
          matrix.setPixelColor(j, sleepColor);
        }
        matrix.show();
        delay(10);
      }
    }

    matrix.clear();
    matrix.show();
    sleepAnimationDone = true;
    Serial.println("Sleep animation complete.");
  }
}