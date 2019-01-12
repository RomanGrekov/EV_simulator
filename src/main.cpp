#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Pushbutton.h>
#include <Thread.h>
#include <ThreadController.h>

#define V_BAT 3.3  // Power supply voltage

////////////////// Proximity pin raw values /////////////////////////////
#define PROX_NC_V 2.9 // proximity pin voltage when not connected
#define PROX_C_PRESSED 1.8 // proximity pin voltage when not connected, button
                           // pressed
#define PROX_C 0.9 // proximity pin voltage when connected, unpressed
#define prox_v_treshold 0.5 // +- volts
////////////////////////////////////////////////////////////////////////

//////////////// Pilot pin converted values ////////////////////////////
#define SB_V 12.0 // = 12V
#define Con_V 9.0 // = 9V
#define Charge_V 6.0 // = 6V
#define Charge_vent_V 3.0 // = 3V
#define NC_V 0 // = 0V
#define v_treshold 2.0 // +- volts
#define divider_k 4.16 // Voltage divider at ADC
////////////////////////////////////////////////////////////////////////

///////////////////// Pilot pin frequency //////////////////////////////
#define F_NOMINAL 1000
#define F_TRESHOLD 10
////////////////////////////////////////////////////////////////////////

#define OLED_ADDR 0x3c
#define BTN_1_PIN PC13
#define BTN_2_PIN PC14
#define LED_ONBOARD PC13
#define REL0 PA15
#define REL1 PB3
#define REL2 PB4
#define REL3 PA10
#define ADC_PILOT_PIN PA0
#define ADC_PROX_PIN PA1
// PA6 - Timer changel 1

// Global vars
float PilotVoltage;
float ProxVoltage;
volatile int T;
volatile int T_h;
volatile int T_l;
volatile int F;
volatile int Duty;
int ChargeCurrent;
char footer[10] = {""};
enum state {NC,
            Connected_standby,
            Vehicle_detected,
            Charge,
            Charge_vent_req,
            Error};
enum state State = NC;

const char state_names[6][20] = {"Not_connected",
                                 "Connected_standby",
                                 "Vehicle_detected",
                                 "Charge",
                                 "Charge_vent",
                                 "Error"};

enum prox_state {Unplugged, Pressed, Unpressed};
enum prox_state ProxState = Unplugged;
const char prox_state_names[3][20] = {"0", "_", "^"};

bool Btn1_pressed = false;
bool Btn2_pressed = false;

volatile bool need_adc_check = false;
volatile bool need_count_f = false;

// SDA - PB7, SCL - PB6
Adafruit_SSD1306 display(-1);

// button
Pushbutton button1(BTN_1_PIN);
Pushbutton button2(BTN_2_PIN);

// Threads ThreadController
ThreadController controll = ThreadController();
Thread LedBlinker = Thread();
Thread Btn1Handler = Thread();
Thread Btn2Handler = Thread();
Thread ADCPilotHandler = Thread();
Thread ADCProxHandler = Thread();
Thread DispHandler = Thread();
Thread StateMHandler = Thread();

// Clean Frequency variables
void clean_f(void){
  F = 0;
  T = 0;
  T = 0;
  T_h = 0;
  T_l = 0;
  Duty = 0;
}

void timer1_interruption(void){
  need_adc_check = true;
  clean_f();
}

void has_pilot_rise_interruption(void){
  T = Timer3.getCompare(TIMER_CH1);
  need_adc_check = true;
  need_count_f = true;
}

void has_pilot_fall_interruption(void){
  T_h = Timer3.getCompare(TIMER_CH2);
}

bool is_approx_eq(float v1, float v2, float treshold){
  if (v1 >= (v2 - treshold) && v1 <= (v2 + treshold)) return true;
  return false;
}

// Alive led blink
void led_blink_callback(){
  static bool ledstatus = false;
  ledstatus = !ledstatus;
  digitalWrite(LED_ONBOARD, ledstatus);
}

