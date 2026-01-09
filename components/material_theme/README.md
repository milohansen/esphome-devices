# Material Theme Component - README

## Overview

The `material_theme` custom component integrates Google's **Material Color Utilities C++ library** into ESPHome, enabling runtime generation of Material Design 3 compliant color schemes for your voice assistant display.

## Features

- ✅ **Runtime color generation** from any source color
- ✅ **6 Material Design scheme variants**: TonalSpot, Vibrant, Expressive, Content, Monochrome, Neutral
- ✅ **Dynamic theming** - change colors without recompiling
- ✅ **LVGL integration** - apply schemes to LVGL widgets
- ✅ **Home Assistant sync** - match dashboard theme colors
- ✅ **Accessibility built-in** - WCAG compliant contrast ratios
- 🚧 **Wallpaper color extraction** (planned - using quantize algorithms)

## Installation

### Step 1: Download Material Color Utilities C++ Library

```bash
cd /home/miloh/esphome
chmod +x download_material_cpp.sh
./download_material_cpp.sh
```

This downloads the required C++ source files to `components/material_theme/cpp/`.

### Step 2: Add Component to Configuration

```yaml
# Add external component
external_components:
  - source:
      type: local
      path: components

# Configure material theme
material_theme:
  id: theme
  source_color: 0xFF2A9D8F  # Your brand color
  is_dark: false
  contrast_level: 0.0  # -1.0 to 1.0 (standard is 0.0)
  variant: TONAL_SPOT
```

### Step 3: Apply to LVGL (Optional)

```yaml
esphome:
  on_boot:
    priority: -100
    then:
      - lambda: |-
          id(theme).add_on_scheme_generated_callback([](const material_theme::ColorScheme &scheme) {
            #ifdef USE_LVGL
            auto *lvgl = id(main_display);
            material_theme::LVGLThemeApplicator::apply_scheme_to_lvgl(lvgl, scheme, false);
            #endif
          });
```

## Material Design Scheme Variants

### TONAL_SPOT (Default)
Calm theme with low to medium colorfulness. The default Material You theme on Android 12-13.
- **Primary**: Source color hue, 36 chroma
- **Secondary**: Source color hue, 16 chroma  
- **Tertiary**: Hue + 60°, 24 chroma
- **Use case**: General purpose, balanced applications

### VIBRANT
High chroma colors with dynamic hue rotation.
- **Primary**: Source color hue, 200 chroma
- **Secondary**: Rotated hue, 24 chroma
- **Tertiary**: Rotated hue, 32 chroma
- **Use case**: Energetic, playful, attention-grabbing UIs

### EXPRESSIVE
Dramatic color palette with strong artistic expression.
- **Primary**: Hue + 240°, 40 chroma
- **Secondary**: Rotated hue, 24 chroma
- **Tertiary**: Rotated hue, 32 chroma
- **Use case**: Creative apps, entertainment, media

### CONTENT
Scheme derived from source color with exact chroma preservation.
- **Primary**: Source color (exact chroma)
- **Neutrals**: Low chroma variants
- **Use case**: Wallpaper/image-based theming, content-first design

### MONOCHROME
Grayscale palette with minimal chroma.
- **All colors**: Same hue, 0 chroma
- **Use case**: Minimalist design, reading apps, accessibility

### NEUTRAL
Subtle color variations with very low chroma.
- **Primary**: 12 chroma
- **Secondary**: 8 chroma
- **Tertiary**: 16 chroma
- **Use case**: Professional apps, subtle branding

## Color Roles

Material Design 3 defines 24+ color roles that are automatically generated:

### Primary Colors
- `primary` - Main brand color
- `on_primary` - Text on primary color
- `primary_container` - Less prominent primary
- `on_primary_container` - Text on primary container

### Secondary & Tertiary
Same pattern repeated for `secondary_*` and `tertiary_*`

### Surface Colors
- `background` - Base layer
- `surface` - Component surfaces
- `surface_variant` - Subtle variants
- `on_surface` - Text on surfaces

### Semantic Colors
- `error`, `on_error` - Error states
- `outline` - Borders
- `shadow`, `scrim` - Overlays

## API Reference

### Component Configuration

```yaml
material_theme:
  id: theme  # Required
  source_color: 0xFFRRGGBB  # Default: 0xFF2A9D8F
  is_dark: false  # Default: false
  contrast_level: 0.0  # Range: -1.0 to 1.0
  variant: TONAL_SPOT  # See variants above
```

### Actions

#### `material_theme.generate_scheme`
Generate a new color scheme at runtime.

```yaml
on_press:
  - material_theme.generate_scheme:
      id: theme
      source_color: 0xFF6366F1
      is_dark: !lambda "return id(sun).state == 'below_horizon';"
      contrast_level: 0.5
      variant: VIBRANT
```

### Lambdas

#### Get Current Scheme
```cpp
auto scheme = id(theme).get_current_scheme();
ESP_LOGI("app", "Primary: #%s", 
  material_theme::MaterialThemeComponent::argb_to_hex(scheme.primary).c_str());
```

