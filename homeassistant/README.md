# Home Assistant Integration for Material Theme

This folder contains files for integrating Material Design 3 color extraction with Home Assistant.

## Setup Instructions

### 1. Choose Your Method

**For Home Assistant Green / Home Assistant OS (Recommended):**

**Use the Simple Script** - No installation needed!
- Uses only built-in Python libraries (PIL/Pillow is already included)
- Copy `extract_dominant_color_simple.py` to `/config/python_scripts/`
- Rename to `extract_dominant_color.py`
- Works immediately, no pip installation required

**Alternative: Advanced SSH & Web Terminal Add-on**

If you want the full Material Color Utilities algorithm:

1. Install "Advanced SSH & Web Terminal" from Add-on Store
2. Disable "Protection mode" in the add-on settings
3. Start the add-on and open its terminal
4. Run:
   ```bash
   pip3 install material-color-utilities pillow
   ```
5. Use the full `extract_dominant_color.py` script

### 2. Copy Python Script to Home Assistant Green

**Using File Editor Add-on (Easiest):**

1. Install "File editor" add-on from Settings → Add-ons
2. Open File Editor
3. Navigate to `/config/` folder
4. Create `python_scripts` folder if it doesn't exist
5. Create new file: `python_scripts/extract_dominant_color.py`
6. Paste contents from `extract_dominant_color_simple.py`
7. Save

**Using Studio Code Server Add-on:**

1. Install "Studio Code Server" add-on
2. Open it and navigate to `/config/python_scripts/`
3. Create `extract_dominant_color.py` with the simple version contents

**Using Samba Share:**

1. Install "Samba share" add-on
2. Connect from your computer: `\\homeassistant.local\config`
3. Create `python_scripts` folder
4. Copy the file there

### 3. Enable Python Scripts

Add to `configuration.yaml`:

```yaml
python_script:
```

### 4. Add Configuration

Add the contents of `example_configuration.yaml` to your `configuration.yaml`:
- Input text entities for storing colors
- Optional: Input selects for theme variants and modes
- Optional: Template sensor for complete theme config

### 5. Add Automations

Add automations from `example_automations.yaml` to your `automations.yaml`:
- Choose the ones that fit your use case
- Customize entity IDs and paths as needed

### 6. Restart Home Assistant

```bash
# Restart Home Assistant to load changes
ha core restart
```

## Usage

### Manual Color Extraction

Use the Developer Tools → Services:

```yaml
service: python_script.extract_dominant_color
data:
  image_path: "wallpapers/sunset.jpg"  # Relative to /config/www/
  target_entity: input_text.theme_color
```

Or from URL:

```yaml
service: python_script.extract_dominant_color
data:
  image_url: "https://example.com/image.jpg"
  target_entity: input_text.theme_color
```

### Automatic Updates

The automations will automatically:
- Extract colors when wallpaper changes
- Update theme based on media player album art
- Sync with Home Assistant's theme
- Switch between light/dark modes

## File Structure

```
homeassistant/
├── python_scripts/
│   └── extract_dominant_color.py    # Color extraction script
├── example_configuration.yaml       # Home Assistant config snippets
├── example_automations.yaml         # Example automations
└── README.md                        # This file
```

## Image Storage

Store images in `/config/www/` directory in Home Assistant:

```
/config/www/
├── wallpapers/
│   ├── sunset.jpg
│   ├── ocean.jpg
│   └── forest.jpg
└── snapshots/
    └── camera_latest.jpg
```

Reference them in the script with relative paths:
- `wallpapers/sunset.jpg`
- `snapshots/camera_latest.jpg`

## ESPHome Integration

The ESPHome side is already configured in `core/material_theme.yaml`:
- Receives colors from `input_text.theme_color`
- Automatically generates Material Design 3 schemes
- Applies to LVGL display

## Troubleshooting

### Python script errors
Check Home Assistant logs: Settings → System → Logs

### Image not found
Verify path is relative to `/config/www/`

### No color extracted
Ensure PIL/Pillow can open the image format (JPEG, PNG supported)

### Library not found
- Use the simple version: `extract_dominant_color_simple.py` (no dependencies)
- Or reinstall via correct method (see Method A above)

### "pip: command not found"
**On Home Assistant Green/OS:** Use the simple script - no installation needed.

### "Failed building wheel" or "CMAKE_CXX_COMPILER not set"
**This is expected on Home Assistant Green/OS.** The material-color-utilities package requires C++ compilation which isn't available. Use `extract_dominant_color_simple.py` instead - it provides excellent color extraction without needing compiled packages.

If you need the full version:
- Only possible on Home Assistant Container/Core with build tools installed
- Requires: `apt-get install build-essential cmake python3-dev`
- Not recommended for Home Assistant Green/OS

## Advanced: Custom Color Scoring

Edit `extract_dominant_color.py` to customize color selection:

```python
# Prioritize warm colors
ranked = Score.score(colors, desired_chroma=0.5)

# Or manually filter
filtered_colors = [c for c in colors if (c & 0xFF0000) > 0x800000]  # More red
```

## Material Color Utilities Algorithm

The script uses the same algorithm as Android 12+ Material You:
1. **Quantization** (Celebi): Reduces image to 128 dominant colors
2. **Scoring**: Ranks colors by:
   - Chroma (colorfulness) - higher is better
   - Population (frequency in image)
   - Filters out colors that are too close to white/black/gray
3. **Returns**: Top-scored color suitable for Material Design 3 theming

## Testing

Test the integration:

1. Place test image: `/config/www/test.jpg`
2. Call service:
   ```yaml
   service: python_script.extract_dominant_color
   data:
     image_path: "test.jpg"
     target_entity: input_text.theme_color
   ```
3. Check `input_text.theme_color` value
4. ESPHome should regenerate theme automatically
