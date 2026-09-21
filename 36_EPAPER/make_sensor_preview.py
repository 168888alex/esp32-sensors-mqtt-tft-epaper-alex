from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).with_name("epaper_sensor_layout_preview.png")
W, H = 296, 128
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)
black = (0, 0, 0)
red = (210, 35, 35)

font_title = ImageFont.truetype(r"C:\Windows\Fonts\arialbd.ttf", 13)
font_label = ImageFont.truetype(r"C:\Windows\Fonts\arialbd.ttf", 10)
font_value = ImageFont.truetype(r"C:\Windows\Fonts\arialbd.ttf", 22)
font_unit = ImageFont.truetype(r"C:\Windows\Fonts\arialbd.ttf", 12)

# Header
d.text((10, 5), "ESP32 SENSOR", font=font_title, fill=black)
d.text((211, 6), "LIVE", font=font_label, fill=red)
d.line((8, 23, 288, 23), fill=red, width=2)

centers = [51, 148, 245]
for x in (99, 196):
    d.line((x, 31, x, 119), fill=black, width=1)

def thermometer(cx, cy):
    d.ellipse((cx - 9, cy + 12, cx + 9, cy + 30), fill=red, outline=black, width=2)
    d.rounded_rectangle((cx - 5, cy - 23, cx + 5, cy + 22), radius=5,
                        fill="white", outline=black, width=2)
    d.rectangle((cx - 2, cy - 5, cx + 2, cy + 21), fill=red)
    for yy in (-13, -4, 5):
        d.line((cx + 7, cy + yy, cx + 11, cy + yy), fill=black, width=1)

def droplet(cx, cy):
    pts = [(cx, cy - 25), (cx - 15, cy - 3), (cx - 14, cy + 10),
           (cx - 6, cy + 22), (cx, cy + 25), (cx + 6, cy + 22),
           (cx + 14, cy + 10), (cx + 15, cy - 3)]
    d.polygon(pts, fill=red, outline=black)
    d.line((cx - 6, cy - 4, cx - 11, cy + 6), fill="white", width=2)

def sun(cx, cy):
    d.ellipse((cx - 12, cy - 12, cx + 12, cy + 12), fill=red, outline=black, width=2)
    for dx, dy in ((0, -24), (0, 24), (-24, 0), (24, 0),
                   (-17, -17), (17, -17), (-17, 17), (17, 17)):
        d.line((cx + dx * 0.7, cy + dy * 0.7, cx + dx, cy + dy), fill=black, width=2)

thermometer(centers[0], 53)
droplet(centers[1], 53)
sun(centers[2], 53)

labels = ["TEMP", "HUMI", "LIGHT"]
values = [("26.5", "°C"), ("58", "%"), ("72", "%")]
for cx, label, (value, unit) in zip(centers, labels, values):
    d.text((cx - d.textlength(label, font=font_label) / 2, 84), label,
           font=font_label, fill=black)
    vw = d.textlength(value, font=font_value)
    uw = d.textlength(unit, font=font_unit)
    total = vw + 3 + uw
    x = cx - total / 2
    d.text((x, 98), value, font=font_value, fill=black)
    d.text((x + vw + 3, 104), unit, font=font_unit, fill=red)

img.save(OUT)
print(OUT)
