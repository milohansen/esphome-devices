"""
Extract dominant color from images - SIMPLE VERSION (No dependencies).

This version uses only Python standard library and PIL/Pillow (built into Home Assistant).
Perfect for Home Assistant Green/OS where C++ compilation isn't available.

It provides excellent color extraction using:
- Color quantization to reduce computation
- Saturation and brightness filtering
- Frequency-weighted scoring
- Suitable for Material Design 3 theming

No external packages required - works immediately on all Home Assistant installations!

Usage:
  service: python_script.extract_dominant_color_simple
  data:
    image_path: "wallpapers/sunset.jpg"
    target_entity: "input_text.theme_color"
"""

try:
    from PIL import Image
    import os
    import urllib.request
    from collections import Counter
    
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    logger.error("PIL/Pillow not available!")

def rgb_to_argb(r, g, b):
    """Convert RGB to ARGB integer."""
    return (255 << 24) | (r << 16) | (g << 8) | b

def get_saturation(r, g, b):
    """Calculate color saturation (0-1)."""
    max_val = max(r, g, b)
    min_val = min(r, g, b)
    if max_val == 0:
        return 0
    return (max_val - min_val) / max_val

def get_brightness(r, g, b):
    """Calculate perceived brightness (0-255)."""
    return (0.299 * r + 0.587 * g + 0.114 * b)

def is_suitable_color(r, g, b):
    """Filter out unsuitable colors (too dark, too light, too gray)."""
    brightness = get_brightness(r, g, b)
    saturation = get_saturation(r, g, b)
    
    # Too dark or too light
    if brightness < 30 or brightness > 225:
        return False
    
    # Too gray (low saturation)
    if saturation < 0.15:
        return False
    
    return True

def score_color(r, g, b, count):
    """Score a color for suitability as a theme color."""
    saturation = get_saturation(r, g, b)
    brightness = get_brightness(r, g, b)
    
    # Prefer high saturation
    saturation_score = saturation * 100
    
    # Prefer colors that appear more often
    frequency_score = count / 100.0
    
    # Prefer medium brightness
    brightness_normalized = brightness / 255.0
    brightness_score = 1.0 - abs(brightness_normalized - 0.5) * 2
    brightness_score *= 50
    
    return saturation_score + frequency_score + brightness_score

def extract_dominant_color(image_path):
    """Extract dominant color using simple k-means-like approach."""
    img = Image.open(image_path)
    
    # Resize for performance
    img.thumbnail((256, 256))
    
    # Convert to RGB
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Get all pixels
    pixels = list(img.getdata())
    
    # Quantize to 32 colors per channel (32768 total) for faster processing
    quantized = []
    for r, g, b in pixels:
        qr = (r >> 3) << 3  # Keep top 5 bits
        qg = (g >> 3) << 3
        qb = (b >> 3) << 3
        quantized.append((qr, qg, qb))
    
    # Count occurrences
    color_counts = Counter(quantized)
    
    # Filter suitable colors and score them
    scored_colors = []
    for (r, g, b), count in color_counts.items():
        if is_suitable_color(r, g, b):
            score = score_color(r, g, b, count)
            scored_colors.append((score, r, g, b))
    
    # Sort by score
    scored_colors.sort(reverse=True)
    
    # Return top color or fallback
    if scored_colors:
        _, r, g, b = scored_colors[0]
        return rgb_to_argb(r, g, b)
    
    # Fallback: most common color regardless of suitability
    if color_counts:
        r, g, b = color_counts.most_common(1)[0][0]
        return rgb_to_argb(r, g, b)
    
    # Ultimate fallback
    return 0xFF6366F1  # Indigo

def main():
    if not HAS_PIL:
        logger.error("Cannot run without PIL/Pillow")
        return
    
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
            # Download from URL
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
        
        # Extract color
        color = extract_dominant_color(full_path)
        hex_color = f"{color:08X}"
        
        logger.info(f"Extracted color (simple method): #{hex_color}")
        
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
# main()
