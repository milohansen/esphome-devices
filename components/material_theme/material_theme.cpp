/*
 * Material Theme Component Implementation
 * 
 * Integrates Google's Material Color Utilities C++ library
 * for runtime Material Design 3 color scheme generation
 */

#include "material_theme.h"
#include "esphome/core/log.h"

// Material Color Utilities C++ library includes
#include "cpp/cam/hct.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_vibrant.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_content.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/dynamiccolor/material_dynamic_colors.h"

namespace esphome {
namespace material_theme {

// Use the TAG from header file

void MaterialThemeComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Material Theme component...");
  
  // Generate initial scheme on setup
  this->generate_scheme_internal_();
  
  ESP_LOGCONFIG(TAG, "Material Theme setup complete");
}

void MaterialThemeComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Material Theme:");
  ESP_LOGCONFIG(TAG, "  Source Color: #%06X", this->source_color_ & 0xFFFFFF);
  ESP_LOGCONFIG(TAG, "  Dark Mode: %s", YESNO(this->is_dark_));
  ESP_LOGCONFIG(TAG, "  Contrast Level: %.2f", this->contrast_level_);
  ESP_LOGCONFIG(TAG, "  Variant: %d", this->variant_);
  ESP_LOGCONFIG(TAG, "  Current Scheme:");
  ESP_LOGCONFIG(TAG, "    Primary: #%s", argb_to_hex(this->current_scheme_.primary).c_str());
  ESP_LOGCONFIG(TAG, "    Secondary: #%s", argb_to_hex(this->current_scheme_.secondary).c_str());
  ESP_LOGCONFIG(TAG, "    Tertiary: #%s", argb_to_hex(this->current_scheme_.tertiary).c_str());
  ESP_LOGCONFIG(TAG, "    Surface: #%s", argb_to_hex(this->current_scheme_.surface).c_str());
  ESP_LOGCONFIG(TAG, "    Background: #%s", argb_to_hex(this->current_scheme_.background).c_str());
}

