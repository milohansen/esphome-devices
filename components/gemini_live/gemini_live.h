#include "esphome.h"
#include "esphome/components/socket/socket.h"
#include "esphome/components/microphone/microphone.h"
#include "esphome/components/speaker/speaker.h"
#include "esphome/core/log.h"
#include <vector>
#include <cstdint>
#include <arpa/inet.h> // For inet_pton, htons, etc.
#include <netinet/in.h>

// Define port
#define UDP_PORT 7000

namespace esphome {
namespace gemini_live {

class GeminiLiveComponent : public Component {
 public:
  std::unique_ptr<esphome::socket::Socket> socket_ = nullptr;
  std::string proxy_url; // Format: http://ip:port or just ip:port
  struct sockaddr_in proxy_addr;
  bool proxy_addr_valid = false;
  
  bool is_streaming = false;
  
  esphome::microphone::Microphone *mic{nullptr};
  esphome::speaker::Speaker *speaker{nullptr};

  GeminiLiveComponent(esphome::microphone::Microphone *mic_ptr, esphome::speaker::Speaker *speaker_ptr) 
      : mic(mic_ptr), speaker(speaker_ptr) {}

  void setup() override {
    ESP_LOGI("gemini_live", "Setting up Gemini Live Component...");
    
    // Use raw macros (AF_INET = 2, etc)
    this->socket_ = esphome::socket::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (!this->socket_) {
        ESP_LOGE("gemini_live", "Failed to create socket");
        this->mark_failed();
        return;
    }
    
    bool blocking = false;
    this->socket_->setblocking(blocking);

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(UDP_PORT);

    if (this->socket_->bind((struct sockaddr *)&server, sizeof(server)) != 0) {
        ESP_LOGE("gemini_live", "Failed to bind socket port %d/udp", UDP_PORT);
        this->mark_failed();
        return;
    }
    ESP_LOGI("gemini_live", "UDP listening on port %d", UDP_PORT);

    // Register Microphone Callback
    if (this->mic != nullptr) {
        this->mic->add_data_callback([this](const std::vector<uint8_t> &data) {
            this->send_audio_data(data);
        });
        ESP_LOGI("gemini_live", "Microphone callback registered");
    }
  }

  void loop() override {
    if (!this->socket_) return;

    // Check for incoming UDP packets
    uint8_t buf[2048];
    struct sockaddr_storage src;
    socklen_t len = sizeof(src);
    
    // recvfrom in ESPHome Socket API has 4 args (no flags)
    ssize_t read = this->socket_->recvfrom(buf, sizeof(buf), (struct sockaddr*)&src, &len);
    
    if (read > 0) {
      // Send to Speaker
      if (this->speaker != nullptr) {
        this->speaker->play(buf, read);
      }
    }
  }

  void start_streaming(std::string url) {
    this->proxy_url = url;
    this->parse_proxy_url();
    
    this->is_streaming = true;
    
    if (this->mic->is_stopped()) {
        this->mic->start();
    }
    ESP_LOGI("gemini_live", "Started streaming to %s", url.c_str());
  }

  void stop_streaming() {
    this->is_streaming = false;
    if (this->mic->is_running()) {
        this->mic->stop();
    }
    ESP_LOGI("gemini_live", "Stopped streaming");
  }
  
  void parse_proxy_url() {
      // Simple parsing: http://host:port or host:port
      std::string host = this->proxy_url;
      int port = UDP_PORT;

      // Strip http://
      if (host.find("http://") == 0) {
          host = host.substr(7);
      }
      
      // Split port
      size_t colon = host.find(':');
      if (colon != std::string::npos) {
          port = std::stoi(host.substr(colon + 1));
          host = host.substr(0, colon);
      }

      // Resolve IP
      memset(&this->proxy_addr, 0, sizeof(this->proxy_addr));
      this->proxy_addr.sin_family = AF_INET;
      this->proxy_addr.sin_port = htons(port);
      
      // Use inet_pton for standard IP parsing
      if (inet_pton(AF_INET, host.c_str(), &this->proxy_addr.sin_addr) == 1) {
           this->proxy_addr_valid = true;
           ESP_LOGI("gemini_live", "Resolved proxy to %s:%d", host.c_str(), port);
      } else {
           ESP_LOGW("gemini_live", "Failed to parse IP from %s", host.c_str());
           this->proxy_addr_valid = false;
      }
  }

  void send_audio_data(const std::vector<uint8_t> &data) {
      if (!this->is_streaming || !this->proxy_addr_valid || !this->socket_) {
          return;
      }
      
      const uint8_t* raw_data = data.data();
      size_t len = data.size();
      
      // sendto in ESPHome Socket API: (buf, len, flags, addr, addrlen)
      this->socket_->sendto(raw_data, len, 0, (struct sockaddr*)&this->proxy_addr, sizeof(this->proxy_addr));
  }
};

} // namespace gemini_live
} // namespace esphome
