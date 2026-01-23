#pragma once

#include "esphome.h"
#include "esphome/components/socket/socket.h"
#include "esphome/components/microphone/microphone.h"
#include "esphome/components/speaker/speaker.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esp_websocket_client.h"
#include "esphome/components/json/json_util.h"
#include "base64.h"
#include <vector>
#include <queue>

namespace ArduinoJson {
  class DynamicJsonDocument;
}
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
      // Bridge mode variables
      std::unique_ptr<esphome::socket::Socket> socket_ = nullptr;
      std::string proxy_url;
      struct sockaddr_in proxy_addr;
      bool proxy_addr_valid = false;

      // Direct mode variables
      esp_websocket_client_handle_t ws_client_ = nullptr;
      esphome::text_sensor::TextSensor *gemini_token_sensor_{nullptr};
      esphome::text_sensor::TextSensor *gemini_config_sensor_{nullptr};

      // Common variables
      std::string connection_type_ = "bridge";
      std::string model_ = "gemini-2.5-flash-native-audio-preview-12-2025";
      bool is_streaming = false;
      std::mutex queue_mutex_;
      std::queue<std::vector<uint8_t>> tx_queue_;
      esphome::microphone::Microphone *mic{nullptr};
      esphome::speaker::Speaker *speaker{nullptr};
      CallbackManager<void()> on_stop_callback_;

      GeminiLiveComponent(esphome::microphone::Microphone *mic_ptr, esphome::speaker::Speaker *speaker_ptr)
          : mic(mic_ptr), speaker(speaker_ptr) {}

      void setup() override
      {
        ESP_LOGI("gemini_live", "Setting up Gemini Live Component...");
      }

      float get_setup_priority() const override { return setup_priority::LATE; }

      void set_connection_type(const std::string &connection_type) { this->connection_type_ = connection_type; }
      std::string get_connection_type() { return this->connection_type_; }
      void set_model(const std::string &model) { this->model_ = model; }
      void set_gemini_token_sensor(esphome::text_sensor::TextSensor *token_sensor) { this->gemini_token_sensor_ = token_sensor; }
      void set_gemini_config_sensor(esphome::text_sensor::TextSensor *config_sensor) { this->gemini_config_sensor_ = config_sensor; }
      void add_on_stop_callback(std::function<void()> callback) { this->on_stop_callback_.add(std::move(callback)); }

      void loop() override
      {
        if (this->connection_type_ == "bridge")
        {
          this->loop_bridge();
        }
        else
        {
          this->process_tx_queue_direct();
        }
      }

      void initialize(std::string url)
      {
        this->proxy_url = url;
        ESP_LOGI("gemini_live", "Initializing Gemini Live Component with URL: %s", url.c_str());
        this->initialize();
      }

      void initialize()
      {
        ESP_LOGI("gemini_live", "Initializing Gemini Live Component...");
        if (this->connection_type_ == "bridge")
        {
          this->initialize_bridge();
        }
        else
        {
          this->initialize_direct();
        }

        if (this->mic != nullptr)
        {
          this->mic->add_data_callback([this](const std::vector<uint8_t> &data)
                                       { this->queue_audio_data(data); });
          ESP_LOGI("gemini_live", "Microphone callback registered");
        }
      }

      void start_streaming()
      {
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

        if (this->connection_type_ == "bridge")
        {
          this->start_streaming_bridge();
        }
        else
        {
          this->start_streaming_direct();
        }
        ESP_LOGI("gemini_live", "Started streaming");
      }

      void stop_streaming()
      {
        this->is_streaming = false;
        if (this->mic->is_running())
        {
          this->mic->stop();
        }
        ESP_LOGI("gemini_live", "Stopped streaming");

        if (this->connection_type_ == "direct" && this->ws_client_)
        {
          esp_websocket_client_close(this->ws_client_, 1000);
        }

        this->on_stop_callback_.call();
      }

    protected:
      void loop_bridge()
      {
        if (!this->socket_)
          return;

        if (this->is_streaming && this->mic->is_stopped())
        {
          this->mic->start();
        }

        this->process_tx_queue_bridge();

        uint8_t buf[2048];
        struct sockaddr_storage src;
        socklen_t len = sizeof(src);
        ssize_t read = this->socket_->recvfrom(buf, sizeof(buf), (struct sockaddr *)&src, &len);
        if (read > 0)
        {
          ESP_LOGD("gemini_live", "Received %d bytes from proxy", read);
          if (read == 11 && memcmp(buf, "GEMINI_STOP", 11) == 0)
          {
            ESP_LOGW("gemini_live", "Received STOP command from proxy");
            this->stop_streaming();
            return;
          }
          if (this->speaker != nullptr)
          {
            this->speaker->play(buf, read);
          }
        }
      }

      void initialize_bridge()
      {
        this->socket_ = esphome::socket::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (!this->socket_)
        {
          ESP_LOGE("gemini_live", "Failed to create socket");
          this->mark_failed();
          return;
        }

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
      }

      void initialize_direct()
      {
        esp_websocket_client_config_t ws_cfg = {};
        ws_cfg.uri = "wss://generativelanguage.googleapis.com/ws/google.ai.generativelanguage.v1beta.GenerativeService.BidiGenerateContent";
        ws_cfg.user_context = this;

        this->ws_client_ = esp_websocket_client_init(&ws_cfg);
        esp_websocket_register_events(this->ws_client_, ESP_WEBSOCKET_EVENT_ANY, &GeminiLiveComponent::websocket_event_handler, this);
      }

      void start_streaming_bridge()
      {
        this->parse_proxy_url();
      }

      void start_streaming_direct()
      {
        if (this->gemini_token_sensor_ == nullptr || this->gemini_config_sensor_ == nullptr)
        {
          ESP_LOGE("gemini_live", "Gemini token or config sensor not configured");
          this->stop_streaming();
          return;
        }

        std::string token = this->gemini_token_sensor_->state;
        if (token.empty())
        {
          ESP_LOGE("gemini_live", "Gemini token is empty");
          this->stop_streaming();
          return;
        }

        std::string auth_header = "Bearer " + token;
        esp_websocket_client_set_header(this->ws_client_, "Authorization", auth_header.c_str());

        esp_websocket_client_start(this->ws_client_);
      }

      void queue_audio_data(const std::vector<uint8_t> &data)
      {
        if (!this->is_streaming)
          return;
        std::lock_guard<std::mutex> lock(this->queue_mutex_);
        if (this->tx_queue_.size() < 50)
        {
          this->tx_queue_.push(data);
        }
      }

      void process_tx_queue()
      {
        if (this->connection_type_ == "bridge")
        {
          this->process_tx_queue_bridge();
        }
        else
        {
          this->process_tx_queue_direct();
        }
      }

      void process_tx_queue_bridge()
      {
        if (!this->is_streaming || !this->proxy_addr_valid || !this->socket_)
          return;

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
        if (count > 0)
        {
          ESP_LOGD("gemini_live", "Processed %d packets from TX queue", count);
        }
      }

      void process_tx_queue_direct()
      {
        if (!this->is_streaming || !this->ws_client_ || !esp_websocket_client_is_connected(this->ws_client_))
          return;

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
            std::string encoded_audio = base64_encode(packet.data(), packet.size());
            std::string json_payload = "{\"realtimeInput\": {\"audio\": {\"mime_type\": \"audio/l16; rate=16000\", \"data\": \"" + encoded_audio + "\"}}}";
            esp_websocket_client_send_text(this->ws_client_, json_payload.c_str(), json_payload.length(), 100);
          }
          count++;
        }
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
          const char *port_str = host.c_str() + colon + 1;
          char *end;
          long port_long = strtol(port_str, &end, 10);
          if (end == port_str || *end != '\0' || port_long < 0 || port_long > 65535)
          {
            ESP_LOGE("gemini_live", "Invalid port number in proxy URL");
            this->proxy_addr_valid = false;
            return;
          }
          port = (int)port_long;
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

      static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
      {
        GeminiLiveComponent *component = (GeminiLiveComponent *)handler_args;
        esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
        switch (event_id)
        {
        case WEBSOCKET_EVENT_CONNECTED:
        {
          ESP_LOGI("gemini_live", "WebSocket Connected");
          std::string config = component->gemini_config_sensor_->state;
          esp_websocket_client_send_text(component->ws_client_, config.c_str(), config.length(), 100);
          break;
        }
        case WEBSOCKET_EVENT_DISCONNECTED:
          ESP_LOGI("gemini_live", "WebSocket Disconnected");
          break;
        case WEBSOCKET_EVENT_DATA:
        {
          ESP_LOGD("gemini_live", "WebSocket received data, len=%d", data->data_len);
          if (component->speaker != nullptr)
          {
            ArduinoJson::DynamicJsonDocument doc(data->data_len);
            deserializeJson(doc, (const char *)data->data_ptr, data->data_len);
            if (doc.containsKey("serverContent"))
            {
              if (doc["serverContent"].containsKey("modelTurn"))
              {
                if (doc["serverContent"]["modelTurn"].containsKey("parts"))
                {
                  for (JsonObject part : doc["serverContent"]["modelTurn"]["parts"].as<JsonArray>())
                  {
                    if (part.containsKey("inlineData"))
                    {
                      if (part["inlineData"].containsKey("data"))
                      {
                        std::string encoded_audio = part["inlineData"]["data"];
                        std::vector<unsigned char> decoded_audio = base64_decode(encoded_audio);
                        component->speaker->play(decoded_audio.data(), decoded_audio.size());
                      }
                    }
                  }
                }
              }
            }
          }
          break;
        }
        case WEBSOCKET_EVENT_ERROR:
          ESP_LOGE("gemini_live", "WebSocket Error");
          break;
        }
      }
    };

    class GeminiLiveOnStopStreamingTrigger : public Trigger<>
    {
    public:
      explicit GeminiLiveOnStopStreamingTrigger(GeminiLiveComponent *parent)
      {
        parent->add_on_stop_callback([this]()
                                     { this->trigger(); });
      }
    };

  } // namespace gemini_live
} // namespace esphome