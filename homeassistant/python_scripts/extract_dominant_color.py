"""
Extract dominant color from images using Material Color Utilities.

This script extracts the most suitable color from an image for Material Design 3
theming. It uses the same quantization and scoring algorithms as Android 12+.

Installation in Home Assistant:
  pip install material-color-utilities pillow

Usage:
  service: python_script.extract_dominant_color
  data:
    image_path: "wallpapers/sunset.jpg"  # Relative to /config/www/
    target_entity: "input_text.theme_color"
"""

try:
    from materialyoucolor.quantize import QuantizeCelebi
    from materialyoucolor.score.score import Score
    from PIL import Image
    import os
    
    METHOD = "material"
except ImportError:
    # Fallback to ColorThief if material-color-utilities not available
    try:
        from colorthief import ColorThief
        from PIL import Image
        METHOD = "colorthief"
    except ImportError:
        METHOD = "none"
        logger.error("Neither material-color-utilities nor colorthief installed!")

def extract_with_material(image_path):
    """Extract color using Material Color Utilities (recommended)."""
    img = Image.open(image_path)
    
    # Resize for performance - 512x512 is good balance
    img.thumbnail((512, 512))
    
    # Convert to RGB if needed
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Get pixel data
    pixels = list(img.getdata())
    
    # Convert to ARGB format (Material Color Utilities format)
    argb_pixels = [(255 << 24) | (r << 16) | (g << 8) | b for r, g, b in pixels]
    
    # Extract up to 128 dominant colors using Celebi quantization
    colors = QuantizeCelebi(argb_pixels, 128)
    
    # Score and rank colors (filters unsuitable colors, prioritizes chroma)
    ranked = Score.score(colors)
    
    if ranked:
        return ranked[0]  # Return top scored color
    
    # Fallback to first quantized color
    return colors[0] if colors else 0xFF6366F1

def extract_with_colorthief(image_path):
    """Extract color using ColorThief (fallback)."""
    ct = ColorThief(image_path)
    rgb = ct.get_color(quality=1)  # Get dominant color
    
    # Convert RGB to ARGB
    return (255 << 24) | (rgb[0] << 16) | (rgb[1] << 8) | rgb[2]

def main():
    # Get parameters
    image_path = data.get('image_path')
    image_url = data.get('image_url')
    target_entity = data.get('target_entity', 'input_text.theme_color')
    
    if not image_path and not image_url:
        logger.error("No image_path or image_url provided")
        return
    
    try:
        # Determine full path
        if image_url:
            # Download from URL (e.g., album art)
            import urllib.request
            temp_path = "/tmp/theme_image.jpg"
            urllib.request.urlretrieve(image_url, temp_path)
            full_path = temp_path
        else:
            # Local file from www directory
            full_path = f"/config/www/{image_path}"
        
        # Check file exists
        if not os.path.exists(full_path):
            logger.error(f"Image not found: {full_path}")
            return
        
        # Extract color based on available method
        if METHOD == "material":
            color = extract_with_material(full_path)
            logger.info(f"Extracted color using Material Color Utilities: #{color:08X}")
        elif METHOD == "colorthief":
            color = extract_with_colorthief(full_path)
            logger.info(f"Extracted color using ColorThief: #{color:08X}")
        else:
            logger.error("No color extraction library available")
            return
        
        # Convert to hex string (ARGB format)
        hex_color = f"{color:08X}"
        
        # Update Home Assistant entity
        hass.services.call('input_text', 'set_value', {
            'entity_id': target_entity,
            'value': hex_color
        })
        
        logger.info(f"Updated {target_entity} with color: #{hex_color}")
        
        # Cleanup temp file if downloaded
        if image_url and os.path.exists("/tmp/theme_image.jpg"):
            os.remove("/tmp/theme_image.jpg")
            
    except Exception as e:
        logger.error(f"Error extracting color: {str(e)}")
        import traceback
        logger.error(traceback.format_exc())

# Run the script
main()
