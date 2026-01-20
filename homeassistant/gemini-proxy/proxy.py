import asyncio
import os
import socket
import logging
import numpy as np
from scipy import signal as scipy_signal
from google import genai
import onnxruntime

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
SILENCE_CHUNKS_TO_STOP = 10

# Logging
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)


class VADWrapper:
    def __init__(self):
        if not os.path.exists(VAD_MODEL_PATH):
            logger.info("Downloading Silero VAD model...")
            import urllib.request

            urllib.request.urlretrieve(
                "https://github.com/snakers4/silero-vad/raw/refs/heads/master/src/silero_vad/data/silero_vad.onnx",
                VAD_MODEL_PATH,
            )

        # Suppress onnxruntime warnings
        sess_options = onnxruntime.SessionOptions()
        sess_options.log_severity_level = 3
        self.session = onnxruntime.InferenceSession(VAD_MODEL_PATH, sess_options)
        self.reset_states()

    def reset_states(self):
        self._h = np.zeros((2, 1, 64), dtype=np.float32)
        self._c = np.zeros((2, 1, 64), dtype=np.float32)

    def is_speech(self, audio_chunk_16k):
        audio_int16 = np.frombuffer(audio_chunk_16k, dtype=np.int16)
        audio_float32 = audio_int16.astype(np.float32) / 32768.0

        if len(audio_float32) < 32:
            return 0.0

        input_data = {
            "input": audio_float32[np.newaxis, :],
            "sr": np.array([16000], dtype=np.int64),
            "h": self._h,
            "c": self._c,
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

        self.ai_is_speaking = False
        self.connection_active = asyncio.Event()  # Used to trigger connection

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
        logger.info("UDP Listener started")
        while self.running:
            try:
                data, addr = await loop.sock_recvfrom(self.udp_sock, 4096)

                if not self.esp_address:
                    logger.info(f"Connection established from ESP32 at {addr}")
                    self.esp_address = addr
                    self.connection_active.set()  # Signal to start Gemini
                elif self.esp_address[0] != addr[0]:
                    self.esp_address = addr
                    self.connection_active.set()

                # Resample 48k -> 16k
                audio_16k = await asyncio.to_thread(
                    self.resample_audio, data, ESP_INPUT_RATE, GEMINI_INPUT_RATE
                )

                # VAD Check
                prob = await asyncio.to_thread(self.vad.is_speech, audio_16k)

                # Gating Logic
                if self.ai_is_speaking:
                    if prob > 0.8:  # Strong speech detected (Barge-in)
                        logger.debug("Barge-in detected!")
                        await self.audio_queue_mic.put(audio_16k)
                else:
                    await self.audio_queue_mic.put(audio_16k)

            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"UDP Receive Error: {e}")
                await asyncio.sleep(0.1)

    async def udp_sender_task(self):
        loop = asyncio.get_running_loop()
        logger.info("UDP Sender started")
        while self.running:
            try:
                chunk = await self.audio_queue_speaker.get()

                if self.esp_address:
                    target_port = ESP_RESPONSE_PORT
                    target_addr = (self.esp_address[0], target_port)

                    # Split into smaller UDP packets to avoid fragmentation issues
                    max_size = 1024
                    for i in range(0, len(chunk), max_size):
                        sub_chunk = chunk[i : i + max_size]
                        await loop.sock_sendto(self.udp_sock, sub_chunk, target_addr)

            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"UDP Send Error: {e}")

    async def gemini_session_handler(self):
        """Manages the Gemini Session, including connection and tasks"""

        # Wait for first UDP packet before connecting
        logger.info("Waiting for ESP32 connection before connecting to Gemini...")
        await self.connection_active.wait()

        config = {
            "response_modalities": ["AUDIO"],
            "system_instruction": "You are a helpful and friendly AI assistant. Be concise.",
        }

        logger.info(f"Connecting to Gemini Live ({GEMINI_MODEL})...")

        try:
            async with self.client.aio.live.connect(
                model=GEMINI_MODEL, config=config
            ) as session:
                logger.info("Connected to Gemini API!")

                async with asyncio.TaskGroup() as tg:
                    tg.create_task(self.gemini_sender_task(session))
                    tg.create_task(self.gemini_receiver_task(session))

        except Exception as e:
            logger.error(f"Gemini Session Error: {e}")
            # Clear queue to prevent stale audio from rushing in on reconnect
            while not self.audio_queue_mic.empty():
                self.audio_queue_mic.get_nowait()
            raise  # Propagate to trigger reconnection in run()

    async def gemini_sender_task(self, session):
        while self.running:
            try:
                chunk = await self.audio_queue_mic.get()
                await session.send_realtime_input(
                    audio={
                        "data": chunk,
                        "mime_type": f"audio/pcm;rate={GEMINI_INPUT_RATE}",
                    }
                )
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"Gemini Send Error: {e}")
                raise  # Break the TaskGroup to restart session

    async def gemini_receiver_task(self, session):
        while self.running:
            try:
                async for response in session.receive():
                    if response.server_content and response.server_content.model_turn:
                        for part in response.server_content.model_turn.parts:
                            if part.inline_data and isinstance(
                                part.inline_data.data, bytes
                            ):
                                self.ai_is_speaking = True
                                audio_24k = part.inline_data.data

                                # Resample 24k -> 48k for ESP32
                                audio_48k = await asyncio.to_thread(
                                    self.resample_audio,
                                    audio_24k,
                                    GEMINI_OUTPUT_RATE,
                                    ESP_OUTPUT_RATE,
                                )
                                await self.audio_queue_speaker.put(audio_48k)

                    if (
                        response.server_content
                        and response.server_content.turn_complete
                    ):
                        self.ai_is_speaking = False

            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"Gemini Receive Error: {e}")
                self.ai_is_speaking = False
                raise  # IMPORTANT: Break loop to force reconnection

    async def handle_http_health_check(self, reader, writer):
        """Simple HTTP handler to confirm service is up."""
        try:
            # Read the request (we don't strictly need it, but good practice to grab headers)
            request = await reader.read(1024)

            # Simple HTTP 200 Response
            response = (
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Gemini Live Proxy is Running!"
            )

            writer.write(response.encode("utf-8"))
            await writer.drain()
        except Exception as e:
            logger.error(f"HTTP Health Check Error: {e}")
        finally:
            writer.close()
            await writer.wait_closed()

    async def run(self):
        # Start persistent UDP tasks
        udp_listener = asyncio.create_task(self.udp_listener_task())
        udp_sender = asyncio.create_task(self.udp_sender_task())

        # Start TCP/HTTP Health Check on same port (TCP 7000)
        http_server = await asyncio.start_server(
            self.handle_http_health_check, UDP_IP, UDP_PORT
        )
        logger.info(f"HTTP Health Check available at http://{UDP_IP}:{UDP_PORT}")

        # Connection Retry Loop
        while self.running:
            try:
                await self.gemini_session_handler()
            except Exception:
                logger.info("Gemini session ended. Reconnecting in 2 seconds...")
                await asyncio.sleep(2)
        # Cleanup
        http_server.close()
        await http_server.wait_closed()
        udp_listener.cancel()
        udp_sender.cancel()


if __name__ == "__main__":
    if not GEMINI_API_KEY and os.path.exists("/data/options.json"):
        try:
            import json

            with open("/data/options.json", "r") as f:
                options = json.load(f)
                GEMINI_API_KEY = options.get("gemini_api_key")
        except Exception as e:
            logger.error(f"Failed to read options.json: {e}")

    if not GEMINI_API_KEY:
        logger.error("GEMINI_API_KEY not found.")
        exit(1)

    proxy = AudioProxy()
    try:
        asyncio.run(proxy.run())
    except KeyboardInterrupt:
        logger.info("Stopping...")
