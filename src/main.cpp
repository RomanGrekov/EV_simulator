#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Pushbutton.h>
#include <Thread.h>
#include <ThreadController.h>
//#include <timer.h>

#define OLED_ADDR 0x3c
#define BTN_PIN PC14
#define LED_ONBOARD PC13
#define LED0 PB4
#define LED1 PB3
#define LED2 PA15
#define LED3 PA10
#define ADC_PIN PA7

// Global vars
float Voltage;
float T;
float T_h;
float T_l;
float F;

// SDA - PB7, SCL - PB6
Adafruit_SSD1306 display(-1);

// button
Pushbutton button(BTN_PIN);

// Threads ThreadController
ThreadController controll = ThreadController();
Thread LedBlinker = Thread();
Thread BtnHandler = Thread();
Thread ADCHandler = Thread();
Thread DispHandler = Thread();

// Create timer
//auto timer = timer_create_default();

void led_blink_callback(){
  static bool ledstatus = false;
  ledstatus = !ledstatus;

  digitalWrite(LED_ONBOARD, ledstatus);
}

void btn_callback(){
  static bool btn_state = false;
  static bool btn_state_old = false;

  btn_state = button.isPressed();
  if (btn_state != btn_state_old){
    btn_state_old = btn_state;
    if (btn_state == true){
      digitalWrite(LED3, HIGH);
    }
    else{
      digitalWrite(LED3, LOW);
    }
  }
}

void adc_read_callback(){
  int val = 0;
  val = analogRead(ADC_PIN);
  Voltage = (float(val) / 4096) * 3.3;

}

void display_show_callback(){
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 2);
  display.print("V: ");
  display.print(Voltage);
  display.setCursor(0, 17);
  display.print("F: ");
  display.print(F);
  display.setCursor(0, 30);
  display.print("T_h: ");
  display.print(T_h);
  display.setCursor(0, 45);
  display.print("T_l: ");
  display.print(T_l);
  display.display();

  Serial.print("V ");
  Serial.print(Voltage);
  Serial.print(" F: ");
  Serial.print(F);
  Serial.print(" T_l: ");
  Serial.print(T_l);
  Serial.print(" T_h: ");
  Serial.println(T_h);
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
    ADCHandler.setInterval(10);
    DispHandler.onRun(display_show_callback);
    DispHandler.setInterval(100);

    controll.add(&LedBlinker);
    controll.add(&BtnHandler);
    controll.add(&ADCHandler);
    controll.add(&DispHandler);

    Timer3.pause();
    Timer3.setPrescaleFactor(72);
    Timer3.setInputCaptureMode(TIMER_CH1, TIMER_IC_INPUT_DEFAULT);
    Timer3.setInputCaptureMode(TIMER_CH2, TIMER_IC_INPUT_SWITCH);
    Timer3.setPolarity(TIMER_CH2, 1);
    Timer3.setSlaveFlags(TIMER_SMCR_TS_TI1FP1 | TIMER_SMCR_SMS_RESET);
    Timer3.refresh();
    Timer3.resume();
}

void loop() {
  if (Timer3.getInputCaptureFlag(TIMER_CH2)){
    T_l = Timer3.getCompare(TIMER_CH2);
  }
  if (Timer3.getInputCaptureFlag(TIMER_CH1)){
    T = Timer3.getCompare(TIMER_CH1);
    F = 1000000/T;
    T_h = T - T_l;
  }
  controll.run();
}
