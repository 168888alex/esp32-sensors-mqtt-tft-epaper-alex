from pathlib import Path
from colorsys import rgb_to_hsv
from PIL import Image, ImageDraw, ImageFont, ImageOps

SOURCE = Path(__file__).with_name("mid_autumn_source.png")
OUT = Path(__file__).with_name("mid_autumn_data.h")
W, H = 128, 296

src = Image.open(SOURCE).convert("RGBA")
# Remove the generated illustration's large empty lower area, then fit the
# complete festive scene into the native portrait e-paper area.
src = src.crop((0, 0, src.width, int(src.height * 0.75)))
art = ImageOps.fit(src, (128, 245), method=Image.Resampling.LANCZOS,
                   centering=(0.5, 0.48))
canvas = Image.new("RGBA", (W, H), (255, 255, 255, 255))
canvas.alpha_composite(art, (0, 0))

# Add exact Chinese text after image generation so the wording is accurate.
draw = ImageDraw.Draw(canvas)
font = ImageFont.truetype(r"C:\Windows\Fonts\msjhbd.ttc", 23)
label = "合家歡"
box = draw.textbbox((0, 0), label, font=font)
tx = (W - (box[2] - box[0])) // 2
draw.text((tx, 258), label, font=font, fill=(0, 0, 0, 255))

black = [0xFF] * (W * H // 8)
red = [0xFF] * (W * H // 8)

for y in range(H):
    for x in range(W):
        r, g, b, a = canvas.getpixel((x, y))
        if a < 80:
            continue
        h, s, v = rgb_to_hsv(r / 255, g / 255, b / 255)
        lum = 0.299 * r + 0.587 * g + 0.114 * b
        # The panel has no yellow channel: map yellow/orange to its red channel
        # so festive objects remain visible instead of disappearing as white.
        is_red = s > 0.15 and (h < 0.20 or h > 0.96) and v > 0.35
        is_black = lum < 78 or (lum < 120 and s < 0.20)
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
    c_array("midAutumnBlackImage", black) + "\n" +
    c_array("midAutumnRedImage", red),
    encoding="ascii",
)
