# BLESync: a BT-LE tap tempo controller.
This is a BPM to tap tempo controller with a twist: I can receive BPM values (encoded as MIDI CCs) via a Bluetooth LE connection. This allows to sync delays and other pedals with a tap tempo input with Ableton live (Requires a companion Max/MSP patch).

It can also work standalone. You can use an encoder to manually enter the tempo in BPM.

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
- Companion Max for Live patch (will upload it soon)

## Notes:
- So far the project works on Mac OS (tested on El Capitan). Not tested yet on Windows, but it is supposed to work.
- As per MIDI specs, BLE MIDI does not transmit MIDI clock. This project encodes tempo as MIDI CC messages. It uses both the controller    number and value to allow for a wide range of tempos (20-999 BPM).
- Based on original code by Adafruit Industries, including the "Hello World" Bluefruit Feather UNTZtrument MIDI example here:
  https://github.com/adafruit/Adafruit_UNTZtrument
  as well as the Adafruit OLED FeatherWing sample code here:
  https://learn.adafruit.com/adafruit-oled-featherwing?view=all
