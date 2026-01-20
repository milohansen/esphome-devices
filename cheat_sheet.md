Gemini Live API Reference:

- Endpoint: wss://generativelanguage.googleapis.com/ws/google.ai.generativelanguage.v1alpha.GenerativeService.BidiGenerateContent?key=YOUR_API_KEY
- Handshake: Send {"setup": {"model": "models/gemini-2.0-flash-exp", "generationConfig": {"responseModalities": ["AUDIO"]}}}
- Input Format: {"realtimeInput": {"mediaChunks": [{"mimeType": "audio/pcm;rate=16000", "data": "<base64_pcm>"}]}}
- Output Format: Server sends serverContent messages containing modelTurn with audio bytes (PCM 24kHz).
- Python Libraries: Use websockets for the connection and scipy.signal.resample (or audioop if available/efficient) to downsample 24kHz -> 16kHz for the ESP32.

Python Audio Processing Notes:
- VAD Library: Suggest using silero-vad (onnx) as it is very efficient and robust against background noise compared to webrtc-vad.
- Algorithm:
    ```py
    if ai_is_speaking and not user_is_shouting(audio_chunk):
        # Drop packet or send silence to prevent echo
        return silence_frame
    else:
        # Pass through to Gemini
        return audio_chunk
    ```