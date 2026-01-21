

# from aiohttp import web, WSMsgType

# HTML Template for Web Client
INDEX_HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>Gemini Live Proxy Test</title>
    <style>
        body { font-family: sans-serif; max-width: 600px; margin: 2rem auto; padding: 0 1rem; text-align: center; }
        button { padding: 10px 20px; font-size: 1.2rem; cursor: pointer; margin: 10px; }
        #status { margin-top: 20px; color: #666; }
        .recording { background-color: #ff4444; color: white; }
        .monitor { background-color: #4CAF50; color: white; }
    </style>
</head>
<body>
    <h1>Gemini Live Proxy Test</h1>
    
    <div style="border: 1px solid #ccc; padding: 20px; border-radius: 8px; margin-bottom: 20px;">
        <h3>Microphone Input</h3>
        <p>Stream your browser microphone to Gemini.</p>
        <button id="startBtn">Start Mic</button>
        <button id="stopBtn" disabled>Stop Mic</button>
    </div>

    <div style="border: 1px solid #ccc; padding: 20px; border-radius: 8px;">
        <h3>ESP32 Monitor</h3>
        <p>You will automatically hear audio from the ESP32 when it is connected.</p>
    </div>

    <div id="status">Ready</div>

    <script>
        let audioContext;
        let websocket;
        let processor;
        let source;
        let isRecording = false;
        let nextStartTime = 0;

        const startBtn = document.getElementById('startBtn');
        const stopBtn = document.getElementById('stopBtn');
        const status = document.getElementById('status');

        // Initialize Audio Context on user interaction
        async function initAudio() {
            if (!audioContext) {
                audioContext = new (window.AudioContext || window.webkitAudioContext)({sampleRate: 48000});
            }
            if (audioContext.state === 'suspended') {
                await audioContext.resume();
            }
        }

        async function connectWebSocket() {
            if (websocket && websocket.readyState === WebSocket.OPEN) return;

            websocket = new WebSocket('ws://' + window.location.host + '/ws');
            websocket.binaryType = 'arraybuffer';

            websocket.onopen = () => {
                status.innerText = "Connected to Proxy";
            };

            websocket.onmessage = async (event) => {
                // Ensure audio context is ready before playing
                await initAudio(); 
                playAudio(event.data);
            };

            websocket.onclose = () => {
                stopRecording();
                status.innerText = "Disconnected";
            };
        }

        // Auto-connect on load so we can hear ESP32 immediately (after interaction)
        window.addEventListener('load', () => {
            connectWebSocket();
            // We need a user interaction to unlock audio playback usually
            document.body.addEventListener('click', initAudio, { once: true });
        });

        startBtn.onclick = async () => {
            await initAudio();
            await connectWebSocket();

            try {
                const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
                startRecording(stream);
            } catch (err) {
                console.error(err);
                status.innerText = "Error: " + err.message;
            }
        };

        stopBtn.onclick = stopRecording;

        function startRecording(stream) {
            isRecording = true;
            source = audioContext.createMediaStreamSource(stream);
            processor = audioContext.createScriptProcessor(4096, 1, 1);

            processor.onaudioprocess = (e) => {
                if (!isRecording) return;
                const inputData = e.inputBuffer.getChannelData(0);
                const pcmData = new Int16Array(inputData.length);
                for (let i = 0; i < inputData.length; i++) {
                    let s = Math.max(-1, Math.min(1, inputData[i]));
                    pcmData[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
                }
                if (websocket && websocket.readyState === WebSocket.OPEN) {
                    websocket.send(pcmData.buffer);
                }
            };

            source.connect(processor);
            processor.connect(audioContext.destination);
            startBtn.disabled = true;
            stopBtn.disabled = false;
            startBtn.classList.add('recording');
        }

        function stopRecording() {
            isRecording = false;
            if (source) { source.disconnect(); source = null; }
            if (processor) { processor.disconnect(); processor = null; }
            // Don't close websocket so we can still hear incoming audio
            
            startBtn.disabled = false;
            stopBtn.disabled = true;
            startBtn.classList.remove('recording');
            status.innerText = "Mic Stopped (Still Listening)";
            nextStartTime = 0;
        }

        function playAudio(arrayBuffer) {
            if (!audioContext) return;
            
            const data = new Int16Array(arrayBuffer);
            const floatData = new Float32Array(data.length);
            
            for (let i = 0; i < data.length; i++) {
                floatData[i] = data[i] / 32768.0;
            }

            const buffer = audioContext.createBuffer(1, floatData.length, 48000);
            buffer.getChannelData(0).set(floatData);

            const sourceNode = audioContext.createBufferSource();
            sourceNode.buffer = buffer;
            sourceNode.connect(audioContext.destination);

            const currentTime = audioContext.currentTime;
            if (nextStartTime < currentTime) {
                nextStartTime = currentTime;
            }
            sourceNode.start(nextStartTime);
            nextStartTime += buffer.duration;
        }
    </script>
</body>
</html>
"""

