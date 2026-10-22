#!/usr/bin/env python3
"""Before/after sheet from two calced_layout_shots.py output folders:
calced_layout_sheet.py BEFORE_DIR AFTER_DIR OUT.png"""
import sys
from PIL import Image, ImageDraw

before, after, out = sys.argv[1:4]
codes = [("nwchem", "NWChem"), ("orca", "ORCA"), ("g16", "Gaussian 16"), ("ecceqm", "ECCE-QM")]
pad, head = 20, 28
cells = []
for key, _ in codes:
    cells.append((Image.open("%s/calced-%s-default.png" % (before, key)).convert("RGB"),
                  Image.open("%s/calced-%s-default.png" % (after, key)).convert("RGB")))
colw = [max(c[i].width for c in cells) for i in (0, 1)]
rowh = [max(c[0].height, c[1].height) for c in cells]
W = pad * 3 + sum(colw)
sheet = Image.new("RGB", (W, pad + head + sum(h + head + pad for h in rowh)), "white")
d = ImageDraw.Draw(sheet)
d.text((pad, pad // 2), "BEFORE (main)", fill="black")
d.text((pad * 2 + colw[0], pad // 2), "AFTER (wip/calced-layout-2)", fill="black")
y = pad + head // 2
for (key, label), (b, a), h in zip(codes, cells, rowh):
    d.text((pad, y), label, fill="black")
    y += head
    sheet.paste(b, (pad, y))
    sheet.paste(a, (pad * 2 + colw[0], y))
    y += h + pad
sheet.save(out)
