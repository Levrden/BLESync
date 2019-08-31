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

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BLEMIDI.h"
#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.7.0"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEMIDI midi(ble);

#define LEDTAP     13 // Pin for heartbeat LEDTAP (shows code is working)
#define CHANNEL 1  // MIDI channel number
#define SWPIN 2

bool isConnected = false;
unsigned long previousTime = 0;
int delayTime = 500;
int tempo = 120;
bool newTempo = true;
int tapCounter = 0;


void setup() {
  pinMode(LEDTAP, OUTPUT);
  pinMode(SWPIN, INPUT);
 
  Serial.begin(115200);
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
  Serial.print(F("Waiting for a connection..."));
  while(!isConnected) {
    ble.update(500);
  }

  midi.setRxCallback(midiInCallback);
  
}

void loop() {
  ble.update(1);
  if(! isConnected) {
    return;
  }

  unsigned long currentTime = millis();
  
  // Send only 3 taps to avoid delay jitter.
  // The tap counter ir reset every time a new tempo is detected.

  if(newTempo)
  {
    tapCounter = 0;
    newTempo = false;
  }
  
  if (tapCounter<=2)
  {
    if( currentTime - previousTime >= delayTime )
    {
      digitalWrite(LEDTAP, HIGH);
      previousTime = currentTime;
      delay(20);
      digitalWrite(LEDTAP, LOW);
      tapCounter++;
    }
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
    delayTime = 60000/tempo;
    Serial.print("Tempo: ");
    Serial.print(tempo);
    Serial.println(" bpm");
    Serial.print("-> Delay: ");
    Serial.print(delayTime);
    Serial.println(" ms");
  }

}
