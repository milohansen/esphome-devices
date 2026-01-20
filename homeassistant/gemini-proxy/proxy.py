import asyncio
import os
import socket
import logging
import signal
import numpy as np
from scipy import signal as scipy_signal
from google import genai
import onnxruntime
from collections import deque

# Configuration
UDP_IP = "0.0.0.0"
UDP_PORT = 7000
ESP_RESPONSE_PORT = 7001

GEMINI_MODEL = "gemini-2.5-flash-native-audio-preview-12-2025"
GEMINI_API_KEY = os.environ.get("GEMINI_API_KEY")

# Audio Config
ESP_INPUT_RATE = 48000  # ESP32 P4 Native
GEMINI_INPUT_RATE = 16000
GEMINI_OUTPUT_RATE = 24000
ESP_OUTPUT_RATE = 48000

# VAD Config
VAD_MODEL_PATH = "silero_vad.onnx"
VAD_THRESHOLD = 0.5
SILENCE_CHUNKS_TO_STOP = 10 # Approx 300-500ms

# Logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class VADWrapper:
    def __init__(self):
        # Download model if not exists
        if not os.path.exists(VAD_MODEL_PATH):
            logger.info("Downloading Silero VAD model...")
            import urllib.request
            urllib.request.urlretrieve("https://github.com/snakers4/silero-vad/raw/master/files/silero_vad.onnx", VAD_MODEL_PATH)
        
        self.session = onnxruntime.InferenceSession(VAD_MODEL_PATH)
        self.reset_states()
        
    def reset_states(self):
        self._h = np.zeros((2, 1, 64), dtype=np.float32)
        self._c = np.zeros((2, 1, 64), dtype=np.float32)

    def is_speech(self, audio_chunk_16k):
        # Audio must be 16kHz, float32, range [-1, 1]
        # audio_chunk_16k is expected to be bytes int16
        
        audio_int16 = np.frombuffer(audio_chunk_16k, dtype=np.int16)
        audio_float32 = audio_int16.astype(np.float32) / 32768.0
        
        # Add batch dimension: (Batch, Time) -> (1, N)
        if len(audio_float32) < 32: # Too small
             return 0.0
             
        # Silero expects specific chunk sizes usually (512, 1024, 1536) for valid context logic 
        # but supports others if we manage state carefully. 
        # For simplicity, we just pass what we get, assuming chunks are reasonable (~30-50ms).
        # 16000Hz * 0.032s = 512 samples.
        
        input_data = {
            "input": audio_float32[np.newaxis, :],
            "sr": np.array([16000], dtype=np.int64),
            "h": self._h,
            "c": self._c
        }
        
        out, h, c = self.session.run(None, input_data)
        self._h, self._c = h, c
        
        return out[0][0]

