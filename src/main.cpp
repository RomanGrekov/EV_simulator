#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Pushbutton.h>
#include <Thread.h>
#include <ThreadController.h>
#include <timer.h>

#define OLED_ADDR 0x3c
#define BTN_PIN PC14
#define LED_ONBOARD PC13
#define LED0 PB4
#define LED1 PB3
#define LED2 PA15
#define LED3 PA10
#define ADC_PIN PA7

// SDA - PB7, SCL - PB6
Adafruit_SSD1306 display(-1);

// button
Pushbutton button(BTN_PIN);

// Threads ThreadController
ThreadController controll = ThreadController();
Thread LedBlinker = Thread();
Thread BtnHandler = Thread();
Thread ADCHandler = Thread();

// Create timer
//auto timer = timer_create_default();

void led_blink_callback(){
  static bool ledstatus = false;
  ledstatus = !ledstatus;

  digitalWrite(LED1, ledstatus);
}

void btn_callback(){
  static bool btn_state = false;
  static bool btn_state_old = false;

  btn_state = button.isPressed();
  if (btn_state != btn_state_old){
    btn_state_old = btn_state;
    if (btn_state == true){
      digitalWrite(LED0, HIGH);
      digitalWrite(LED3, HIGH);
    }
    else{
      digitalWrite(LED0, LOW);
      digitalWrite(LED3, LOW);
    }
  }
}

void adc_read_callback(){
  static int val = 0;
  static int val_old = 0;
  static int f = 0;
  static int status = 0;
  static int status_old = 0;
  static bool started = false;
  static int start_t = 0;
  static int end_t = 0;

  int min_v = 2;
  int max_v = 3;
  float voltage = 0;

  val = analogRead(ADC_PIN);
  if (val != val_old){
    val_old = val;
    voltage = (float(val) / 4096) * 3.3;

    if (voltage > max_v) status = 1;
    if (voltage < min_v) status = 0;
    if (status != status_old){
      status_old = status;
      if (started != true){
        started = true;
        start_t = millis();
        Serial.print("started");
        Serial.println(start_t);
      }
      else{
        started = false;
        end_t = millis() - start_t;
        Serial.print("Finished");
        Serial.println(end_t);
        f = int ((1.0 / float((end_t / 1000))));
      }
    }
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(10, 20);
    display.print("V: ");
    display.print(voltage);
    display.setCursor(10, 40);
    display.print("F: ");
    display.print(f);
    display.display();
  }
}

void setup() {
    pinMode(LED_ONBOARD, OUTPUT);
    pinMode(LED0, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(LED3, OUTPUT);

    pinMode(ADC_PIN, INPUT_ANALOG);

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
    ADCHandler.onRun(adc_read_callback);
    ADCHandler.setInterval(50);

    controll.add(&LedBlinker);
    controll.add(&BtnHandler);
    controll.add(&ADCHandler);
}

void loop() {
  controll.run();
}
