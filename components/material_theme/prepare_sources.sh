#!/bin/bash
# Prepare Material Color Utilities C++ sources for ESPHome compilation
# ESPHome/PlatformIO only compiles .cpp files, not .cc files
# This script renames .cc files to .cpp so they get automatically compiled

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPP_DIR="$SCRIPT_DIR/cpp"

echo "Preparing Material Color Utilities sources for ESPHome..."

if [ ! -d "$CPP_DIR" ]; then
    echo "Error: cpp/ directory not found. Run download_material_cpp.sh first."
    exit 1
fi

# Count how many .cc files exist
CC_COUNT=$(find "$CPP_DIR" -name "*.cc" 2>/dev/null | wc -l)

if [ "$CC_COUNT" -eq 0 ]; then
    echo "No .cc files found - sources may already be prepared or not downloaded."
    exit 0
fi

echo "Found $CC_COUNT .cc files to rename to .cpp"

# Rename all .cc files to .cpp
find "$CPP_DIR" -name "*.cc" -type f | while read -r file; do
    new_file="${file%.cc}.cpp"
    echo "  Renaming: $(basename "$file") -> $(basename "$new_file")"
    mv "$file" "$new_file"
done

echo ""
echo "✓ Sources prepared successfully!"
echo "  All .cc files renamed to .cpp"
echo "  ESPHome will now automatically compile them"
