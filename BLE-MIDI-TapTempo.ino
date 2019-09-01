/*
BT tap tempo controller.
Will receive a tempo value (encoded as MIDI CCs) and generate tap tempo signals.
This allows to sync delays and other pedals with a tap tempo input
with Ableton live. Requires the companion Max/MSP patch.

NOTES:
- Based on the "Hello World" Bluefruit Feather UNTZtrument MIDI example here:
  https://github.com/adafruit/Adafruit_UNTZtrument
- BLE MIDI does not transmit MIDI clock, so MIDI CC messages are used
  to encode a wide range of tempos (0-999)

Requires an Adafruit Bluefruit LE Feather, the Adafruit 
BluefruitLE nRF51 library, and a MIDI software synth on your
host computer or mobile device.

Author: JP Carrascal
Based on origianl code by Adafruit Industries (Phil Burgess, Todd Treece)

*/



#include <Wire.h>
#include <SPI.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BLEMIDI.h"
#include "BluefruitConfig.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);


#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.7.0"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEMIDI midi(ble);

#define TAP_TEMPO_OUT  13
#define CHANNEL 1  // MIDI channel number
#define ENCODER_1 11
#define ENCODER_2 12

#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

#define MULTIPLIER_COUNT 3

bool isConnected = false;
bool externalBPM = false;
unsigned long previousTime = 0;
int delayTime = 500;
int tempo = 120;
bool newTempo = true;
int tapCounter = 0;
float multipliers[MULTIPLIER_COUNT] = {1, 0.5, 0.75};
String multitext[MULTIPLIER_COUNT] = {"1/1", "1/5", "3/4"};
int currentMultiplier = 0;
bool buttonCPressed = false;

void setup() {
  pinMode(TAP_TEMPO_OUT, OUTPUT);
  pinMode(ENCODER_1, INPUT);
  pinMode(ENCODER_2, INPUT);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  
  Serial.begin(115200);
  init_display();
  Serial.print(F("Bluefruit Feather: "));

  if ( !ble.begin(VERBOSE_MODE) ) {
    error(F("Couldn't find Bluefruit, check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE ) {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }
  ble.println("AT+GAPDEVNAME=BLEMIDITapTempo");
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();
  
  /* Set BLE callbacks */
  ble.setConnectCallback(connected);
  ble.setDisconnectCallback(disconnected);
  Serial.println(F("Enable MIDI: "));
  
  if ( ! midi.begin(true) ) {
    error(F("Could not enable MIDI"));
  }
    
  ble.verbose(false);
  Serial.println(F("Waiting for a connection..."));
  /*
  while(!isConnected) {
    ble.update(500);
  }
  */
  midi.setRxCallback(midiInCallback);
  
}

void loop() {
  ble.update(1);
  /*
  if(! isConnected) {
    return;
  }
  */
  unsigned long currentTime = millis();
  
  // Send only 3 taps to avoid delay jitter.
  // The tap counter ir reset every time a new tempo is detected.

  if(newTempo)
  {
    tapCounter = 0;
    delayTime = (60000 * multipliers[currentMultiplier])/tempo;
    newTempo = false;
    update_display();
    update_serial();
  }
  
  if (tapCounter<=2)
  {
    if( currentTime - previousTime >= delayTime )
    {
      digitalWrite(TAP_TEMPO_OUT, HIGH);
      previousTime = currentTime;
      delay(20);
      digitalWrite(TAP_TEMPO_OUT, LOW);
      tapCounter++;
    }
  }
  
  if(!digitalRead(BUTTON_C))
  {
    if(!buttonCPressed)
    {
      buttonCPressed = true;
      if(currentMultiplier < (MULTIPLIER_COUNT-1) )
        currentMultiplier++;
      else
        currentMultiplier = 0;
      update_display();
      newTempo = true;
    }
  }
  else
    buttonCPressed = false;
}

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void connected(void) {
  isConnected = true;
  Serial.println(F(" CONNECTED!"));
}

void disconnected(void) {
  Serial.println("disconnected");
  isConnected = false;
}

// If incoming MIDI message is a controller message (regardless of channel)
// sets the tempo and delayTime to what's incoming
// Uses both controller nuber and controller value
// to have a big range of tempos (>128)


void midiInCallback(uint16_t tstamp, uint8_t status, uint8_t data0, uint8_t data1)
{ 
  if(status >= 176 && status <= 191 && data0 <= 99)
  {
    newTempo = true;
    tempo = data0 * 10 + data1;
    externalBPM = true;
  }
}


void init_display() {
  Serial.println("OLED FeatherWing test");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
 
  Serial.println("OLED begun");
 
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);
 
  // Clear the buffer.
  display.clearDisplay();
  display.display();
 
  Serial.println("IO test");
 
  // text display tests
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("120bpm INT");
  display.println("1/1 500 ms");
  display.setCursor(0,0);
  display.display();
}

void update_display()
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(tempo);
  if(tempo < 100)
    display.print(" bpm");
  else
    display.print("bpm");

  if(externalBPM)
    display.println(" EXT");
  else
    display.println(" INT");

  display.print(multitext[currentMultiplier]);
  display.print(" ");
  display.print(delayTime);
  if(delayTime < 100)
    display.println("  ms");
  else if(delayTime > 999)
    display.println("ms");
  else
    display.println(" ms");
  yield();
  display.display();
}

void update_serial()
{
  Serial.print(tempo);
  if(tempo < 100)
    Serial.print(" bpm");
  else
    Serial.print("bpm");

  if(externalBPM)
    Serial.print(" EXT\t");
  else
    Serial.print(" INT\t");

  Serial.print(multitext[currentMultiplier]);
  Serial.print(" ");
  Serial.print(delayTime);
  if(delayTime < 100)
    Serial.println("  ms");
  else if(delayTime > 999)
    Serial.println("ms");
  else
    Serial.println(" ms");
}
