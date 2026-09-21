from pathlib import Path
from colorsys import rgb_to_hsv
from PIL import Image, ImageOps

SOURCE = Path(r"C:\Users\user\AppData\Local\Temp\codex-clipboard-d1d4dba1-03a2-4f76-94ae-89a6d0aa77f9.png")
OUT = Path(__file__).with_name("uploaded_picture_data.h")
W, H = 128, 296

src = Image.open(SOURCE).convert("RGB")
# Preserve the complete square composition and center it on the portrait panel.
picture = ImageOps.contain(src, (128, 190), method=Image.Resampling.LANCZOS)
canvas = Image.new("RGB", (W, H), "white")
canvas.paste(picture, ((W - picture.width) // 2, (H - picture.height) // 2))

black = [0xFF] * (W * H // 8)
red = [0xFF] * (W * H // 8)
for y in range(H):
    for x in range(W):
        r, g, b = canvas.getpixel((x, y))
        h, s, v = rgb_to_hsv(r / 255, g / 255, b / 255)
        lum = 0.299 * r + 0.587 * g + 0.114 * b
        # Red/pink/orange/yellow are represented by the panel's red channel.
        is_red = s > 0.22 and (h < 0.20 or h > 0.94) and v > 0.28
        is_black = lum < 92 or (lum < 125 and s < 0.20)
        index = y * (W // 8) + x // 8
        bit = 0x80 >> (x % 8)
        if is_red and not is_black:
            red[index] &= ~bit
        elif is_black:
            black[index] &= ~bit

def c_array(name, values):
    rows = []
    for i in range(0, len(values), 16):
        rows.append("  " + ", ".join(f"0x{v:02X}" for v in values[i:i + 16]))
    return f"const uint8_t {name}[{len(values)}] PROGMEM = {{\n" + ",\n".join(rows) + "\n};\n"

OUT.write_text(
    "#pragma once\n#include <Arduino.h>\n\n" +
    c_array("uploadedBlackImage", black) + "\n" +
    c_array("uploadedRedImage", red),
    encoding="ascii",
)
