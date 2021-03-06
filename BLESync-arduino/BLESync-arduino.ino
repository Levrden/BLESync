/*
BLESync: A BT-LE CONTROLLER FOR TAP-TEMPO GUITAR PEDALS

Will receive a tempo value (encoded as MIDI CCs) and generate tap tempo signals.
This allows to sync delays and other pedals with a tap tempo input
with Ableton live. Uses theAdafuir OLED wing to display tempo/delay time
information.

HARDWARE REQUIREMENTS:
- Adafruit Bluefruit LE Feather
- Adafruit OLED FeatherWing

LIBRARIES:
- Adafruit BluefruitLE nRF51 library: https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
- FortySevenEffects MIDI Arduino Library: https://github.com/FortySevenEffects/arduino_midi_library
- Adafruit SSD1306: https://github.com/adafruit/Adafruit_SSD1306
- Adafruit_GFX: https://github.com/adafruit/Adafruit-GFX-Library

NOTES:
- BLE MIDI does not transmit MIDI clock, so MIDI CC messages are used
  to encode a wide range of tempos (20-999)
- Based on the "Hello World" Bluefruit Feather UNTZtrument MIDI example here:
  https://github.com/adafruit/Adafruit_UNTZtrument
  as well as the Adafruit OLED FeatherWing sample code here:
  https://learn.adafruit.com/adafruit-oled-featherwing?view=all

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

#define CHANNEL 0  // MIDI channel number
#define ENCODER_1   A0
#define ENCODER_2   A1
#define PUSH_BUTTON A2
#define RELAY_OUT   A3
#define OPTO_OUT    13

#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
#define VBATPIN A7

// Number of delay time signatures
#define SIGNATURE_COUNT 3
#define MIN_TEMPO 20
#define MAX_TEMPO 999
#define SOURCE_COUNT 3

bool isConnected = false;
bool externalBPM = false;
unsigned long previousTime = 0;
int tempoIncrement = 0;
int previousIncrement = 0;
int delayTime = 500;
int tempoMSB = 12;
int tempoLSB = 0;
bool tempoMSBreceived = false;
int tempo = 120;
bool newTempo = false; // Do not send tempo at startup
int tapCounter = 3; // Do not send tempo at startup
// Variables for switching internal or external tempo control
// by long push button presses
bool remoteEnabled = true;
bool remoteEnableChanged = false;
bool encoderEnabled = true;
bool encoderEnableChanged = false;

float sources[SOURCE_COUNT] = {0, 1, 2};
String sourceLabels[SIGNATURE_COUNT] = {"i+e", "int", "ext"};
int currentSource = 0;


// Delay time signature:
// 1=quarter note, 0.5=eighth note, 0.75=three sixteenths (aka "The Edge" delay)
float signatures[SIGNATURE_COUNT] = {1, 0.5, 0.75};
String sigLabels[SIGNATURE_COUNT] = {"1/1", "1/2", "3/4"};
int currentSignature = 1;
bool pushBPressed = false;
unsigned long pushBTime = 0;
unsigned long prevPushBTime = 0;
// Encoder variables
int prevCode = 3;
unsigned long previousTimeEncoder = 0;

void setup() {
  pinMode(OPTO_OUT, OUTPUT);
  pinMode(RELAY_OUT, OUTPUT);
  pinMode(ENCODER_1, INPUT);
  pinMode(ENCODER_2, INPUT);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  
  Serial.begin(115200);
  // Wait a bit before initializing the display.
  delay(1000);
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
  ble.println("AT+GAPDEVNAME=BLESync");
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
  update_display();

  midi.setRxCallback(MIDI_in_callback);
}

void loop() {
  ble.update(1);
  unsigned long currentTime = millis();
  
  // Send only 3 taps to avoid delay jitter.
  // The tap counter ir reset every time a new tempo is detected.

  if(newTempo)
  {
    tapCounter = 0;
    delayTime = (60000 * signatures[currentSignature])/tempo;
    newTempo = false;
    update_display();
  }
  
  if (tapCounter<=2)
  {
    if( currentTime - previousTime >= delayTime )
    {
      digitalWrite(OPTO_OUT, HIGH);
      digitalWrite(RELAY_OUT, HIGH);
      previousTime = currentTime;
      delay(20);
      digitalWrite(OPTO_OUT, LOW);
      digitalWrite(RELAY_OUT, LOW);
      tapCounter++;
    }
  }
  
  if(!digitalRead(PUSH_BUTTON))
  {
    if(!pushBPressed)
    {
      pushBPressed = true;
      pushBTime = millis();
      prevPushBTime = millis();
    }
    if( (millis()  - prevPushBTime)  > 1500)
    {
      switch_source();
      prevPushBTime = millis();
    }
  }
  else
  {
    if(pushBTime != 0)
    {
      Serial.print("Time held: ");
      Serial.println(millis() - pushBTime);
      if( (millis() - pushBTime) <= 1500 )
        switch_signature();
    }
    pushBTime = 0;
    pushBPressed = false;
  }

  /*
    Use the encoder to manually adjust tempo.
    The grey code sequence for the encoder I'm using:
    CW  rotation: (bin)...11 -> 00 -> 10 -> 11... = (dec)...3 -> 1 -> 0 -> 3...
    CCW rotation: (bin)...11 -> 10 -> 00 -> 11... = (dec)...3 -> 0 -> 1 -> 3...
    Other sequences are noise and thus rejected.
    Tempo is just updated when the sequence is completed (3 is received)
    This code rejects bounces effectively.
  */
  bool encoder1 = digitalRead(ENCODER_1);
  bool encoder2 = digitalRead(ENCODER_2);
  int code = (encoder1 << 1) + encoder2;
  if(code != prevCode && (currentSource != 2) )
  {
    if(prevCode == 1 && code == 0)
        tempoIncrement = 1;
    else if(prevCode == 0 && code == 1)
        tempoIncrement = -1;
    else if(code == 3)
    {
      externalBPM = false;
      set_tempo(tempo+tempoIncrement);
      // Send new tempo via MIDI?
      //send_tempo(tempo);
      previousTimeEncoder = currentTime;
      previousIncrement = tempoIncrement;
    }
    prevCode = code;
  }

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
  externalBPM = false;
}

