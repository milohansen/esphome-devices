from extract_dominant_color_simple import extract_dominant_color as edc 


field = "./DSC_0044.jpg"
color = edc(field)
print(f"[FIELD] Dominant color: {color:#06x}")

landscape = "./DSC_4482.jpg"
color = edc(landscape)
print(f"[LANDSCAPE] Dominant color: {color:#06x}")

cat = "./DSC_5385.jpg"
color = edc(cat)
print(f"[CAT] Dominant color: {color:#06x}")