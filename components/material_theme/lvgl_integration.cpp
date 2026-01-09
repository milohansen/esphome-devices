/*
 * LVGL Integration Implementation
 */

#include "lvgl_integration.h"

#ifdef USE_LVGL

#include "esphome/core/log.h"

namespace esphome
{
  namespace material_theme
  {

    static const char *const TAG_LVGL = "material_theme.lvgl";

    void LVGLThemeApplicator::apply_scheme_to_lvgl(lvgl::LvglComponent *lvgl_component,
                                                   const ColorScheme &scheme, bool is_dark)
    {
      if (!lvgl_component)
      {
        ESP_LOGW(TAG_LVGL, "LVGL component is null, cannot apply scheme");
        return;
      }

      ESP_LOGI(TAG_LVGL, "Applying Material scheme to LVGL theme");

      // Execute in LVGL context - capture scheme by value
      ColorScheme scheme_copy = scheme;
      bool is_dark_copy = is_dark;

      // lvgl_component->add_on_idle_callback([scheme_copy, is_dark_copy](uint32_t idle_time) {
      lv_disp_t *disp = lv_disp_get_default();
      if (!disp)
      {
        ESP_LOGW(TAG_LVGL, "No default display found");
        return;
      }

      // Create new Material theme (LVGL v8)
      lv_theme_t *theme = lv_theme_default_init(
          disp,
          argb_to_lv_color(scheme_copy.primary),
          argb_to_lv_color(scheme_copy.secondary),
          is_dark_copy,
          LV_FONT_DEFAULT);

      lv_disp_set_theme(disp, theme);

      ESP_LOGI(TAG_LVGL, "Material theme applied to display");
      // });
    }

    void LVGLThemeApplicator::apply_to_widget(lv_obj_t *widget, const ColorScheme &scheme, bool is_dark)
    {
      if (!widget)
      {
        return;
      }

      // Apply Material Design styles
      lv_obj_set_style_bg_color(widget, argb_to_lv_color(scheme.surface), LV_PART_MAIN);
      lv_obj_set_style_text_color(widget, argb_to_lv_color(scheme.on_surface), LV_PART_MAIN);
      lv_obj_set_style_border_color(widget, argb_to_lv_color(scheme.outline), LV_PART_MAIN);

      // Apply Material Design border radius (8dp standard)
      lv_obj_set_style_radius(widget, 8, LV_PART_MAIN);
    }

    void LVGLThemeApplicator::set_style_color(lv_obj_t *widget, lv_style_selector_t selector,
                                              lv_part_t part, Argb color)
    {
      if (!widget)
      {
        return;
      }

      lv_color_t lv_color = argb_to_lv_color(color);
      lv_obj_set_style_bg_color(widget, lv_color, selector | part);
    }

    lv_color_t LVGLThemeApplicator::argb_to_lv_color(Argb argb)
    {
      uint8_t r = MaterialThemeComponent::get_red(argb);
      uint8_t g = MaterialThemeComponent::get_green(argb);
      uint8_t b = MaterialThemeComponent::get_blue(argb);

      return lv_color_make(r, g, b);
    }

    void LVGLThemeApplicator::apply_elevation(lv_obj_t *widget, int elevation)
    {
      if (!widget)
      {
        return;
      }

      ESP_LOGV(TAG_LVGL, "Applying elevation %d to widget", elevation);

      // Material Design elevation shadows
      // Elevation levels: 1, 2, 3, 4, 6, 8, 9, 12, 16, 24
      lv_opa_t shadow_opa;
      lv_coord_t shadow_width;
      lv_coord_t shadow_offset_y;

      switch (elevation)
      {
      case 1:
        shadow_opa = LV_OPA_20;
        shadow_width = 3;
        shadow_offset_y = 1;
        break;
      case 2:
        shadow_opa = LV_OPA_30;
        shadow_width = 6;
        shadow_offset_y = 2;
        break;
      case 3:
        shadow_opa = LV_OPA_30;
        shadow_width = 8;
        shadow_offset_y = 3;
        break;
      case 4:
        shadow_opa = LV_OPA_40;
        shadow_width = 10;
        shadow_offset_y = 4;
        break;
      case 6:
        shadow_opa = LV_OPA_40;
        shadow_width = 14;
        shadow_offset_y = 6;
        break;
      case 8:
        shadow_opa = LV_OPA_50;
        shadow_width = 18;
        shadow_offset_y = 8;
        break;
      default:
        shadow_opa = LV_OPA_20;
        shadow_width = 3;
        shadow_offset_y = elevation / 2;
        break;
      }

      lv_obj_set_style_shadow_width(widget, shadow_width, LV_PART_MAIN);
      lv_obj_set_style_shadow_opa(widget, shadow_opa, LV_PART_MAIN);
      lv_obj_set_style_shadow_ofs_y(widget, shadow_offset_y, LV_PART_MAIN);
      lv_obj_set_style_shadow_color(widget, lv_color_black(), LV_PART_MAIN);
    }

    Argb LVGLThemeApplicator::get_text_color_for_background(Argb background, const ColorScheme &scheme)
    {
      // Calculate relative luminance (simple approximation)
      uint8_t r = MaterialThemeComponent::get_red(background);
      uint8_t g = MaterialThemeComponent::get_green(background);
      uint8_t b = MaterialThemeComponent::get_blue(background);

      // Relative luminance formula (simplified)
      float luminance = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;

      // Return light or dark text based on background luminance
      return (luminance > 0.5f) ? scheme.on_surface : scheme.inverse_on_surface;
    }

  } // namespace material_theme
} // namespace esphome

#endif // USE_LVGL
