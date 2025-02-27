//  \ O /
//   ) (   Betty egg laying mechnism  -  2024 gaijin / Stefan@Recksiegel.de  
//  /   \

#include <WS2812FX.h>
#if defined(ARDUINO_ARCH_ESP8266)
 #include <ESP8266WiFi.h>   // scarfs have ESP M3s 
#elif defined(ARDUINO_ARCH_ESP32)
 #include <WiFi.h>          // but I also use ESP32s
#endif
// #include <WifiEspNowBroadcast.h>
// local copy, mods by SR
#include "WifiEspNowBroadcast.h"
//#include <HashMap.h>

// Pin assignments for ESP8266
#if defined(ARDUINO_ARCH_ESP8266)
 #define LED_PIN  2     // usually 2=D4 on D1 modules, 14 for M3s or 3 for 12V 100LED contoller
 #define SWITCH_PIN 4   // aka flash
 //#define DIM_PIN 4       // Dim the LEDs for darker environments, optional
 #define STATUS_LED 2  // 2 on M3, 16 to use secondary onboard LED on NodeMCU, 2 for D1, 1 for TX pin (onboard LED ESP01)
 #define STATUS_INVERTED true 

// and ESP32
#elif defined(ARDUINO_ARCH_ESP32)  // D1 ESP32 routes different IOs to the pins
 #define LED_PIN 16     // D4=16 for D1 modules, 21 for microphone boards
 #define SWITCH_PIN 17  // D3 routed to IO17 on D1 ESP32
 #define SWITCH_PIN2 32 // secondary switch, 32 on my microphone ESP32 boards
 #define DIM_PIN 4      // Dim the LEDs for darker environments, not yet used on ESP32
 #define STATUS_LED 2   // LED0 on D1 ESP32 is IO2
 #define STATUS_INVERTED true
#endif

#define NUM_LEDS   150  // 60 is 2m, most stolas are shorter, but bikes 60, vest 120 (+1 driver pixel)
#define BRIGHTNESS 100  // usually 200, 30 for testing, 100 for ManRemote, up to 255
#define SPEED     2000  // 2000 smaller numbers are faster! - leave alone to keep things in sync

// My LED scarfs use silicon covered GRB strips
WS2812FX ws2812fx = WS2812FX(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800); 

// 1m=60 LEDs RGBW without Silicon tube - e.g. Burning Man bike strips, Alex' scarf (30/m), ring (60+24)
//WS2812FX ws2812fx = WS2812FX(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);
// 5m=100 LEDs RGB in Silicon tube with ATX12V plug - NB: 400kHz!
//WS2812FX ws2812fx = WS2812FX(101, LED_PIN, NEO_GRB + NEO_KHZ400);
//WS2812FX ws2812fx = WS2812FX(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ400);
// test strips for development, eyes of the ManRemote, fairy lights
//WS2812FX ws2812fx = WS2812FX(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);


/////////////////////////////////////////////////////////////////////////////

// leave this alone, otherwise the scarfs get out of sync
#define NUM_MODES 8 
int effect[NUM_MODES]={43,12,7,10,22,33,46,51};  /* Kitchen LEDs: 1st mode 0=solid */
/* the effects are:
 *  43 Larson Scanner - K.I.T.T.
    12 Rainbow Cycle - Cycles a rainbow over the entire string of LEDs.
     07 Color Wipe Random - Turns all LEDs after each other to a random color. Then starts over with another color.
     10 Multi Dynamic - Lights every LED in a random color. Changes all LED at the same time to new random colors.
    22 Twinkle Fade Random - Blink several LEDs in random colors on, fading out.
    33 Chase Rainbow - White running on rainbow.
    46 Fireworks Random - Random colored firework sparks.
    51 Circus Combustus - Alternating white/red/black pixels running.
*/

#define WIFI_CHANNEL 1
#define SWITCH_DELAY 1000 // only 1 change of mode per second

void sendMessage();
void processRx(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg);

unsigned long gMode = 12;
unsigned long oldMode = 12;

unsigned long lastSentMillis;
unsigned long sendIntervalMillis = 5000;  // 2000
unsigned long lastReceivedMillis = 0;
unsigned long lastReceivedRawMillis = 0;
unsigned long lastPrintedMillis = 0;
unsigned long lastChangeMillis = 0;
unsigned long blockedUntilMillis;
unsigned long IntervalConnect = 7000;
unsigned int  dim = 0;

void setup() {
  if ( STATUS_LED != 1 && LED_PIN != 1 && LED_PIN != 3 ) {   // No serial output if TX pin used for status LED etc
    Serial.begin(115200); Serial.println();
    Serial.printf("Starting SR Egg mechanism, output on GPIO%i\n", LED_PIN);  }
  
// connection indicator LED
pinMode(STATUS_LED, OUTPUT);

pinMode(SWITCH_PIN, INPUT_PULLUP);
#ifdef SWITCH_PIN2
 pinMode(SWITCH_PIN2, INPUT_PULLUP);
#endif
#ifdef DIM_PIN
 if (Serial) { Serial.printf("Dimming switch on GPIO%i\n", DIM_PIN); }
 pinMode(DIM_PIN, INPUT_PULLUP);
#endif

ws2812fx.init();
ws2812fx.setBrightness(BRIGHTNESS);
ws2812fx.setSpeed(SPEED);
ws2812fx.setColor(0xFF0000); // red
//ws2812fx.setColor(0xFF000000); // four bytes for RGBW! - warm white

ws2812fx.setMode(12);   // 14 is two red running pixels, just for starters, similar but not equal to mode 0/43
//ws2812fx.setMode(0);   // solid colour, eg. for kitchen light
ws2812fx.start();


if (Serial) { Serial.println("Setup finished"); }
lastReceivedMillis = 0; 
} // setup


void loop() {
ws2812fx.service();

// Egg motor active?
if( (digitalRead(SWITCH_PIN) == LOW) && (gMode == 12) && (millis() - lastChangeMillis > SWITCH_DELAY) ) {
     gMode=51; ws2812fx.setMode(gMode); ws2812fx.setBrightness(255); lastChangeMillis = millis();
     if (Serial) { Serial.println("Switching to Circus"); } 
}

if( (digitalRead(SWITCH_PIN) == HIGH) && (gMode == 51) && (millis() - lastChangeMillis > SWITCH_DELAY) ) {
     gMode=12; ws2812fx.setMode(gMode); ws2812fx.setBrightness(100); lastChangeMillis = millis();
     if (Serial) { Serial.println("Switching go Rainbow"); } 
}


delay(10);
} // loop