// Simulate ventelate required state
void btn_1_callback(){
  Btn1_pressed = button1.isPressed();
}

// Just for test
void btn_2_callback(){
  static bool btn_state = false;
  static bool btn_state_old = false;

  btn_state = button2.isPressed();
  if (btn_state != btn_state_old){
    btn_state_old = btn_state;
    if (btn_state == true){
      strcpy(footer, "Leaking!");
      digitalWrite(REL3, HIGH); // Disable diode
    }
    else{
      strcpy(footer, "");
      digitalWrite(REL3, LOW); // Enable diode
    }
  }
}

void adc_pilot_read_callback(){
  const float l = 0.1; // 10%
  static float x_old = 1;
  float x = 0;
  float x_t = 0;

  x = float(analogRead(ADC_PILOT_PIN)) / 4096 * V_BAT;
  x_t = l * x + (1 - l) * x_old;
  x_old = x_t;
  PilotVoltage = x_t * divider_k;
}

void adc_prox_read_callback(){
  int val = 0;
  val = analogRead(ADC_PROX_PIN);
  ProxVoltage = (float(val) / 4096) * V_BAT;
}

void display_show_callback(){
  int height=13;
  int line=1;
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, line);
  display.print(prox_state_names[ProxState]);
  display.print(" ");
  display.print(state_names[State]);
  line = line + height + 2;
  display.setCursor(0, line);
  display.print("I: ");
  display.print(ChargeCurrent);
  display.print("A");
  line = line + height;
  display.setCursor(0, line);
  display.print("Vpr: ");
  display.print(ProxVoltage);
  display.print(" Vpil: ");
  display.print(PilotVoltage);
  line = line + height;
  display.setCursor(0, line);
  display.print("F: ");
  display.print(F);
  display.print(" D: ");
  display.print(Duty);
  line = line + height;
  display.setCursor(0, line);
  display.print(footer);
  display.display();


  Serial.print(" Vproxy ");
  Serial.print(ProxVoltage);
  Serial.print(" Vpilot ");
  Serial.print(PilotVoltage);
  Serial.print(" F: ");
  Serial.print(F);
  Serial.print(" T_l: ");
  Serial.print(T_l);
  Serial.print(" T_h: ");
  Serial.println(T_h);
}


prox_state get_prox_state(void){
  if (is_approx_eq(ProxVoltage, PROX_NC_V, prox_v_treshold)) return Unplugged;
  if (is_approx_eq(ProxVoltage, PROX_C_PRESSED, prox_v_treshold)) return Pressed;
  if (is_approx_eq(ProxVoltage, PROX_C, prox_v_treshold)) return Unpressed;
  return Unplugged;
}

state get_pilot_state(void){
  if (is_approx_eq(PilotVoltage, NC_V, v_treshold)) return NC;
  if (is_approx_eq(PilotVoltage, SB_V, v_treshold) && F == 0) return Connected_standby;
  if (is_approx_eq(PilotVoltage, Con_V, v_treshold) && F == 0) return Connected_standby;
  if (is_approx_eq(PilotVoltage, Con_V, v_treshold) && is_approx_eq(F, F_NOMINAL, F_TRESHOLD)) return Vehicle_detected;
  if (is_approx_eq(PilotVoltage, Charge_V, v_treshold) && is_approx_eq(F, F_NOMINAL, F_TRESHOLD)) return Charge;
  if (is_approx_eq(PilotVoltage, Charge_vent_V, v_treshold) && is_approx_eq(F, F_NOMINAL, F_TRESHOLD)) return Charge_vent_req;
  return NC;
}