ColorScheme MaterialThemeComponent::generate_scheme(uint32_t source_color, bool is_dark, 
                                                     float contrast_level, SchemeVariant variant) {
  using namespace material_color_utilities;
  
  ESP_LOGD(TAG, "Generating Material Design 3 scheme from color #%06X (variant=%d, dark=%d, contrast=%.2f)", 
           source_color & 0xFFFFFF, variant, is_dark, contrast_level);
  
  // Create HCT color from source
  Hct source_hct(source_color);
  
  // Generate scheme based on variant
  DynamicScheme *scheme = nullptr;
  
  switch (variant) {
    case VARIANT_TONAL_SPOT:
      scheme = new SchemeTonalSpot(source_hct, is_dark, contrast_level);
      ESP_LOGV(TAG, "Using TonalSpot variant");
      break;
    case VARIANT_VIBRANT:
      scheme = new SchemeVibrant(source_hct, is_dark, contrast_level);
      ESP_LOGV(TAG, "Using Vibrant variant");
      break;
    case VARIANT_EXPRESSIVE:
      scheme = new SchemeExpressive(source_hct, is_dark, contrast_level);
      ESP_LOGV(TAG, "Using Expressive variant");
      break;
    case VARIANT_CONTENT:
      scheme = new SchemeContent(source_hct, is_dark, contrast_level);
      ESP_LOGV(TAG, "Using Content variant");
      break;
    case VARIANT_MONOCHROME:
      scheme = new SchemeMonochrome(source_hct, is_dark, contrast_level);
      ESP_LOGV(TAG, "Using Monochrome variant");
      break;
    case VARIANT_NEUTRAL:
      scheme = new SchemeNeutral(source_hct, is_dark, contrast_level);
      ESP_LOGV(TAG, "Using Neutral variant");
      break;
    default:
      scheme = new SchemeTonalSpot(source_hct, is_dark, contrast_level);
      ESP_LOGW(TAG, "Unknown variant %d, defaulting to TonalSpot", variant);
  }
  
  // Extract all Material Design color roles from the generated scheme
  ColorScheme result;
  
  // Primary colors
  result.primary = MaterialDynamicColors::Primary().GetArgb(*scheme);
  result.on_primary = MaterialDynamicColors::OnPrimary().GetArgb(*scheme);
  result.primary_container = MaterialDynamicColors::PrimaryContainer().GetArgb(*scheme);
  result.on_primary_container = MaterialDynamicColors::OnPrimaryContainer().GetArgb(*scheme);
  
  // Secondary colors
  result.secondary = MaterialDynamicColors::Secondary().GetArgb(*scheme);
  result.on_secondary = MaterialDynamicColors::OnSecondary().GetArgb(*scheme);
  result.secondary_container = MaterialDynamicColors::SecondaryContainer().GetArgb(*scheme);
  result.on_secondary_container = MaterialDynamicColors::OnSecondaryContainer().GetArgb(*scheme);
  
  // Tertiary colors
  result.tertiary = MaterialDynamicColors::Tertiary().GetArgb(*scheme);
  result.on_tertiary = MaterialDynamicColors::OnTertiary().GetArgb(*scheme);
  result.tertiary_container = MaterialDynamicColors::TertiaryContainer().GetArgb(*scheme);
  result.on_tertiary_container = MaterialDynamicColors::OnTertiaryContainer().GetArgb(*scheme);
  
  // Error colors
  result.error = MaterialDynamicColors::Error().GetArgb(*scheme);
  result.on_error = MaterialDynamicColors::OnError().GetArgb(*scheme);
  result.error_container = MaterialDynamicColors::ErrorContainer().GetArgb(*scheme);
  result.on_error_container = MaterialDynamicColors::OnErrorContainer().GetArgb(*scheme);
  
  // Background and surface colors
  result.background = MaterialDynamicColors::Background().GetArgb(*scheme);
  result.on_background = MaterialDynamicColors::OnBackground().GetArgb(*scheme);
  result.surface = MaterialDynamicColors::Surface().GetArgb(*scheme);
  result.on_surface = MaterialDynamicColors::OnSurface().GetArgb(*scheme);
  result.surface_variant = MaterialDynamicColors::SurfaceVariant().GetArgb(*scheme);
  result.on_surface_variant = MaterialDynamicColors::OnSurfaceVariant().GetArgb(*scheme);
  
  // Outline colors
  result.outline = MaterialDynamicColors::Outline().GetArgb(*scheme);
  result.outline_variant = MaterialDynamicColors::OutlineVariant().GetArgb(*scheme);
  
  // Shadow and scrim
  result.shadow = MaterialDynamicColors::Shadow().GetArgb(*scheme);
  result.scrim = MaterialDynamicColors::Scrim().GetArgb(*scheme);
  
  // Inverse colors
  result.inverse_surface = MaterialDynamicColors::InverseSurface().GetArgb(*scheme);
  result.inverse_on_surface = MaterialDynamicColors::InverseOnSurface().GetArgb(*scheme);
  result.inverse_primary = MaterialDynamicColors::InversePrimary().GetArgb(*scheme);
  
  // Surface container colors (Material Design 3)
  result.surface_container_lowest = MaterialDynamicColors::SurfaceContainerLowest().GetArgb(*scheme);
  result.surface_container_low = MaterialDynamicColors::SurfaceContainerLow().GetArgb(*scheme);
  result.surface_container = MaterialDynamicColors::SurfaceContainer().GetArgb(*scheme);
  result.surface_container_high = MaterialDynamicColors::SurfaceContainerHigh().GetArgb(*scheme);
  result.surface_container_highest = MaterialDynamicColors::SurfaceContainerHighest().GetArgb(*scheme);
  
  // Clean up
  delete scheme;
  
  // Store and notify
  this->current_scheme_ = result;
  this->notify_scheme_generated_();
  
  ESP_LOGI(TAG, "Generated Material Design 3 scheme:");
  ESP_LOGI(TAG, "  Primary: #%s", argb_to_hex(result.primary).c_str());
  ESP_LOGI(TAG, "  Secondary: #%s", argb_to_hex(result.secondary).c_str());
  ESP_LOGI(TAG, "  Tertiary: #%s", argb_to_hex(result.tertiary).c_str());
  ESP_LOGI(TAG, "  Surface: #%s", argb_to_hex(result.surface).c_str());
  ESP_LOGI(TAG, "  Background: #%s", argb_to_hex(result.background).c_str());
  
  return result;
}

ColorScheme MaterialThemeComponent::generate_scheme() {
  // Generate scheme using stored member variables
  return this->generate_scheme(
    this->source_color_,
    this->is_dark_,
    this->contrast_level_,
    this->variant_
  );
}

void MaterialThemeComponent::generate_scheme_internal_() {
  this->current_scheme_ = this->generate_scheme();
}

void MaterialThemeComponent::notify_scheme_generated_() {
  for (auto &callback : this->scheme_generated_callbacks_) {
    callback(this->current_scheme_);
  }
}

uint16_t MaterialThemeComponent::argb_to_rgb565(Argb argb) {
  uint8_t r = get_red(argb);
  uint8_t g = get_green(argb);
  uint8_t b = get_blue(argb);
  
  // Convert 8-bit RGB to 5-6-5
  uint16_t r5 = (r >> 3) & 0x1F;
  uint16_t g6 = (g >> 2) & 0x3F;
  uint16_t b5 = (b >> 3) & 0x1F;
  
  return (r5 << 11) | (g6 << 5) | b5;
}

std::string MaterialThemeComponent::argb_to_hex(Argb argb) {
  char buf[7];
  snprintf(buf, sizeof(buf), "%06X", argb & 0xFFFFFF);
  return std::string(buf);
}

// Material Color Utilities C++ library handles all color generation
// No stub helper functions needed

}  // namespace material_theme
}  // namespace esphome
