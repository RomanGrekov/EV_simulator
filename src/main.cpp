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
Thread BtnHandler = Thread();

void led_blink_callback(){
  static bool ledstatus = false;
  ledstatus = !ledstatus;

  digitalWrite(LED1, ledstatus);
  Serial.println("Blink...");
}

void btn_callback(){
  static bool btn_state = false;
  static bool btn_state_old = false;

  btn_state = button.isPressed();
  if (btn_state != btn_state_old){
    btn_state_old = btn_state;
    if (btn_state == true){
      digitalWrite(LED0, HIGH);
      Serial.println("Pressed");
    }
    else{
      digitalWrite(LED0, LOW);
      Serial.println("Unpressed");
    }
  }
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

    LedBlinker.onRun(led_blink_callback);
    LedBlinker.setInterval(1000);
    BtnHandler.onRun(btn_callback);
    BtnHandler.setInterval(10);

    controll.add(&LedBlinker);
    controll.add(&BtnHandler);
}

void loop() {
  controll.run();
}
