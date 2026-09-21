from pathlib import Path
from colorsys import rgb_to_hsv
from PIL import Image, ImageDraw, ImageFont, ImageOps

SOURCE = Path(__file__).with_name("chang_e_source.png")
OUT = Path(__file__).with_name("chang_e_data.h")
PREVIEW = Path(__file__).with_name("chang_e_preview.png")
W, H = 128, 296

src = Image.open(SOURCE).convert("RGB")
picture = ImageOps.fit(src, (W, H), method=Image.Resampling.LANCZOS,
                       centering=(0.5, 0.5))

# Vertical red/white greeting at the lower-right corner.
draw = ImageDraw.Draw(picture)
font = ImageFont.truetype(r"C:\Windows\Fonts\msjhbd.ttc", 15)
for i, char in enumerate("蔡政達敬賀"):
    draw.text((108, 205 + i * 17), char, font=font,
              fill=(255, 255, 255),
              stroke_width=1, stroke_fill=(0, 0, 0))

picture.save(PREVIEW)

black = [0xFF] * (W * H // 8)
red = [0xFF] * (W * H // 8)
for y in range(H):
    for x in range(W):
        r, g, b = picture.getpixel((x, y))
        h, s, v = rgb_to_hsv(r / 255, g / 255, b / 255)
        lum = 0.299 * r + 0.587 * g + 0.114 * b
        is_red = s > 0.22 and (h < 0.20 or h > 0.94) and v > 0.25
        is_black = lum < 82 or (lum < 125 and s < 0.18)
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
    c_array("changEBlackImage", black) + "\n" +
    c_array("changERedImage", red),
    encoding="ascii",
)
