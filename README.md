# BLESync: a BT-LE controller for tap-tempo guitar pedals
<img src="https://github.com/jpcarrascal/BLESync/blob/master/hero.png?raw=true" />

BLESync receives BPM values (encoded as MIDI CCs) from a computer via Bluetooth LE. This allows to sync delays and other pedals with a tap tempo input.
As per MIDI specs, BLE MIDI does not transmit MIDI clock, so it is necessary to encode TEMPO as MIDI CC as a MSB/LSB controller combination. The encoding is simple:

	tempoMSB = bpm / 10
	tempoLSB = bpm % 10

Where Both CCnumber and CCvalue are integers. Decoding (which happens in the Arduino code) is also very simple:

    bpm = tempoMSB * 10 + tempoLSB

As per MIDI specs recommendation (https://www.midi.org/), uses CC 16 for tempoMSB and CC 48 for tempoLSB.

Currently tested with Ableton live. A companion Max/MSP patch is included to simplify encoding tempo as MIDI CCs.

It can also work standalone. You can use the built-in encoder to manually enter the tempo in BPM.

Uses the Adafruit OLED wing to display tempo/delay time information.

## Requirements

### Hardware Requirements:
- Adafruit Feather 32u4 Bluefruit LE: https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/overview
- Adafruit OLED FeatherWing: https://learn.adafruit.com/adafruit-oled-featherwing

### Libraries:
- Adafruit BluefruitLE nRF51 library: https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
- FortySevenEffects MIDI Arduino Library: https://github.com/FortySevenEffects/arduino_midi_library
- Adafruit SSD1306: https://github.com/adafruit/Adafruit_SSD1306
- Adafruit_GFX: https://github.com/adafruit/Adafruit-GFX-Library

### Other requirements for synchronizing with Ableton Live:
- Ableton Live
- Max for Live

## Notes:
- So far the project works on Mac OS (tested on El Capitan). Not tested yet on Windows, but it is supposed to work.
- Based on original code by Adafruit Industries, including the "Hello World" Bluefruit Feather UNTZtrument MIDI example here:
  https://github.com/adafruit/Adafruit_UNTZtrument
  as well as the Adafruit OLED FeatherWing sample code here:
  https://learn.adafruit.com/adafruit-oled-featherwing?view=all
