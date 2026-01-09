#!/bin/bash
# Fix all #include statements in mcu_*.cpp and mcu_*.h files
# Replace cpp/subdir/file.h with mcu_cpp_subdir_file.h

cd "$(dirname "$0")"

for file in mcu_*.cpp mcu_*.h; do
  [ -f "$file" ] || continue
  
  # Fix includes: cpp/cam/hct.h -> mcu_cpp_cam_hct.h
  sed -i 's|"cpp/|"mcu_cpp_|g' "$file"
  sed -i 's|/|_|g; s|\.h"|.h"|g' "$file" 
  
  echo "Fixed includes in $file"
done

echo ""
echo "✓ All includes fixed!"
