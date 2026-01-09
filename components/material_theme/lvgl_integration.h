/*
 * LVGL Integration for Material Theme
 * 
 * Provides functions to apply Material Design color schemes to LVGL widgets
 */

#pragma once

#include "material_theme.h"

#ifdef USE_LVGL
#include "esphome/components/lvgl/lvgl_esphome.h"

namespace esphome {
namespace material_theme {

class LVGLThemeApplicator {
 public:
  /**
   * Apply Material color scheme to LVGL theme
   * This creates a new LVGL theme with Material Design colors
   */
  static void apply_scheme_to_lvgl(lvgl::LvglComponent *lvgl_component, const ColorScheme &scheme, bool is_dark);
  
  /**
   * Update specific widget styles with Material colors
   */
  static void apply_to_widget(lv_obj_t *widget, const ColorScheme &scheme, bool is_dark);
  
  /**
   * Apply Material color to specific style property
   */
  static void set_style_color(lv_obj_t *widget, lv_style_selector_t selector,
                              lv_part_t part, Argb color);
  
  /**
   * Convert ARGB to lv_color_t (RGB565)
   */
  static lv_color_t argb_to_lv_color(Argb argb);
  
  /**
   * Create Material Design elevation shadow
   */
  static void apply_elevation(lv_obj_t *widget, int elevation);
  
 private:
  /**
   * Get appropriate text color based on background luminance
   * Ensures WCAG contrast requirements
   */
  static Argb get_text_color_for_background(Argb background, const ColorScheme &scheme);
};

}  // namespace material_theme
}  // namespace esphome

#endif  // USE_LVGL
