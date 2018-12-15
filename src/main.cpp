#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define OLED_ADDR 0x3c

// SDA - PB7, SCL - PB6

Adafruit_SSD1306 display(-1);

void setup() {
    // put your setup code here, to run once:
    pinMode(PC13, OUTPUT);

    // Test opto
    pinMode(PB4, OUTPUT);
    pinMode(PB3, OUTPUT);
    pinMode(PA15, OUTPUT);
    pinMode(PA10, OUTPUT);
    // Debug port (using native USB)
    Serial.begin(9600);

    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    display.clearDisplay();
    display.display();

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(27, 30);
    display.print("Hello, world!");
    display.display();
}

void loop() {
//  myOLED.clrScr();                                   // Чистим экран.
//  myOLED.print( "ABCDEFGHIJKLM", LEFT, 24);
  Serial.println("Hello world");
  digitalWrite(PC13, HIGH);
  digitalWrite(PB4, HIGH);
  digitalWrite(PB3, HIGH);
  digitalWrite(PA15, HIGH);
  digitalWrite(PA10, HIGH);
  delay(1000);
  digitalWrite(PC13, LOW);
  digitalWrite(PB4, LOW);
  digitalWrite(PB3, LOW);
  digitalWrite(PA15, LOW);
  digitalWrite(PA10, LOW);
  delay(1000);
}
