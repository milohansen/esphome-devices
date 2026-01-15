/*
 * Material Theme Component for ESPHome
 * 
 * Integrates Google's Material Color Utilities C++ library for dynamic theming
 * Generates Material Design 3 compliant color schemes from source colors
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/color.h"

// Include Material Color Utilities - we'll download these separately
// For now, forward declare what we need
#include <vector>
#include <functional>

namespace esphome {
namespace material_theme {

static const char *const TAG = "material_theme";

// Forward declarations for Material Color Utilities types
// We'll include the full headers once we integrate the library
typedef uint32_t Argb;

enum SchemeVariant {
  VARIANT_TONAL_SPOT = 0,
  VARIANT_VIBRANT = 1,
  VARIANT_EXPRESSIVE = 2,
  VARIANT_CONTENT = 3,
  VARIANT_MONOCHROME = 4,
  VARIANT_NEUTRAL = 5,
};

// Material Design color roles (24 total in full spec)
struct ColorScheme {
  // Primary colors
  Argb primary;
  Argb on_primary;
  Argb primary_container;
  Argb on_primary_container;
  
  // Secondary colors
  Argb secondary;
  Argb on_secondary;
  Argb secondary_container;
  Argb on_secondary_container;
  
  // Tertiary colors
  Argb tertiary;
  Argb on_tertiary;
  Argb tertiary_container;
  Argb on_tertiary_container;
  
  // Error colors
  Argb error;
  Argb on_error;
  Argb error_container;
  Argb on_error_container;
  
  // Surface colors
  Argb background;
  Argb on_background;
  Argb surface;
  Argb on_surface;
  Argb surface_variant;
  Argb on_surface_variant;
  
  // Outline colors
  Argb outline;
  Argb outline_variant;
  
  // Additional surface colors
  Argb surface_container_lowest;
  Argb surface_container_low;
  Argb surface_container;
  Argb surface_container_high;
  Argb surface_container_highest;
  
  // Inverse colors
  Argb inverse_surface;
  Argb inverse_on_surface;
  Argb inverse_primary;
  
  // Shadow/scrim
  Argb shadow;
  Argb scrim;
};

class MaterialThemeComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  
  // Configuration setters
  void set_source_color(uint32_t color) { this->source_color_ = color; }
  void set_is_dark(bool is_dark) { this->is_dark_ = is_dark; }
  void set_contrast_level(float level) { this->contrast_level_ = level; }
  // void set_variant(SchemeVariant variant) { this->variant_ = variant; }
  void set_variant(SchemeVariant variant) {
    ESP_LOGCONFIG(TAG, "set_variant was: %d, desired: %d", this->variant_, variant);
    this->variant_ = variant;
    ESP_LOGCONFIG(TAG, "set_variant confirm: %d", this->variant_);
  }
  void set_variant(int variant) { this->variant_ = static_cast<SchemeVariant>(variant); }
  
  // Generate a new color scheme from source color
  ColorScheme generate_scheme(uint32_t source_color, bool is_dark, float contrast_level, SchemeVariant variant);
  
  // Generate scheme using stored member variables
  ColorScheme generate_scheme();
  
  // Get current scheme
  const ColorScheme& get_current_scheme() const { return this->current_scheme_; }
  
  // Callback registration
  void add_on_scheme_generated_callback(std::function<void(const ColorScheme&)> callback) {
    this->scheme_generated_callbacks_.push_back(callback);
  }
  
  // Helper to convert ARGB to RGB565 (for LVGL displays)
  static uint16_t argb_to_rgb565(Argb argb);
  
  // Helper to convert ARGB to hex string
  static std::string argb_to_hex(Argb argb);

  // Helper to convert ARGB to esphome::Color
  static esphome::Color argb_to_color(Argb argb);
  
  // Extract RGB components
  static uint8_t get_red(Argb argb) { return (argb >> 16) & 0xFF; }
  static uint8_t get_green(Argb argb) { return (argb >> 8) & 0xFF; }
  static uint8_t get_blue(Argb argb) { return argb & 0xFF; }
  static uint8_t get_alpha(Argb argb) { return (argb >> 24) & 0xFF; }
  
 protected:
  uint32_t source_color_{0xFF2A9D8F};  // Default: Pacific Teal from current config
  bool is_dark_{false};
  float contrast_level_{0.0};
  SchemeVariant variant_{VARIANT_CONTENT};
  
  ColorScheme current_scheme_{};
  std::vector<std::function<void(const ColorScheme&)>> scheme_generated_callbacks_;
  
  // Internal: Generate scheme using Material Color Utilities C++ library
  void generate_scheme_internal_();
  
  // Internal: Call all registered callbacks
  void notify_scheme_generated_();
};

// Action to generate a new scheme
template<typename... Ts>
class GenerateSchemeAction : public Action<Ts...> {
 public:
  explicit GenerateSchemeAction(MaterialThemeComponent *parent) : parent_(parent) {}
  
  TEMPLATABLE_VALUE(uint32_t, source_color)
  TEMPLATABLE_VALUE(bool, is_dark)
  TEMPLATABLE_VALUE(float, contrast_level)
  TEMPLATABLE_VALUE(SchemeVariant, variant)
  
  void play(Ts... x) override {
    uint32_t source = this->source_color_.value_or(x..., this->parent_->get_current_scheme().primary);
    bool dark = this->is_dark_.value_or(x..., false);
    float contrast = this->contrast_level_.value_or(x..., 0.0);
    SchemeVariant var = this->variant_.value_or(x..., VARIANT_TONAL_SPOT);
    
    ESP_LOGD(TAG, "Generating color scheme: source=0x%08X, dark=%d, contrast=%.2f, variant=%d",
             source, dark, contrast, var);
    
    auto scheme = this->parent_->generate_scheme(source, dark, contrast, var);
    ESP_LOGI(TAG, "Generated scheme - Primary: #%s, Surface: #%s, OnSurface: #%s",
             MaterialThemeComponent::argb_to_hex(scheme.primary).c_str(),
             MaterialThemeComponent::argb_to_hex(scheme.surface).c_str(),
             MaterialThemeComponent::argb_to_hex(scheme.on_surface).c_str());
  }
  
 protected:
  MaterialThemeComponent *parent_;
};

}  // namespace material_theme
}  // namespace esphome
