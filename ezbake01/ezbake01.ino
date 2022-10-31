
#include <Adafruit_MAX31856.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "displaymanager.h"

#define LED_PIN         13
#define MAX31856_CS_PIN 7

// use hardware SPI, just pass in the CS pin
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(MAX31856_CS_PIN);

DisplayManager display;

bool led_state = false;
int tick_it = 0;
int t_sec = 0;

struct ReflowPhase 
{
  enum Direction {Ramp, Hold, Peak, Cool};
  ReflowPhase(String lbl, Direction dir, float T0, float T1, float total_time) 
  : name(lbl), gradient(dir), Tstart(T0), Tend(T1), duration(total_time) 
  {
    
  }
  
  String name;
  Direction gradient;
  float Tstart;
  float Tend;
  float duration;
};


int const n_steps = 5;
int reflow_step = -1;
/*
ReflowPhase reflow_profile[] = {
  ReflowPhase("preheat", ReflowPhase::Ramp, 24, 28, 15),  
  ReflowPhase("soak",    ReflowPhase::Hold, 28, 30, 10),
  ReflowPhase("ramp up", ReflowPhase::Ramp, 30, 32, 15),
  ReflowPhase("reflow",  ReflowPhase::Peak, 32, 33, 5),
  ReflowPhase("cooling", ReflowPhase::Cool, 24, 33, 60)
};
*/

ReflowPhase reflow_profile[] = {
  ReflowPhase("preheat", ReflowPhase::Ramp, 50,  150, 100),  
  ReflowPhase("soak",    ReflowPhase::Hold, 150, 175, 90),
  ReflowPhase("ramp",    ReflowPhase::Ramp, 175, 220, 40),
  ReflowPhase("reflow",  ReflowPhase::Peak, 220, 250, 30),
  ReflowPhase("cooling", ReflowPhase::Cool, 250, 50,  60)
};


void to_standby() 
{
  display.set_title("standby");
  display.reset_clock(6000);
  display.update_T_ranges(reflow_profile[0].Tstart,reflow_profile[0].Tend,reflow_profile[1].Tend);
  display.update_icon(1);  
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("ezbake");

  // initialize digital pin 13 as an output.
  pinMode(LED_PIN, OUTPUT);

  if (!maxthermo.begin()) {
    Serial.println("Could not initialize thermocouple.");
    while (1) delay(10);
  }

  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
  maxthermo.setConversionMode(MAX31856_ONESHOT_NOWAIT);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin("ezbake")) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.show();
  
  delay(4000);
  display.clear_all();
  display.show();
  
  delay(1000); 
  to_standby(); 
  float T = maxthermo.readThermocoupleTemperature();  
  display.update_current_T(int(T));  
  display.show();
}

void loop() {
  digitalWrite(13, led_state);   
  led_state = !led_state;
  
  // trigger a conversion, returns immediately
  maxthermo.triggerOneShot();

  //
  // here's where you can do other things
  //
  delay(500); // replace this with whatever
  tick_it++;
  //

  // check for conversion complete and read temperature
  
  float T = -1.f;
  if (maxthermo.conversionComplete()) {
    T = maxthermo.readThermocoupleTemperature();
    //Serial.println(T);
    bool const in_range = reflow_step < 0
                 || ((reflow_profile[reflow_step].gradient == ReflowPhase::Cool) && (T < reflow_profile[reflow_step].Tstart))
                 || ((T >= reflow_profile[reflow_step].Tstart) && (T <= reflow_profile[reflow_step].Tend));
    display.update_current_T(int(T), in_range);  
  } else {
    display.update_current_T(9000, false);  
    conv_ok = false;
    //Serial.println("Conversion not complete!");
  }

  if (reflow_step >= 0) {
    if (tick_it == 0) {
      tick_it = 1;  
      Serial.print(reflow_step);
      Serial.print(",");
      Serial.print(t_sec);
      Serial.print(",");
      Serial.println(T);
    } else if (tick_it == 1) {
      tick_it = 0;
      display.tick_clock();
      t_sec++;
    } 
  }

  // state machine
  if (T > 0.f) {
    if (((reflow_step == -1) && (T > reflow_profile[0].Tstart)) // start pre-heat
      || (reflow_step == 0 && T > reflow_profile[0].Tend)       // pre heat complete
      || (reflow_step == 1 && display.clock() < 0)              // hold complete
      || (reflow_step == 2 && T > reflow_profile[2].Tend)       // ramp up complete
      || (reflow_step == 3 && display.clock() < 0))             // reflow complete
    {
      reflow_step++;
      display.reset_clock(reflow_profile[reflow_step].duration);
      display.set_title(reflow_profile[reflow_step].name.c_str());
      int const T0 = reflow_profile[reflow_step].Tstart;
      int const T1 = reflow_profile[reflow_step].Tend;
      int const next_T = (reflow_step + 1 < n_steps) ? reflow_profile[reflow_step+1].Tend : 9000;
      display.update_T_ranges(T0, T1, next_T);
      display.update_icon((int)(reflow_profile[reflow_step].gradient));
    } else if (reflow_step == 4 && (display.clock() < 0 || T < reflow_profile[4].Tend)) {        // cooling complete
      reflow_step = -1;
      t_sec = 0;
      to_standby();
    } 
  }

  display.show();        

}
