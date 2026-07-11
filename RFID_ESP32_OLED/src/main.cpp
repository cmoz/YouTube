#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_SDA      21
#define OLED_SCL      22

#define RFID_CS       5
#define RFID_RST      2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 rfid(RFID_CS, RFID_RST);

const unsigned char heartBitmap[] PROGMEM = {
  0b00001100, 0b00110000,
  0b00011110, 0b01111000,
  0b00111111, 0b11111100,
  0b01111111, 0b11111110,
  0b01111111, 0b11111110,
  0b01111111, 0b11111110,
  0b00111111, 0b11111100,
  0b00011111, 0b11111000,
  0b00001111, 0b11110000,
  0b00000111, 0b11100000,
  0b00000011, 0b11000000,
  0b00000001, 0b10000000,
  0b00000000, 0b00000000
};

// ❤️ Pulsing Heart Animation
void animateHeart() {
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.drawBitmap(56, 26, heartBitmap, 16, 13, SSD1306_WHITE);
    display.display();
    delay(300);
    display.clearDisplay();
    display.display();
    delay(150);
  }
}

void showSkeletonFrame(bool armsUp) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(50, 10);  // Head
  display.println("  O  ");

  display.setCursor(48, 20);  // Arms
  display.println(armsUp ? " /|\\ " : " \\|/ ");

  display.setCursor(50, 30);  // Torso
  display.println("  |  ");

  display.setCursor(48, 40);  // Legs
  display.println(" / \\ ");

  display.display();
}


// 🕺 Dancing Skeleton Animation
void animateSkeleton() {
  for (int i = 0; i < 4; i++) {
    showSkeletonFrame(i % 2 == 0);
    delay(300);
  }
}


// ✨ Sparkle Burst Animation
void sparkleBurst() {
  for (int i = 0; i < 10; i++) {
    display.clearDisplay();
    for (int j = 0; j < 8; j++) {
      int x = random(0, SCREEN_WIDTH);
      int y = random(0, SCREEN_HEIGHT);
      display.drawPixel(x, y, SSD1306_WHITE);
    }
    display.display();
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  SPI.begin(18, 19, 23); // SCK, MISO, MOSI

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (true);
  }
  display.clearDisplay();

  // RFID
  rfid.PCD_Init();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 30);
  display.println("Scan a tag...");
  display.display();
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  Serial.print("Tag UID: ");
  Serial.println(uid);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 0);
  display.print("UID: ");
  display.println(uid);

  if (uid == "4c49055") {
    animateHeart();
    sparkleBurst();
  } else {
    animateSkeleton();
    sparkleBurst();
  }

  delay(2000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 30);
  display.println("Scan a tag...");
  display.display();
}