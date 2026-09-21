from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).with_name("mid_autumn_data.h")
W, H = 128, 296
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

black = (0, 0, 0)
red = (210, 35, 35)

# Large red moon with bold black outline and simple crater lines.
d.ellipse((14, 18, 114, 118), fill=red, outline=black, width=3)
d.ellipse((31, 40, 45, 51), outline=black, width=2)
d.ellipse((79, 32, 96, 45), outline=black, width=2)
d.arc((52, 68, 83, 94), 20, 150, fill=black, width=2)

# Minimal white jade rabbit silhouette on the moon.
d.ellipse((48, 55, 78, 92), fill="white", outline=black, width=2)
d.ellipse((53, 43, 67, 65), fill="white", outline=black, width=2)
d.ellipse((64, 43, 76, 65), fill="white", outline=black, width=2)
d.ellipse((59, 55, 62, 58), fill=black)
d.arc((65, 57, 73, 65), 10, 160, fill=black, width=1)
d.line((50, 75, 41, 69), fill=black, width=2)
d.line((76, 76, 84, 70), fill=black, width=2)

# Pomelo on the lower left.
d.ellipse((8, 145, 55, 192), fill="white", outline=black, width=3)
d.arc((14, 151, 49, 186), 210, 30, fill=red, width=3)
d.line((30, 145, 34, 138), fill=black, width=2)
d.ellipse((28, 136, 37, 142), fill=red, outline=black, width=1)

# Mooncake on the lower right, simplified for three-color output.
d.rounded_rectangle((68, 148, 120, 188), radius=8, fill=red, outline=black, width=3)
d.ellipse((68, 137, 120, 166), fill=red, outline=black, width=3)
d.ellipse((84, 143, 104, 160), outline=black, width=2)
d.line((94, 145, 94, 158), fill=black, width=2)
d.line((85, 151, 103, 151), fill=black, width=2)

# Decorative black/red separators.
d.line((8, 207, 120, 207), fill=red, width=3)
d.ellipse((12, 220, 18, 226), fill=red)
d.ellipse((110, 220, 116, 226), fill=red)

# Exact Chinese greeting.
font = ImageFont.truetype(r"C:\Windows\Fonts\msjhbd.ttc", 17)
label = "月圓人團圓"
box = d.textbbox((0, 0), label, font=font)
tx = (W - (box[2] - box[0])) // 2
d.text((tx, 238), label, font=font, fill=black)

# Native portrait memory layout: 128 x 296, one bit per pixel per plane.
black_bytes = [0xFF] * (W * H // 8)
red_bytes = [0xFF] * (W * H // 8)
for y in range(H):
    for x in range(W):
        r, g, b = img.getpixel((x, y))
        idx = y * (W // 8) + x // 8
        bit = 0x80 >> (x % 8)
        if r < 80 and g < 80 and b < 80:
            black_bytes[idx] &= ~bit
        elif r > 120 and r > g * 1.35 and r > b * 1.35:
            red_bytes[idx] &= ~bit

def c_array(name, values):
    rows = []
    for i in range(0, len(values), 16):
        rows.append("  " + ", ".join(f"0x{v:02X}" for v in values[i:i + 16]))
    return f"const uint8_t {name}[{len(values)}] PROGMEM = {{\n" + ",\n".join(rows) + "\n};\n"

OUT.write_text(
    "#pragma once\n#include <Arduino.h>\n\n" +
    c_array("midAutumnBlackImage", black_bytes) + "\n" +
    c_array("midAutumnRedImage", red_bytes),
    encoding="ascii",
)
