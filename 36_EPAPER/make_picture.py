from pathlib import Path
from colorsys import rgb_to_hsv
from PIL import Image, ImageOps

SOURCE = Path(r"C:\Users\user\AppData\Local\Temp\codex-clipboard-a7613d4b-459e-4ae1-ac60-6044eb2a2439.png")
OUT = Path(__file__).with_name("picture_data.h")
W, H = 128, 296

src = Image.open(SOURCE).convert("RGBA")
canvas = Image.new("RGBA", (W, H), (255, 255, 255, 255))
fitted = ImageOps.contain(src, (W, 190), method=Image.Resampling.LANCZOS)
canvas.alpha_composite(fitted, ((W - fitted.width) // 2, (H - fitted.height) // 2))

black = [0xFF] * (W * H // 8)
red = [0xFF] * (W * H // 8)  # 1 means no red; 0 means red on this controller

for y in range(H):
    for x in range(W):
        r, g, b, a = canvas.getpixel((x, y))
        if a < 80:
            continue
        h, s, v = rgb_to_hsv(r / 255, g / 255, b / 255)
        lum = 0.299 * r + 0.587 * g + 0.114 * b
        # Warm saturated colors become the red e-paper channel.
        is_red = (s > 0.28 and (h < 0.16 or h > 0.96) and v > 0.35)
        is_black = (lum < 82) or (lum < 125 and s < 0.20)
        index = y * (W // 8) + x // 8
        bit = 0x80 >> (x % 8)
        if is_red and not is_black:
            red[index] &= ~bit
        elif is_black:
            black[index] &= ~bit

def c_array(name, values):
    lines = []
    for i in range(0, len(values), 16):
        lines.append("  " + ", ".join(f"0x{v:02X}" for v in values[i:i + 16]))
    return f"const uint8_t {name}[{len(values)}] PROGMEM = {{\n" + ",\n".join(lines) + "\n};\n"

OUT.write_text(
    "#pragma once\n#include <Arduino.h>\n\n" +
    c_array("pictureBlackImage", black) + "\n" +
    c_array("pictureRedImage", red),
    encoding="ascii",
)
