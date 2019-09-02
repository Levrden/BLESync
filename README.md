# BT tap tempo controller.
Will receive a tempo value (encoded as MIDI CCs) and generate tap tempo signals.
This allows to sync delays and other pedals with a tap tempo input
with Ableton live. Uses theAdafuir OLED wing to display tempo/delay time
information.

Requires a companion Max/MSP patch.

## Requirements

### Hardware Requirements:
- Adafruit Bluefruit LE Feather
- Adafruit OLED FeatherWing

### Libraries:
- Adafruit BluefruitLE nRF51 library: https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
- FortySevenEffects MIDI Arduino Library: https://github.com/FortySevenEffects/arduino_midi_library
- Adafruit SSD1306: https://github.com/adafruit/Adafruit_SSD1306
- Adafruit_GFX: https://github.com/adafruit/Adafruit-GFX-Library

### Other requirements:
- Ableton Live
- Companion Max for Live patch (will upload it soon)

## Notes:
- BLE MIDI does not transmit MIDI clock, so MIDI CC messages are used
  to encode a wide range of tempos (0-999)
- Based on original code by Adafruit Industries, including the "Hello World"
  Bluefruit Feather UNTZtrument MIDI example here:
  https://github.com/adafruit/Adafruit_UNTZtrument
  as well as the Adafruit OLED FeatherWing sample code here:
  https://learn.adafruit.com/adafruit-oled-featherwing?view=all