void state_machine_callback(){
  switch (ProxState) {
    case Unplugged:
      digitalWrite(REL2, LOW); // Disconnect resistor 330 Omh
      digitalWrite(REL1, LOW); // Disconnect resistor 1300 Omh
      digitalWrite(REL0, LOW); // Disconnect resistor 2740 Omh
    break;
    case Pressed:
      digitalWrite(REL2, LOW); // Disconnect resistor 330 Omh
      digitalWrite(REL1, LOW); // Disconnect resistor 1300 Omh
      digitalWrite(REL0, LOW); // Disconnect resistor 2740 Omh
    break;
    case Unpressed:
      switch (State) {
        case Connected_standby:
          digitalWrite(REL0, HIGH); // Connect resistor 2740 Omh
        break;
        case Vehicle_detected:
          digitalWrite(REL1, HIGH); // Connect resistor 1300 Omh
        break;
        case Charge:
          if (Btn1_pressed) digitalWrite(REL2, HIGH); // Connect resistor 330 Omh
          if (! Btn1_pressed) digitalWrite(REL2, LOW); // Disconnect resistor 330 Omh
        break;
        case Charge_vent_req:
          if (! Btn1_pressed) digitalWrite(REL2, LOW); // Disconnect resistor 330 Omh
        break;
        default:
          digitalWrite(REL2, LOW); // Disconnect resistor 330 Omh
          digitalWrite(REL1, LOW); // Disconnect resistor 1300 Omh
          digitalWrite(REL0, LOW); // Disconnect resistor 2740 Omh
        break;
      }
    break;
  }
}


void setup() {
    pinMode(LED_ONBOARD, OUTPUT);
    pinMode(REL0, OUTPUT);
    pinMode(REL1, OUTPUT);
    pinMode(REL2, OUTPUT);

    pinMode(ADC_PILOT_PIN, INPUT_ANALOG);
    pinMode(ADC_PROX_PIN, INPUT_ANALOG);

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
    Btn1Handler.onRun(btn_1_callback);
    Btn1Handler.setInterval(10);
    Btn2Handler.onRun(btn_2_callback);
    Btn2Handler.setInterval(10);
    ADCProxHandler.onRun(adc_prox_read_callback);
    ADCProxHandler.setInterval(11);
    DispHandler.onRun(display_show_callback);
    DispHandler.setInterval(100);
    StateMHandler.onRun(state_machine_callback);
    StateMHandler.setInterval(100);

    controll.add(&LedBlinker);
    controll.add(&Btn1Handler);
    controll.add(&Btn2Handler);
    controll.add(&ADCProxHandler);
    controll.add(&DispHandler);
    controll.add(&StateMHandler);

    Timer3.pause();
    Timer3.setPrescaleFactor(72);
    Timer3.setInputCaptureMode(TIMER_CH1, TIMER_IC_INPUT_DEFAULT);
    Timer3.setInputCaptureMode(TIMER_CH2, TIMER_IC_INPUT_SWITCH);
    Timer3.setPolarity(TIMER_CH2, 1);
    Timer3.setSlaveFlags(TIMER_SMCR_TS_TI1FP1 | TIMER_SMCR_SMS_RESET);
    Timer3.attachInterrupt(TIMER_CH1, has_pilot_rise_interruption);
    Timer3.attachInterrupt(TIMER_CH2, has_pilot_fall_interruption);
    Timer3.refresh();
    Timer3.resume();

    Timer1.pause();
    Timer1.setPeriod(10000);
    Timer1.attachInterrupt(TIMER_BASIC, timer1_interruption);
    Timer1.refresh();
    Timer1.resume();
}

void loop() {
  controll.run();
  ProxState = get_prox_state();
  State = get_pilot_state();

  if (ProxState == Unpressed){
    if (need_count_f){
      need_count_f = false;
      F = 1000000/T;
      T_l = T - T_h;
      Duty = T_h/(T/100);
      ChargeCurrent = Duty*0.6;

      Timer1.refresh();
    }
    if (need_adc_check){
      adc_pilot_read_callback();
      need_adc_check = false;
    }

  }
  else{
    clean_f();
  }

}
