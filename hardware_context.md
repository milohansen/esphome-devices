# Hardware Context for ESPHome Device

## Audio Hardware Config (from hw/audio.yaml)
- **Microphone:** I2S Audio (id: box_mic)
  - Pin: GPIO48 (DIN)
  - Rate: 16000 Hz
  - Bits: 16-bit
  - Channel: Stereo (but we treat as mono for voice)
- **Speaker:** I2S Audio (id: box_speaker)
  - Pin: GPIO9 (DOUT)
  - DAC: ES8311 (via I2C)
  - Rate: 48000 Hz
  - Buffer: 100ms (needs to be minimized for real-time chat)
- **DAC Chip:** ES8311
- **ADC Chip:** ES7210

## Peripherals Config (from hw/peripherals.yaml)
- **I2S Bus:**
  - LRCLK: GPIO10
  - BCLK: GPIO12
  - MCLK: GPIO13
- **I2C Bus A:**
  - SDA: GPIO7
  - SCL: GPIO8
  - Used by: ES8311 (DAC) and ES7210 (ADC)

## Implementation Constraint
The goal is to bypass the standard Home Assistant `voice_assistant` pipeline which is turn-based. We need a "phone call" style full-duplex connection.
- The ESP32 should start streaming UDP audio when the Wake Word (Micro Wake Word) is detected.
- The ESP32 should stop streaming when the user presses a button or after a timeout of silence.

## Implementation Constraint (ESP32-P4 Specific)
- **Processor:** ESP32-P4 (High Performance).
- **Audio Pipeline:**
  - The P4 can easily handle 48kHz 16-bit PCM audio streams.
  - Please configure the `i2s_audio` and UDP stream to run at native **48kHz** to match the hardware DAC/Speaker, reducing the need for poor-quality downsampling.
  - The Python Proxy should handle the resampling from Gemini's 24kHz to the Device's 48kHz to ensure high fidelity.