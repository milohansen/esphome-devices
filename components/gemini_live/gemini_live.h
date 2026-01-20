#pragma once

#include "esphome.h"
#include "WiFiUdp.h"
#include <vector>

// Define port
#define UDP_PORT 7000
#define PROXY_PORT 7000
// We need to know the Proxy IP. 
// Option 1: Hardcode (User must replace)
// Option 2: Broadcast (Might lose packets)
// Option 3: Use a sensor/text_sensor from HA to set it.
// For now, allow setting via public variable or #define, or constructor.

namespace esphome {
namespace gemini_live {

class GeminiLiveComponent : public Component {
 public:
  WiFiUDP udp;
  std::string proxy_ip;
  uint16_t proxy_port = PROXY_PORT;
  bool is_streaming = false;
  
  // Reference to Microphone and Speaker
  // We will cast to generic Microphone/Speaker interfaces if possible, 
  // or use IDs passed in YAML and resolved in setup() via `id(name)`.
  // Ideally, use: microphone::Microphone *mic;
  microphone::Microphone *mic{nullptr};
  speaker::Speaker *speaker{nullptr};

  GeminiLiveComponent(microphone::Microphone *mic_ptr, speaker::Speaker *speaker_ptr) 
      : mic(mic_ptr), speaker(speaker_ptr) {}

  void setup() override {
    ESP_LOGI("gemini_live", "Setting up Gemini Live Component...");
    
    if (this->udp.begin(UDP_PORT)) {
      ESP_LOGI("gemini_live", "UDP listening on port %d", UDP_PORT);
    } else {
      ESP_LOGE("gemini_live", "Failed to bind UDP port");
    }

    // Register Microphone Callback
    if (this->mic != nullptr) {
        this->mic->add_data_callback([this](const std::vector<int16_t> &data) {
            this->send_audio_data(data);
        });
        ESP_LOGI("gemini_live", "Microphone callback registered");
    }
  }

  void loop() override {
    // Check for incoming UDP packets (Audio from Proxy)
    int packetSize = this->udp.parsePacket();
    if (packetSize) {
      if (!this->is_streaming) {
          // Verify source? For now, accept all to be simpler.
      }
      
      // Read packet
      std::vector<uint8_t> buffer;
      buffer.resize(packetSize);
      this->udp.read(buffer.data(), packetSize);
      
      // Send to Speaker
      // buffer contains raw PCM 48kHz 16bit mono (as per our Proxy logic)
      if (this->speaker != nullptr) {
        // speaker->play expects: const void *data, size_t length
        // Note: Check if speaker expects int16_t or bytes. usually bytes.
        this->speaker->play(buffer.data(), buffer.size());
      }
    }
  }

  void start_streaming(std::string ip) {
    this->proxy_ip = ip;
    this->is_streaming = true;
    
    // Start Microphone
    if (this->mic->is_stopped()) {
        this->mic->start();
    }
    ESP_LOGI("gemini_live", "Started streaming to %s", ip.c_str());
  }

  void stop_streaming() {
    this->is_streaming = false;
    if (this->mic->is_running()) {
        this->mic->stop();
    }
    ESP_LOGI("gemini_live", "Stopped streaming");
  }
  
  void send_audio_data(const std::vector<int16_t> &data) {
      if (!this->is_streaming || this->proxy_ip.empty()) {
          return;
      }

      // Convert int16_t to bytes (Little Endian for ESP32/Gemini usually)
      // data.size() is number of SAMPLES (int16). 
      // Byte size is size * 2.
      
      // Send directly via UDP
      this->udp.beginPacket(this->proxy_ip.c_str(), this->proxy_port);
      this->udp.write((const uint8_t*)data.data(), data.size() * 2);
      this->udp.endPacket();
  }
};

} // namespace gemini_live
} // namespace esphome