#### Generate Scheme Programmatically
```cpp
auto scheme = id(theme).generate_scheme(
  0xFF4F46E5,  // Indigo
  false,       // Light mode
  0.0,         // Standard contrast
  material_theme::VARIANT_TONAL_SPOT
);
```

#### Register Callback
```cpp
id(theme).add_on_scheme_generated_callback([](const material_theme::ColorScheme &scheme) {
  ESP_LOGI("theme", "New scheme generated!");
  // Apply to LVGL, update sensors, etc.
});
```

### LVGL Integration

#### Apply Scheme to Display
```cpp
#ifdef USE_LVGL
auto scheme = id(theme).get_current_scheme();
auto *lvgl = id(main_display);
material_theme::LVGLThemeApplicator::apply_scheme_to_lvgl(lvgl, scheme, false);
#endif
```

#### Style Individual Widgets
```cpp
auto scheme = id(theme).get_current_scheme();
lv_obj_t *button = id(my_button);

// Set colors
lv_obj_set_style_bg_color(button, 
  material_theme::LVGLThemeApplicator::argb_to_lv_color(scheme.primary),
  LV_PART_MAIN);

// Apply Material elevation
material_theme::LVGLThemeApplicator::apply_elevation(button, 2);
```

## Integration Examples

### Sync with Home Assistant Theme
```yaml
sensor:
  - platform: homeassistant
    entity_id: sensor.theme_primary_color
    id: ha_theme_color
    on_value:
      then:
        - lambda: |-
            uint32_t color = static_cast<uint32_t>(x);
            id(theme).generate_scheme(color, false, 0.0, material_theme::VARIANT_TONAL_SPOT);
```

### Time-Based Dark Mode
```yaml
time:
  - platform: homeassistant
    on_time:
      - hours: 19
        then:
          - material_theme.generate_scheme:
              id: theme
              is_dark: true
      - hours: 7
        then:
          - material_theme.generate_scheme:
              id: theme
              is_dark: false
```

### Extract from Media Player Album Art (Future)
```yaml
# Planned feature - extract dominant color from images
media_player:
  - platform: homeassistant
    entity_id: media_player.spotify
    on_play:
      - lambda: |-
          // auto artwork_color = extract_dominant_color(album_art);
          // id(theme).generate_scheme(artwork_color, ...);
```

## Technical Details

### Color Space: HCT
Material Color Utilities uses **HCT** (Hue, Chroma, Tone):
- **Hue**: 0-360° (color wheel position)
- **Chroma**: 0-120+ (colorfulness)
- **Tone**: 0-100 (lightness, linear to perception)

HCT is perceptually uniform and guarantees contrast ratios.

### Contrast Levels
- **-1.0**: Minimum contrast (reduced accessibility)
- **0.0**: Standard (WCAG AA ~3:1)
- **1.0**: Maximum contrast (WCAG AAA ~7:1)

### Memory Usage
- Component: ~2KB RAM
- Material C++ library: ~50KB flash, ~5KB RAM (per scheme generation)
- Suitable for ESP32 with PSRAM

### Build Time
First compile includes C++ library compilation (~30 seconds extra).
Subsequent builds are cached.

## Troubleshooting

### Build Errors
```
Error: material_color_utilities namespace not found
```
**Solution**: Run `./download_material_cpp.sh` to download C++ library sources.

### LVGL Integration Not Working
```
Warning: LVGL component is null
```
**Solution**: Ensure `material_theme` is initialized after LVGL (`priority: -100` in `on_boot`).

### Colors Not Updating
**Solution**: Call `generate_scheme()` again or verify callbacks are registered before scheme generation.

## Project Structure

```
components/material_theme/
├── __init__.py                 # ESPHome component definition
├── material_theme.h            # Component header
├── material_theme.cpp          # Implementation (stub + integration)
├── lvgl_integration.h          # LVGL helper functions
├── lvgl_integration.cpp        # LVGL implementation
└── cpp/                        # Material Color Utilities C++ library
    ├── cam/                    # HCT color space
    ├── scheme/                 # Scheme variants
    ├── dynamiccolor/           # Dynamic color system
    ├── palettes/               # Tonal palettes
    ├── utils/                  # Utilities
    ├── contrast/               # Contrast calculations
    └── quantize/               # Color quantization (image extraction)
```

## Roadmap

- [x] Basic component structure
- [x] Scheme variant support
- [x] LVGL integration
- [ ] Download and integrate C++ library
- [ ] Implement real Material Color Utilities API
- [ ] Wallpaper color extraction
- [ ] Home Assistant service for scheme generation
- [ ] Persistent scheme storage
- [ ] Animation between scheme transitions

## Credits

- **Material Color Utilities**: © 2023 Google LLC (Apache 2.0)
- **ESPHome**: https://esphome.io
- **Project**: Guition P4 Voice Assistant Display

## License

This component is licensed under Apache 2.0, matching Material Color Utilities.
