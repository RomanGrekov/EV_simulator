#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Pushbutton.h>
#include <Thread.h>
#include <ThreadController.h>

#define V_BAT 3.3  // Power supply voltage
#define PROX_NC_V 2.9 // proximity pin voltage when not connected
#define PROX_C_PRESSED 1.8 // proximity pin voltage when not connected, button
                           // pressed
#define PROX_C 0.9 // proximity pin voltage when connected, unpressed

#define NC_V 3.0 // = 12V
#define Con_V 2.28 // = 9V
#define Charge_V 1.5 // = 6V
#define Charge_vent_V 0.7 // = 3V
#define Error_V 0 // = 0V

#define OLED_ADDR 0x3c
#define BTN_PIN PC14
#define LED_ONBOARD PC13
#define REL0 PB4
#define REL1 PB3
#define REL2 PA15
#define LED0 PA10
#define ADC_PILOT_PIN PA0
#define ADC_PROX_PIN PA1
// PA6 - Timer changel 1

// Global vars
float PilotVoltage;
float ProxVoltage;
int T;
int T_h;
int T_l;
int F;
int Duty;
int ChargeCurrent;
enum State {Not_connected,
            Connected,
            Charge,
            Charge_vent_req,
            Error,
            Unknown_error};
enum State cur_state = Not_connected;

const char state_names[6][15] = {"Not_connected",
                                 "Connected",
                                 "Charge",
                                 "Charge_vent",
                                 "Error",
                                 "Unknown_error"};

enum ProxState {NC,
                Connected_btn_pressed,
                Connected_btn_unpressed};
enum ProxState prox_state = NC;
const char prox_state_names[3][20] = {"NC",    // Not connected
                                      "CBP",   // Connected button pressed
                                      "CBUP"}; // Connected button unpressed

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
Thread StateMHandler = Thread();

// Alive led blink
void led_blink_callback(){
  static bool ledstatus = false;
  ledstatus = !ledstatus;
  digitalWrite(LED_ONBOARD, ledstatus);
}

// Simulate ventelate required state
void btn_callback(){
  static bool btn_state = false;
  static bool btn_state_old = false;

  btn_state = button.isPressed();
  if (btn_state != btn_state_old){
    btn_state_old = btn_state;
    if (btn_state == true){
      digitalWrite(REL2, HIGH); // Connect resistor 330 Omh
    }
    else{
      digitalWrite(REL2, LOW); // Connect resistor 330 Omh
    }
  }
}

void adc_read_callback(){
  int val = 0;
  val = analogRead(ADC_PILOT_PIN);
  PilotVoltage = (float(val) / 4096) * V_BAT;
  val = analogRead(ADC_PROX_PIN);
  ProxVoltage = (float(val) / 4096) * V_BAT;

}

void display_show_callback(){
  int height=15;
  int line=2;
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, line);
  display.print(prox_state_names[prox_state]);
  display.print(" ");
  display.print(state_names[cur_state]);
  line = line + height;
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
  display.display();

  Serial.print("Vproxy ");
  Serial.print(ProxVoltage);
  Serial.print("Vpilot ");
  Serial.print(PilotVoltage);
  Serial.print(" F: ");
  Serial.print(F);
  Serial.print(" T_l: ");
  Serial.print(T_l);
  Serial.print(" T_h: ");
  Serial.println(T_h);
}

void state_machine_callback(){
  if (ProxVoltage >= (PROX_NC_V - 0.5)) prox_state = NC;
  if (ProxVoltage >= (PROX_C_PRESSED - 0.5) &&
      ProxVoltage < (PROX_NC_V - 0.5)) prox_state = Connected_btn_pressed;
  if (ProxVoltage >= (PROX_C - 0.5) &&
      ProxVoltage < (PROX_C_PRESSED - 0.5)) prox_state = Connected_btn_unpressed;

  // Process state on pilot pin
  if (prox_state == Connected_btn_unpressed){
    if (PilotVoltage >= (NC_V - 0.5)) cur_state = Not_connected;
    if (PilotVoltage >= (Con_V - 0.5) &&
        PilotVoltage < (NC_V - 0.5) && (990 < F < 1010)) cur_state = Connected;
    if (PilotVoltage >= (Charge_V - 0.5) &&
        PilotVoltage < (Con_V - 0.5) && (990 < F < 1010)) cur_state = Charge;
    if (PilotVoltage >= (Charge_vent_V - 0.5) &&
        PilotVoltage < (Charge_V - 0.5) &&
        (990 < F < 1010)) cur_state = Charge_vent_req;
    if (PilotVoltage < (Charge_vent_V - 0.5)) cur_state = Error;
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
    BtnHandler.onRun(btn_callback);
    BtnHandler.setInterval(10);
    ADCHandler.onRun(adc_read_callback);
    ADCHandler.setInterval(10);
    DispHandler.onRun(display_show_callback);
    DispHandler.setInterval(100);
    StateMHandler.onRun(state_machine_callback);
    StateMHandler.setInterval(100);

    controll.add(&LedBlinker);
    controll.add(&BtnHandler);
    controll.add(&ADCHandler);
    controll.add(&DispHandler);
    controll.add(&StateMHandler);

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
    T_h = Timer3.getCompare(TIMER_CH2);
  }
  if (Timer3.getInputCaptureFlag(TIMER_CH1)){
    T = Timer3.getCompare(TIMER_CH1);
    F = 1000000/T;
    T_l = T - T_h;
    Duty = T_h/(T/100);
    ChargeCurrent = Duty*0.6;
  }
  controll.run();

  switch (prox_state){
    case NC:
      cur_state = Not_connected;
      digitalWrite(REL1, LOW); // Disconnect resistor 1300 Omh
      digitalWrite(REL0, LOW); // Disconnect resistor 2740 Omh
    break;
    case Connected_btn_pressed:
      cur_state = Not_connected;
      digitalWrite(REL1, LOW); // Disconnect resistor 1300 Omh
      digitalWrite(REL0, LOW); // Disconnect resistor 2740 Omh
    break;
    case Connected_btn_unpressed:
      // Start charging
      switch(cur_state){
        case Not_connected:
          digitalWrite(REL0, HIGH); // Connect resistor 2740 Omh
        break;
        case Connected:
          digitalWrite(REL1, HIGH); // Connect resistor 1300 Omh
        break;
        case Charge:
        break;
        case Error:
          digitalWrite(REL1, LOW); // Disconnect resistor 1300 Omh
          digitalWrite(REL0, LOW); // Disconnect resistor 2740 Omh
        break;
      }
    break;
  }
}
