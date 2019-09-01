# BLE-MIDI-TapTempo
### A BluetoothLE-MIDI tap tempo controller
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