// If incoming MIDI message is a controller message (regardless of channel)
// sets the tempo and delayTime to what's incoming
// Uses both controller number and controller value
// to have a big range of tempos (>128)
// TODO: limit to a single channel.

void MIDI_in_callback(uint16_t tstamp, uint8_t status, uint8_t CCnumber, uint8_t CCvalue)
{ 
  if(currentSource != 1)
  {
    if(status >= 176 && status <= 191 && CCnumber <= 99)
    {
      Serial.print("CTRL received: ");
      Serial.println(CCnumber); 
      if(CCnumber == 16)
      {
        tempoMSB = CCvalue;
        tempoMSBreceived = true;   
      }
      else if(CCnumber == 48)
      {
        if(tempoMSBreceived)
        {
          tempoLSB = CCvalue;
          set_tempo(tempoMSB * 10 + tempoLSB);
          externalBPM = true;
          tempoMSBreceived = false;
        }
          else
            Serial.print("No MSB!");
      }
    }
  }
}

// Protects from absurd tempo settings
void set_tempo(int incomingTempo)
{
  if( (incomingTempo < MAX_TEMPO && incomingTempo >= MIN_TEMPO))
  {
    tempo = incomingTempo;
    newTempo = true;
  }
}

// Display initialization code, modified from:
// https://learn.adafruit.com/adafruit-oled-featherwing?view=all

void init_display() {
  Serial.println("OLED test");
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
  display.println("Wait...");
  // Show battery voltage (if much lower than 3.7, batt is almost empty)
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  display.print("VBat: " );
  display.println(measuredvbat);
  display.display();
}

// Updates the display whenever a new tempo is set.
void update_display()
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(tempo);
  if(tempo < 100)
    display.print(" bpm ");
  else
    display.print("bpm ");

  display.println(sourceLabels[currentSource]);

  display.print(sigLabels[currentSignature]);
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

// For debugging purposes:
void send_tempo(int tempo)
{
  int val1 = tempo/10;
  int val2 = tempo%10;
  midi.send(0xB0 | CHANNEL, 17, val1);
  midi.send(0xB0 | CHANNEL, 48, val2);
}

void switch_signature()
{
  if(currentSignature < (SIGNATURE_COUNT-1) )
    currentSignature++;
  else
    currentSignature = 0;
  update_display();
  newTempo = true;
}

void switch_source()
{
  if(currentSource < (SOURCE_COUNT-1) )
    currentSource++;
  else
    currentSource = 0;
  update_display();
  Serial.println("Source changed!");
}
