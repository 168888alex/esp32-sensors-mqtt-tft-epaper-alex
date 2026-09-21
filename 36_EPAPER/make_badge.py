from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps, ImageEnhance

PHOTO = Path(r"C:\Users\user\Downloads\1000071768.jpg")
OUT = Path(__file__).with_name("badge_data.h")
W, H = 296, 128
RAW_W, RAW_H = 128, 296

font_regular = r"C:\Windows\Fonts\msjh.ttc"
font_bold = r"C:\Windows\Fonts\msjhbd.ttc"

black = Image.new("1", (W, H), 0)
red = Image.new("1", (W, H), 0)  # 0 = red, 1 = no red layer
bd = ImageDraw.Draw(black)
rd = ImageDraw.Draw(red)

# White photo panel: suppress the red layer across this rectangle.
photo_box = (5, 7, 105, 120)
rd.rectangle(photo_box, fill=1)
bd.rectangle(photo_box, outline=1, width=2)

# Close head-and-shoulders crop from the supplied portrait.
photo = Image.open(PHOTO).convert("RGB")
photo = photo.crop((2100, 850, 3150, 2200))
photo = ImageOps.fit(photo, (96, 105), method=Image.Resampling.LANCZOS,
                     centering=(0.53, 0.42))
photo = ImageOps.grayscale(photo)
photo = ImageEnhance.Brightness(photo).enhance(1.15)
photo = ImageEnhance.Contrast(photo).enhance(1.25)
photo = ImageOps.autocontrast(photo)
photo = photo.convert("1", dither=Image.Dither.FLOYDSTEINBERG)
black.paste(photo, (7, 9))

def f(path, size):
    return ImageFont.truetype(path, size)

regular10 = f(font_regular, 10)
regular12 = f(font_regular, 12)
bold18 = f(font_bold, 18)
bold21 = f(font_bold, 21)
latin14 = f(r"C:\Windows\Fonts\arialbd.ttf", 14)

# Right-hand identification information.
bd.text((115, 7), "識別證", font=bold18, fill=1)
bd.text((115, 29), "蔡政達", font=bold21, fill=1)
bd.text((115, 57), "NO.  SKY007", font=latin14, fill=1)
bd.text((115, 77), "農業部", font=regular12, fill=1)
bd.text((115, 92), "漁業署", font=regular12, fill=1)
bd.text((115, 108), "CEO", font=latin14, fill=1)

# Convert landscape coordinates to the panel's native 128 x 296 memory layout.
black_bytes = [0xFF] * (RAW_W * RAW_H // 8)
red_bytes = [0x00] * (RAW_W * RAW_H // 8)

for y in range(H):
    for x in range(W):
        raw_x = y
        raw_y = RAW_W * 0 + (RAW_H - 1 - x)
        index = raw_y * (RAW_W // 8) + raw_x // 8
        bit = 0x80 >> (raw_x % 8)
        if black.getpixel((x, y)):
            black_bytes[index] &= ~bit
            red_bytes[index] |= bit
        elif red.getpixel((x, y)):
            red_bytes[index] |= bit

def c_array(name, values):
    lines = []
    for i in range(0, len(values), 16):
        lines.append("  " + ", ".join(f"0x{v:02X}" for v in values[i:i + 16]))
    return f"const uint8_t {name}[{len(values)}] PROGMEM = {{\n" + ",\n".join(lines) + "\n};\n"

OUT.write_text(
    "#pragma once\n#include <Arduino.h>\n\n" +
    c_array("badgeBlackImage", black_bytes) + "\n" +
    c_array("badgeRedImage", red_bytes),
    encoding="ascii",
)
