#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Pushbutton.h>
#include <Thread.h>
#include <ThreadController.h>

#define OLED_ADDR 0x3c
#define BTN_PIN PC14
#define LED_ONBOARD PC13
#define LED0 PB4
#define LED1 PB3
#define LED2 PA15
#define LED3 PA10

// SDA - PB7, SCL - PB6
Adafruit_SSD1306 display(-1);

// button
Pushbutton button(BTN_PIN);

// Threads ThreadController
ThreadController controll = ThreadController();
Thread LedBlinker = Thread();

void led_blink(){
  static bool ledstatus = false;
  ledstatus = !ledstatus;

  digitalWrite(LED1, ledstatus);
  Serial.println("Blink...");
}

void setup() {
    pinMode(LED_ONBOARD, OUTPUT);
    pinMode(LED0, OUTPUT);
    pinMode(LED1, OUTPUT);

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

    //Test Serial
    Serial.println("Hello world");

    LedBlinker.onRun(led_blink);
    LedBlinker.setInterval(1000);

    controll.add(&LedBlinker);
}

void loop() {
  controll.run();
  if (button.isPressed()){
    digitalWrite(LED0, HIGH);
    Serial.println("Pressed");
    delay(50);
  }
  else{
    digitalWrite(LED0, LOW);
    delay(50);
  }
}
