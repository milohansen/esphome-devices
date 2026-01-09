#!/bin/bash
#
# Download Material Color Utilities C++ library
# This script fetches the required source files from the GitHub repository
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPP_DIR="$SCRIPT_DIR/components/material_theme/cpp"

echo "Material Color Utilities C++ Library Downloader"
echo "================================================"
echo ""
echo "Target directory: $CPP_DIR"
echo ""

# Create directory structure
mkdir -p "$CPP_DIR/cam"
mkdir -p "$CPP_DIR/scheme"
mkdir -p "$CPP_DIR/dynamiccolor"
mkdir -p "$CPP_DIR/palettes"
mkdir -p "$CPP_DIR/utils"
mkdir -p "$CPP_DIR/contrast"
mkdir -p "$CPP_DIR/quantize"
mkdir -p "$CPP_DIR/dislike"
mkdir -p "$CPP_DIR/blend"
mkdir -p "$CPP_DIR/temperature"
mkdir -p "$CPP_DIR/score"

REPO_URL="https://raw.githubusercontent.com/material-foundation/material-color-utilities/main"

echo "Downloading C++ source files..."
echo ""

# Core files - HCT color space
download_file() {
    local file="$1"
    local target="$2"
    echo "  - $file"
    curl -sS "$REPO_URL/$file" -o "$target" || {
        echo "    WARNING: Failed to download $file"
        return 1
    }
}

# HCT (Hue, Chroma, Tone) color space
download_file "cpp/cam/hct.h" "$CPP_DIR/cam/hct.h"
download_file "cpp/cam/hct.cc" "$CPP_DIR/cam/hct.cc"
download_file "cpp/cam/cam.h" "$CPP_DIR/cam/cam.h"
download_file "cpp/cam/cam.cc" "$CPP_DIR/cam/cam.cc"
download_file "cpp/cam/viewing_conditions.h" "$CPP_DIR/cam/viewing_conditions.h"
download_file "cpp/cam/viewing_conditions.cc" "$CPP_DIR/cam/viewing_conditions.cc"

# Tonal palettes
download_file "cpp/palettes/tones.h" "$CPP_DIR/palettes/tones.h"
download_file "cpp/palettes/tones.cc" "$CPP_DIR/palettes/tones.cc"

# Scheme implementations
download_file "cpp/scheme/scheme_tonal_spot.h" "$CPP_DIR/scheme/scheme_tonal_spot.h"
download_file "cpp/scheme/scheme_tonal_spot.cc" "$CPP_DIR/scheme/scheme_tonal_spot.cc"
download_file "cpp/scheme/scheme_vibrant.h" "$CPP_DIR/scheme/scheme_vibrant.h"
download_file "cpp/scheme/scheme_vibrant.cc" "$CPP_DIR/scheme/scheme_vibrant.cc"
download_file "cpp/scheme/scheme_expressive.h" "$CPP_DIR/scheme/scheme_expressive.h"
download_file "cpp/scheme/scheme_expressive.cc" "$CPP_DIR/scheme/scheme_expressive.cc"
download_file "cpp/scheme/scheme_content.h" "$CPP_DIR/scheme/scheme_content.h"
download_file "cpp/scheme/scheme_content.cc" "$CPP_DIR/scheme/scheme_content.cc"
download_file "cpp/scheme/scheme_monochrome.h" "$CPP_DIR/scheme/scheme_monochrome.h"
download_file "cpp/scheme/scheme_monochrome.cc" "$CPP_DIR/scheme/scheme_monochrome.cc"
download_file "cpp/scheme/scheme_neutral.h" "$CPP_DIR/scheme/scheme_neutral.h"
download_file "cpp/scheme/scheme_neutral.cc" "$CPP_DIR/scheme/scheme_neutral.cc"

# Dynamic color system
download_file "cpp/dynamiccolor/dynamic_scheme.h" "$CPP_DIR/dynamiccolor/dynamic_scheme.h"
download_file "cpp/dynamiccolor/dynamic_scheme.cc" "$CPP_DIR/dynamiccolor/dynamic_scheme.cc"
download_file "cpp/dynamiccolor/dynamic_color.h" "$CPP_DIR/dynamiccolor/dynamic_color.h"
download_file "cpp/dynamiccolor/dynamic_color.cc" "$CPP_DIR/dynamiccolor/dynamic_color.cc"
download_file "cpp/dynamiccolor/material_dynamic_colors.h" "$CPP_DIR/dynamiccolor/material_dynamic_colors.h"
download_file "cpp/dynamiccolor/material_dynamic_colors.cc" "$CPP_DIR/dynamiccolor/material_dynamic_colors.cc"
download_file "cpp/dynamiccolor/variant.h" "$CPP_DIR/dynamiccolor/variant.h"
download_file "cpp/dynamiccolor/contrast_curve.h" "$CPP_DIR/dynamiccolor/contrast_curve.h"
# Note: contrast_curve.cc does not exist - it's header-only
download_file "cpp/dynamiccolor/tone_delta_pair.h" "$CPP_DIR/dynamiccolor/tone_delta_pair.h"

# Dislike (color filtering)
download_file "cpp/dislike/dislike.h" "$CPP_DIR/dislike/dislike.h"
download_file "cpp/dislike/dislike.cc" "$CPP_DIR/dislike/dislike.cc"

# Temperature (warm/cool color temperature)
download_file "cpp/temperature/temperature_cache.h" "$CPP_DIR/temperature/temperature_cache.h"
download_file "cpp/temperature/temperature_cache.cc" "$CPP_DIR/temperature/temperature_cache.cc"

# Utilities
download_file "cpp/utils/utils.h" "$CPP_DIR/utils/utils.h"
download_file "cpp/utils/utils.cc" "$CPP_DIR/utils/utils.cc"

# Contrast calculations
download_file "cpp/contrast/contrast.h" "$CPP_DIR/contrast/contrast.h"
download_file "cpp/contrast/contrast.cc" "$CPP_DIR/contrast/contrast.cc"

# Color quantization (for extracting colors from images)
download_file "cpp/quantize/celebi.h" "$CPP_DIR/quantize/celebi.h"
download_file "cpp/quantize/celebi.cc" "$CPP_DIR/quantize/celebi.cc"
download_file "cpp/quantize/lab.h" "$CPP_DIR/quantize/lab.h"
download_file "cpp/quantize/lab.cc" "$CPP_DIR/quantize/lab.cc"
download_file "cpp/quantize/wsmeans.h" "$CPP_DIR/quantize/wsmeans.h"
download_file "cpp/quantize/wsmeans.cc" "$CPP_DIR/quantize/wsmeans.cc"
download_file "cpp/quantize/wu.h" "$CPP_DIR/quantize/wu.h"
download_file "cpp/quantize/wu.cc" "$CPP_DIR/quantize/wu.cc"

echo ""
echo "Download complete!"
echo ""
echo "Next steps:"
echo "  1. Review the downloaded files in $CPP_DIR"
echo "  2. Update material_theme.cpp to use the real Material Color Utilities API"
echo "  3. Test the component with: esphome config ./Guition_P4_7.0.yaml"
echo ""
