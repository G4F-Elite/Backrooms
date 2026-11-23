import sys, glob, os
from PIL import Image, ImageDraw
src, out, cols, tw = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
files = sorted(glob.glob(os.path.join(src, "*.bmp")))
if len(sys.argv) > 5:
    a, b = map(int, sys.argv[5].split(":")); files = files[a:b]
ims = [Image.open(f).convert("RGB") for f in files]
th = tw * ims[0].height // ims[0].width
rows = (len(ims) + cols - 1) // cols
sheet = Image.new("RGB", (cols * tw, rows * th))
d = ImageDraw.Draw(sheet)
for k, im in enumerate(ims):
    x, y = (k % cols) * tw, (k // cols) * th
    sheet.paste(im.resize((tw, th), Image.LANCZOS), (x, y))
    d.text((x + 4, y + 2), os.path.basename(files[k])[:-4], fill=(255, 255, 0))
sheet.save(out)
