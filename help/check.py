#!/usr/bin/env python3
"""ctest: render every help page and check links, anchors and images (#219).

    check.py SRC_DIR OUT_DIR
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import render


def main():
    src, out = sys.argv[1], sys.argv[2]
    titles = render.render(src, out)
    bad, notes = [], 0
    pages = {f[:-3] + ".html" for f in titles}
    text, anchors = {}, {}
    for p in pages:
        with open(os.path.join(out, p), encoding="utf-8") as fh:
            text[p] = fh.read()
        anchors[p] = set(re.findall(r'<a name="([^"]+)"', text[p]))
    for p in sorted(pages):
        h = text[p]
        notes += h.count("Image not available yet")
        for url in re.findall(r'href="([^"]+)"', h):
            if re.match(r"[a-z]+:", url):
                continue
            f, _, a = url.partition("#")
            f = f or p
            if f not in pages:
                bad.append("%s: link to missing page %s" % (p, url))
            elif a and a not in anchors[f]:
                bad.append("%s: link to missing anchor %s" % (p, url))
        for url in re.findall(r'<img src="([^"]+)"', h):
            if not os.path.exists(os.path.join(out, url)):
                bad.append("%s: broken image %s" % (p, url))
        if "<p>" not in h or "](" in h:
            bad.append("%s: unconverted Markdown" % p)
    with open(os.path.join(out, "toc.txt"), encoding="utf-8") as fh:
        toc = [l.split("\t")[0] for l in fh]
    for p in sorted(pages):
        if p not in toc:
            bad.append("%s missing from toc.txt" % p)
    print("%d pages, %d image placeholders" % (len(pages), notes))
    for b in bad:
        print("FAIL " + b)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
