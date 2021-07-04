/// @dir radioBlip
/// Send out a radio packet every minute, consuming as little power as possible.
// 2010-08-29 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <Adafruit_NeoPixel.h>
#define RF69_COMPAT 1 // define this to use the RF69 driver i.s.o. RF12

#include <JeeLib.h>

const int node_id = 2;
const int node_group = 210;

#define PIN        4 // On Trinket or Gemma, suggest changing this to 1
const int button = 7;

int brightness = 10;

Adafruit_NeoPixel pixels(1, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

static long counter;
unsigned long lastUpdate = 0;
unsigned long lastLed = 0;
bool ledState = 0;
bool inWater = 0;

const int timeToSleep = 10000;

typedef struct { 
  int brightness; 
} 
PayloadESPLink;
PayloadESPLink esplink;



typedef struct { 
  int switchLevel, counter, battery, brightness ; 
} 
PayloadGLCD;
PayloadGLCD payload;

// this must be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void setup() {

    Serial.begin(115200);
  
    rf12_initialize(node_id, RF12_433MHZ, node_group);
    // see http://tools.jeelabs.org/rfm12b
    rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
    pinMode(button, INPUT);
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    //delay(60000);
}

void parseBrightness() {
  if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {  // and no rf errors
        int node_id = (rf12_hdr & 0x1F);
        //Serial.println(node_id);
        if (node_id = 20 ) {
          esplink = *(PayloadESPLink*) rf12_data;
          brightness = esplink.brightness;
          //Serial.print("New Brightness:");
          //Serial.println(brightness);
        }
      }
}

void loop() {

    //Serial.println(brightness);

    if (rf12_recvDone()) {
      parseBrightness();
    }
    
    bool buttonState = digitalRead(button);
    if (buttonState == true) {
      inWater = 0;
    }
    else {
      inWater = 1;
    }
    ++counter;

    payload.counter = counter;                       // set emonglcd payload
    payload.battery = readVcc();
    payload.switchLevel = inWater;
    //payload.brightness = brightness;

    if (rf12_recvDone()) {
      parseBrightness();
    }

    rf12_sendNow(0, &payload, sizeof payload);
    // set the sync mode to 2 if the fuses are still the Arduino default
    // mode 3 (full powerdown) can only be used with 258 CK startup fuses
    rf12_sendWait(2);

    if (rf12_recvDone()) {
      parseBrightness();
    }

    delay(500);

    if (rf12_recvDone()) {
      parseBrightness();
    }
      


    ledState = 1;
    if (inWater == 1) {
      pixels.setPixelColor(0, pixels.Color(brightness, 0, 0));
    }
    else {
      pixels.setPixelColor(0, pixels.Color(0, brightness, 0));
    }
    pixels.show();

    if (rf12_recvDone()) {
      parseBrightness();
    }
     
    rf12_sleep(RF12_SLEEP);
    //delay(1000);
    Sleepy::loseSomeTime(1000);
    rf12_sleep(RF12_WAKEUP);

    //turn off LED
    pixels.clear(); // Set all pixel colors to 'off'
    pixels.show();
    ledState = 0;

    rf12_sleep(RF12_SLEEP);
    //delay(9000);
    Sleepy::loseSomeTime(8500);
    rf12_sleep(RF12_WAKEUP);

    //if (rf12_recvDone()) {
    //  parseBrightness();
    //}

    
}