class AudioProxy:
    def __init__(self):
        self.udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_sock.bind((UDP_IP, UDP_PORT))
        self.udp_sock.setblocking(False)
        
        self.client = genai.Client(api_key=GEMINI_API_KEY)
        self.esp_address = None
        self.running = True
        
        self.audio_queue_mic = asyncio.Queue()
        self.audio_queue_speaker = asyncio.Queue()
        
        self.vad = VADWrapper()
        
        # State
        self.ai_is_speaking = False
        
        logger.info(f"Listening on UDP {UDP_IP}:{UDP_PORT}")

    def resample_audio(self, audio_data, src_rate, dst_rate):
        if src_rate == dst_rate:
            return audio_data
        
        audio_np = np.frombuffer(audio_data, dtype=np.int16)
        num_samples = int(len(audio_np) * dst_rate / src_rate)
        resampled_np = scipy_signal.resample(audio_np, num_samples)
        return resampled_np.astype(np.int16).tobytes()

    async def udp_listener_task(self):
        loop = asyncio.get_running_loop()
        while self.running:
            try:
                data, addr = await loop.sock_recvfrom(self.udp_sock, 4096)
                if not self.esp_address:
                    logger.info(f"Connection from ESP32 at {addr}")
                    self.esp_address = addr
                elif self.esp_address[0] != addr[0]:
                     self.esp_address = addr

                # Data is 48kHz. Resample to 16kHz for VAD/Gemini
                audio_16k = await asyncio.to_thread(self.resample_audio, data, ESP_INPUT_RATE, GEMINI_INPUT_RATE)
                
                # Check VAD
                # We need to process in chunks that Silero likes? 
                # Or just process what we have. Silero is robust.
                prob = await asyncio.to_thread(self.vad.is_speech, audio_16k)
                
                # Gating Logic
                if self.ai_is_speaking:
                    if prob > 0.8: # Strong speech detected (Barge-in)
                        logger.debug("Barge-in detected!")
                        await self.audio_queue_mic.put(audio_16k)
                    else:
                        # Drop packet (Silence) to prevent echo loops
                        pass
                else:
                    await self.audio_queue_mic.put(audio_16k)
                
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"UDP Receive Error: {e}")
                await asyncio.sleep(0.1)

    async def udp_sender_task(self):
        loop = asyncio.get_running_loop()
        while self.running:
            try:
                chunk = await self.audio_queue_speaker.get()
                
                if self.esp_address:
                    target_port = ESP_RESPONSE_PORT if ESP_RESPONSE_PORT else self.esp_address[1]
                    target_addr = (self.esp_address[0], target_port)
                    
                    max_size = 1024
                    for i in range(0, len(chunk), max_size):
                        sub_chunk = chunk[i:i+max_size]
                        await loop.sock_sendto(self.udp_sock, sub_chunk, target_addr)
                
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"UDP Send Error: {e}")

    async def gemini_sender_task(self, session):
        while self.running:
            chunk = await self.audio_queue_mic.get()
            await session.send_realtime_input(audio={"data": chunk, "mime_type": f"audio/pcm;rate={GEMINI_INPUT_RATE}"})

    async def gemini_receiver_task(self, session):
        while self.running:
            try:
                async for response in session.receive():
                    if (response.server_content and response.server_content.model_turn):
                       for part in response.server_content.model_turn.parts:
                           if part.inline_data and isinstance(part.inline_data.data, bytes):
                               # AI is speaking
                               self.ai_is_speaking = True
                               
                               audio_24k = part.inline_data.data
                               
                               # Upsample to 48kHz for ESP32
                               audio_48k = await asyncio.to_thread(
                                   self.resample_audio, 
                                   audio_24k, 
                                   GEMINI_OUTPUT_RATE, 
                                   ESP_OUTPUT_RATE
                               )
                               
                               await self.audio_queue_speaker.put(audio_48k)
                    
                    if response.server_content and response.server_content.turn_complete:
                         self.ai_is_speaking = False

            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"Gemini Receive Error: {e}")
                self.ai_is_speaking = False
                await asyncio.sleep(1)

    async def run(self):
        config = {
            "response_modalities": ["AUDIO"],
            "system_instruction": "You are a helpful and friendly AI assistant. Be concise.",
        }
        
        logger.info("Connecting to Gemini Live...")
        try:
            async with self.client.aio.live.connect(model=GEMINI_MODEL, config=config) as session:
                logger.info("Connected to Gemini!")
                
                async with asyncio.TaskGroup() as tg:
                    tg.create_task(self.udp_listener_task())
                    tg.create_task(self.udp_sender_task())
                    tg.create_task(self.gemini_sender_task(session))
                    tg.create_task(self.gemini_receiver_task(session))
                    
        except Exception as e:
            logger.error(f"Connection failed: {e}")
        finally:
            self.running = False


if __name__ == "__main__":
    if not GEMINI_API_KEY:
        logger.error("GEMINI_API_KEY not found in environment variables.")
        exit(1)

    proxy = AudioProxy()
    try:
        asyncio.run(proxy.run())
    except KeyboardInterrupt:
        logger.info("Stopping...")
