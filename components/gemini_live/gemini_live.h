#pragma once

#include "esphome.h"
#include "esphome/components/socket/socket.h"
#include "esphome/components/microphone/microphone.h"
#include "esphome/components/speaker/speaker.h"
#include "esphome/core/log.h"
#include <vector>
#include <queue>
#include <mutex>
#include <cstdint>
#include <arpa/inet.h>
#include <netinet/in.h>

// Define port
#define UDP_PORT 7000

namespace esphome
{
  namespace gemini_live
  {

    class GeminiLiveComponent : public Component
    {
    public:
      std::unique_ptr<esphome::socket::Socket> socket_ = nullptr;
      std::string proxy_url;
      struct sockaddr_in proxy_addr;
      bool proxy_addr_valid = false;

      bool is_streaming = false;

      // Thread safety for audio queue
      std::mutex queue_mutex_;
      std::queue<std::vector<uint8_t>> tx_queue_;

      esphome::microphone::Microphone *mic{nullptr};
      esphome::speaker::Speaker *speaker{nullptr};

      GeminiLiveComponent(esphome::microphone::Microphone *mic_ptr, esphome::speaker::Speaker *speaker_ptr)
          : mic(mic_ptr), speaker(speaker_ptr) {}

      void setup() override
      {
        ESP_LOGI("gemini_live", "Setting up Gemini Live Component...");

        // Register Microphone Callback
        if (this->mic != nullptr)
        {
          this->mic->add_data_callback([this](const std::vector<uint8_t> &data)
                                       { this->queue_audio_data(data); });
          ESP_LOGI("gemini_live", "Microphone callback registered");
        }
      }
      float get_setup_priority() const override { return setup_priority::LATE; }

      void loop() override
      {
        if (!this->socket_)
          return;

        // 1. Process outgoing audio queue (from Mic)
        this->process_tx_queue();

        // 2. Check for incoming UDP packets (to Speaker)
        uint8_t buf[2048];
        struct sockaddr_storage src;
        socklen_t len = sizeof(src);

        ssize_t read = this->socket_->recvfrom(buf, sizeof(buf), (struct sockaddr *)&src, &len);

        if (read > 0)
        {
          if (this->speaker != nullptr)
          {
            // Speaker play is usually safe in loop (main thread)
            this->speaker->play(buf, read);
          }
        }
      }

      void initialize()
      {
        ESP_LOGI("gemini_live", "Initializing Gemini Live Component...");

        this->socket_ = esphome::socket::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (!this->socket_)
        {
          ESP_LOGE("gemini_live", "Failed to create socket");
          this->mark_failed();
          return;
        }

        // Set non-blocking
        this->socket_->setblocking(false);

        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(UDP_PORT);

        if (this->socket_->bind((struct sockaddr *)&server, sizeof(server)) != 0)
        {
          ESP_LOGE("gemini_live", "Failed to bind socket port %d/udp", UDP_PORT);
          this->mark_failed();
          return;
        }
        ESP_LOGI("gemini_live", "UDP listening on port %d", UDP_PORT);

        // // Register Microphone Callback
        // if (this->mic != nullptr) {
        //     this->mic->add_data_callback([this](const std::vector<uint8_t> &data) {
        //         this->queue_audio_data(data);
        //     });
        //     ESP_LOGI("gemini_live", "Microphone callback registered");
        // }
      }

      void start_streaming(std::string url)
      {
        this->proxy_url = url;
        this->parse_proxy_url();

        // Clear any stale data in queue
        {
          std::lock_guard<std::mutex> lock(this->queue_mutex_);
          std::queue<std::vector<uint8_t>> empty;
          std::swap(this->tx_queue_, empty);
        }

        this->is_streaming = true;

        if (this->mic->is_stopped())
        {
          this->mic->start();
        }
        ESP_LOGI("gemini_live", "Started streaming to %s", url.c_str());
      }

      void stop_streaming()
      {
        this->is_streaming = false;
        if (this->mic->is_running())
        {
          this->mic->stop();
        }
        ESP_LOGI("gemini_live", "Stopped streaming");
      }

      void parse_proxy_url()
      {
        std::string host = this->proxy_url;
        int port = UDP_PORT;

        if (host.find("http://") == 0)
        {
          host = host.substr(7);
        }

        size_t colon = host.find(':');
        if (colon != std::string::npos)
        {
          // try {
          port = std::stoi(host.substr(colon + 1));
          // } catch (...) {
          //   port = UDP_PORT;
          // }
          host = host.substr(0, colon);
        }

        memset(&this->proxy_addr, 0, sizeof(this->proxy_addr));
        this->proxy_addr.sin_family = AF_INET;
        this->proxy_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &this->proxy_addr.sin_addr) == 1)
        {
          this->proxy_addr_valid = true;
          ESP_LOGI("gemini_live", "Resolved proxy to %s:%d", host.c_str(), port);
        }
        else
        {
          ESP_LOGW("gemini_live", "Failed to parse IP from %s", host.c_str());
          this->proxy_addr_valid = false;
        }
      }

      // Called from Microphone Task (Keep this fast!)
      void queue_audio_data(const std::vector<uint8_t> &data)
      {
        if (!this->is_streaming)
          return;

        std::lock_guard<std::mutex> lock(this->queue_mutex_);
        // Limit queue size to prevent OOM if network hangs
        if (this->tx_queue_.size() < 50)
        {
          this->tx_queue_.push(data);
        }
      }

      // Called from Main Loop
      void process_tx_queue()
      {
        if (!this->is_streaming || !this->proxy_addr_valid || !this->socket_)
          return;

        // Process up to 10 packets per loop to prevent blocking if queue built up
        int count = 0;
        while (count < 10)
        {
          std::vector<uint8_t> packet;
          {
            std::lock_guard<std::mutex> lock(this->queue_mutex_);
            if (this->tx_queue_.empty())
              break;

            packet = this->tx_queue_.front();
            this->tx_queue_.pop();
          }

          if (!packet.empty())
          {
            this->socket_->sendto(packet.data(), packet.size(), 0, (struct sockaddr *)&this->proxy_addr, sizeof(this->proxy_addr));
          }
          count++;
        }
      }
    };

  } // namespace gemini_live
} // namespace esphome