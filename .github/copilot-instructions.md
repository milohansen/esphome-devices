# ESPHome Voice Assistant Display Project

## Project Overview
This is a modular ESPHome configuration for a Guition P4 7.0" display with ESP32-P4 hardware, implementing a Home Assistant voice assistant with LVGL UI, wake word detection, multimedia capabilities, and an automated slideshow system powered by a REST API backend.

## Architecture

### Modular Package System
Inspired by [jtenniswood/esphome-lvgl](https://github.com/jtenniswood/esphome-lvgl), this project uses ESPHome's `packages` system with separation of concerns:

**Active Structure** (packages loaded from [Guition_P4_7.0.yaml](Guition_P4_7.0.yaml)):
- **`core/`**: Application logic and ESPHome configuration
  - `esphome.yaml` - Base ESPHome config, OTA, WiFi, time, slideshow queue fetch interval
  - `code.yaml` - Scripts (wake word, phase management, slideshow queue fetch, auto-advance)
  - `input.yaml` - User input controls (image duration, display index)
  - `sensors.yaml` - Internal template sensors for image URLs (API-populated)
  - `visuals.yaml` - Images, fonts, colors
  - `assistant.yaml` - Voice assistant callbacks and configuration
- **`hw/`**: Hardware-specific definitions
  - `peripherals.yaml` - I2C buses, audio DAC/ADC
  - `audio.yaml` - Microphone, speaker, mixer architecture
  - `screen.yaml` - Touchscreen configuration
  - `lvgl.yaml` - LVGL widget definitions
- **`components/material_theme/`**: Material Design 3 dynamic theming (NEW)
  - Custom ESPHome component integrating Material Color Utilities C++ library
  - Runtime color scheme generation from source colors
  - LVGL integration for applying Material themes to UI
  - Supports 6 scheme variants (TonalSpot, Vibrant, Expressive, Content, Monochrome, Neutral)

**Legacy** (moved to `legacy/` folder, kept for reference only):
- `legacy/device/` - Old monolithic device configs (replaced by `core/` + `hw/`)
- `legacy/addon/`, `legacy/assets/`, `legacy/theme/` - Functionality now in `core/` and `hw/`
- `legacy/localtest/` - Alternative test configurations

### Key Design Patterns

#### 1. Substitution-Driven Configuration
All main configs start with extensive `substitutions:` blocks defining dimensions, paths, entity IDs, and phase constants. These propagate through included files:
```yaml
substitutions:
  screenwidth: "1024"
  screenheight: "600"
  voice_assist_idle_phase_id: "1"
  slideshow_api_base: !secret slideshow_api_base
  slideshow_device_id: "kitchen-display"
```

#### 2. LVGL Widget Templates
[hw/lvgl.yaml](hw/lvgl.yaml) uses YAML anchors for reusable widget configurations:
```yaml
.image_container_template: &image_container_template
  height: ${container_height}
  width: SIZE_CONTENT
  snappable: true
```
Reference with `<<: *image_container_template` in widget definitions. Note: `legacy/device/lvgl.yaml` is deprecated.

#### 3. Voice Assistant State Machine
Voice assistant phases are tracked via numeric IDs (substitutions like `voice_assist_listening_phase_id`). The [core/code.yaml](core/code.yaml) script `set_phase` manages state transitions, and [core/voice.yaml](core/voice.yaml) handles callbacks (`on_listening`, `on_stt_vad_start`, etc.).

#### 4. C++ Helper Integration
[va_helpers.h](va_helpers.h) provides timer management utilities:
- `TimerState` enum (RUNNING, PAUSED, CANCELLED, RINGING, COMPLETED)
- `TimerController` struct for managing multiple timers
- Lambda-safe functions to prevent stack crashes

#### 5. Slideshow API Integration
Automatic image slideshow system fetching content from REST API backend:
- **Device registration**: On WiFi connect, registers with `POST /api/devices`
- **Queue management**: Fetches shuffled image queue via `GET /api/devices/{deviceId}/slideshow`
- **6-slot buffer**: Manages prev/current/next/lookahead images with 4 `online_image` components
- **Auto-advance**: Timer-based progression using `image_duration_minutes` setting
- **URL construction**: Builds image URLs as `{base}/api/devices/{deviceId}/images/{imageId}`
- **Queue refresh**: Every 30 minutes or when local queue exhausted
- **Debug UI**: Status panel shows current index, queue position, loaded slots, fetch time

## Critical Workflows

### Build & Deploy
```bash
# Validate configuration
esphome config ./Guition_P4_7.0.yaml

# Compile and upload
esphome run ./Guition_P4_7.0.yaml

# View logs
esphome logs ./Guition_P4_7.0.yaml
```

### Adding Hardware Components
1. Define I2C buses, pins, and base hardware in `hw/peripherals.yaml`
2. Add component configuration to appropriate `hw/*.yaml`:
   - `hw/audio.yaml` - Audio devices (DAC, ADC, microphone, speakers)
   - `hw/screen.yaml` - Display and touchscreen
   - `hw/lvgl.yaml` - LVGL widgets and UI
3. Add `!include` reference in [Guition_P4_7.0.yaml](Guition_P4_7.0.yaml) `packages:` section (already done for active modules)

### Modifying LVGL UI
1. Edit widget definitions in `hw/lvgl.yaml`
2. Use existing templates with `<<: *template_name`
3. Widget IDs must be unique and referenced in lambdas via `id(widget_name)`
4. Image sources: Use `online_image:` for remote URLs, `image:` for local files

### Voice Assistant Phase Changes
1. Define phase ID in substitutions (e.g., `voice_assist_custom_phase_id: "15"`)
2. Add phase logic to `script.set_phase` in [core/code.yaml](core/code.yaml)
3. Handle in voice callbacks in [core/assistant.yaml](core/assistant.yaml)
4. Update UI widgets to respond to phase changes via `on_value` triggers

### Managing Slideshow Integration
1. **API Configuration**: Set `slideshow_api_base` in [secrets.yaml](secrets.yaml), `slideshow_device_id` in substitutions
2. **Queue Fetch**: `fetch_slideshow_queue` script in [core/code.yaml](core/code.yaml) parses JSON and populates URL sensors
3. **Image Loading**: Existing `update_online_images` script handles downloads with retry logic
4. **Auto-Advance**: `slideshow_timer` script executes `advance_slideshow` after delay
5. **Queue Cycling**: As images viewed, new ones loaded from queue; fetches new queue when exhausted
6. **Debug Status**: LVGL status panel in [hw/lvgl.yaml](hw/lvgl.yaml) shows live slideshow state

## Project Conventions

### File Organization
Main entrypoint: [Guition_P4_7.0.yaml](Guition_P4_7.0.yaml) defines substitutions and imports packages.

- **Sensors** → `core/sensors.yaml` (internal template sensors for image URLs, weather integration)
- **Hardware definitions** → `hw/peripherals.yaml` (I2C buses, pins)
- **Scripts & automation** → `core/code.yaml` (wake word, phase management, slideshow API, queue fetch)
- **Voice assistant** → `core/assistant.yaml` (callbacks, pipeline config)
- **Visual assets** → `core/visuals.yaml` (images, fonts, colors)
- **LVGL UI** → `hw/lvgl.yaml` (widgets, layouts, templates, debug status panel)
- **Secrets** → `secrets.yaml` (WiFi credentials, API URLs) - NEVER commit this file
- **NEVER edit `legacy/` directory** - deprecated files kept for reference only

### YAML Formatting
- Use 2-space indentation consistently
- Commented-out blocks preserve experimentation history (don't delete them)
- Substitutions use snake_case: `voice_assist_idle_phase_id`
- Widget IDs use snake_case: `clock_background`, `main_display`

### C++ Formatting (va_helpers.h and lambdas)
- **Always use braces** for all control structures (if/else/for/while), even single statements
- Use K&R style: opening brace on same line, closing brace on new line
- Example:
  ```cpp
  if (condition)
  {
    statement;
  }
  ```
- Null checks always first with early returns in braces
- Namespace: Use `my_va::` for all helper functions

### ESP32-P4 Specifics
- **Framework**: esp-idf (NOT Arduino)
- **PSRAM**: XIP mode enabled (`CONFIG_SPIRAM_XIP_FROM_PSRAM: "y"`) to prevent OTA flashing
- **LDO Channel 3**: Required at 2.5V for DSI PHY (`esp_ldo:`)
- **esp32_hosted**: ESP32-C6 variant for WiFi offload (pins defined in config)

### Audio System
- **Mixer architecture**: `mixing_speaker` combines `announcement_mixer_input` and `media_mixer_input`
- **Resamplers**: Convert all sources to 48kHz before mixing
- **ES8311 DAC** and **ES7210 ADC** on I2C bus A
- Enable speaker with GPIO11 switch before playback

### Image Handling
- **Format**: RGB565 with little-endian byte order for LVGL
- **Online images**: Use `online_image:` component, set `update_interval: never` for manual control
- **Resize**: Always specify `resize: ${screenwidth}x${screenheight}` to match display
- **Slideshow system**: 4 `online_image` components (previous, current, next, lookahead) buffering 6 display slots
- **Image URLs**: Populated by `fetch_slideshow_queue` from API, stored in template text sensors
- **Loading state**: Tracked via `image_slot_success`, `image_slot_loading`, `image_slot_retry_count` globals

## Dependencies & Integration

### Home Assistant
- **Weather entity**: Required (`weather_entity` substitution)
- **Media player**: `external_media_player` defined in [hw/audio.yaml](hw/audio.yaml)
- **Wake word**: Can use "On device" (micro_wake_word) or "In Home Assistant" mode
- **Slideshow control**: `image_duration_minutes` number entity controls display time

### Slideshow Backend API
- **Base URL**: Stored in `secrets.yaml` as `slideshow_api_base`
- **Device ID**: Set in substitutions (e.g., `kitchen-display`)
- **Endpoints used**:
  - `POST /api/devices` - Register device with dimensions/orientation
  - `GET /api/devices/{deviceId}/slideshow` - Fetch shuffled image queue
  - `GET /api/devices/{deviceId}/images/{imageId}` - Download specific image
- **Queue format**: JSON with array of `{imageId, filePath}` objects
- **Refresh interval**: 30 minutes via `interval:` component in [core/esphome.yaml](core/esphome.yaml)

### Wake Word Engines
Models defined in main config under `micro_wake_word:`. Only include needed models to conserve memory:
```yaml
micro_wake_word:
  models:
    - okay_nabu  # Default, low memory usage
```

### Fonts
- Font definitions in [core/fonts.yaml](core/fonts.yaml) and [assets/fonts.yaml](assets/fonts.yaml)
- Use Google Fonts glyphsets: `GF_Latin_Kernel` (minimal) or `GF_Latin_Core` (extended)
- Font family controlled by `font_family` substitution

## Material Theme Integration

### Runtime Dynamic Theming (NEW)
The project now includes `material_theme` custom component for Material Design 3 compliant color schemes:

**Setup**:
1. Run `./download_material_cpp.sh` to download Material Color Utilities C++ library
2. Add `material_theme:` configuration to main YAML
3. Register callback in `on_boot` to apply schemes to LVGL
4. Use `material_theme.generate_scheme` action to change colors at runtime

**Color Scheme Variants**:
- `TONAL_SPOT`: Balanced, default Material You (Android 12-13)
- `VIBRANT`: High chroma, energetic
- `EXPRESSIVE`: Dramatic, artistic
- `CONTENT`: Derived from image/wallpaper colors
- `MONOCHROME`: Grayscale minimal
- `NEUTRAL`: Subtle, professional

**Key Files**:
- `components/material_theme/` - Custom ESPHome component
- `components/material_theme/cpp/` - Material Color Utilities C++ library (downloaded separately)
- `MATERIAL_THEME_INTEGRATION.md` - Complete integration guide
- `examples/material_theme_example.yaml` - Usage examples

**Capabilities**:
- Generate Material Design 3 schemes from any source color (runtime, no rebuild)
- 24+ semantic color roles (primary, secondary, tertiary, surface, error, etc.)
- WCAG-compliant contrast ratios built-in
- LVGL integration for UI theming
- Future: Wallpaper color extraction, Home Assistant theme sync

## Common Pitfalls

1. **Forgetting substitution context**: Included files inherit substitutions from parent
2. **LVGL widget ID conflicts**: Each `id:` must be globally unique across all LVGL configs
3. **Image byte order**: Always use `byte_order: little_endian` for LVGL displays
4. **OTA flashing**: Requires `CONFIG_SPIRAM_XIP_FROM_PSRAM` or screen flashes
5. **Voice assistant not ready**: Check `api.connected` and `wifi.connected` before starting
6. **Timer crashes**: Use `va_helpers.h` functions in lambdas, never call timer methods directly
7. **Material theme not updating**: Ensure callbacks registered before first scheme generation (`on_boot` priority: -100)
8. **LVGL updates before init**: Always null-check LVGL widgets before calling `lv_*` functions in boot scripts
9. **Slideshow API errors**: Check `slideshow_api_base` secret exists and device registration succeeded
10. **Image loading hangs**: Verify `update_interval: never` on `online_image` components, use `update()` for manual trigger

## Testing & Alternative Configurations
The `legacy/localtest/` directory contains experimental/alternative package structures not currently in use:
- `core-lvgl.yaml`: Monolithic LVGL configuration (alternative to modular `core/` + `hw/`)
- `HW/hw-lvgl.yaml`: Hardware bundle with LVGL
- `clocks-lvgl.yaml`/`clocks-standard.yaml`: Clock display variants

**To use**: Uncomment different `packages:` includes in [Guition_P4_7.0.yaml](Guition_P4_7.0.yaml), but note that active development uses the modular `core/` + `hw/` structure.
