#!/usr/bin/env python3
"""Render help/src/*.md to HTML for ECCE's help window (#219).

    render.py SRC_DIR OUT_DIR

Python 3 standard library only.  Handles the subset the help uses:
headings, paragraphs, lists, fenced code, inline code, bold/italic, links,
images, tables.  Output is plain HTML 3.2-style markup that wxHtmlWindow and
a browser both show.  Also writes OUT_DIR/toc.txt (one "file<TAB>title" per
contents entry: index.md, the pages it links in order, then any others).
"""
import html
import os
import re
import shutil
import sys


def slug(text):
    s = re.sub(r"<[^>]+>", "", text).lower()
    s = re.sub(r"[^\w\- ]", "", s)
    return s.strip().replace(" ", "-")


def inline(text, missing):
    codes = []

    def stash(m):
        codes.append("<code>%s</code>" % html.escape(m.group(1)))
        return "\0%d\0" % (len(codes) - 1)

    text = re.sub(r"`([^`]+)`", stash, text)
    text = html.escape(text, quote=False)

    def img(m):
        alt, src = m.group(1), m.group(2)
        if src in missing:
            return "<i>[Image not available yet: %s]</i>" % alt
        return '<img src="%s" alt="%s">' % (src, alt)

    text = re.sub(r"!\[([^\]]*)\]\(([^)\s]+)\)", img, text)

    def link(m):
        url = m.group(2)
        if not re.match(r"[a-z]+:", url):
            url = re.sub(r"\.md(#|$)", r".html\1", url)
        return '<a href="%s">%s</a>' % (url, m.group(1))

    text = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", link, text)
    text = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", text)
    text = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<i>\1</i>", text)
    text = re.sub(r"(?<![\w])_(?!\s)(.+?)(?<!\s)_(?![\w])", r"<i>\1</i>", text)
    return re.sub(r"\0(\d+)\0", lambda m: codes[int(m.group(1))], text)


def cells(line):
    line = line.strip()
    if line.startswith("|"):
        line = line[1:]
    if line.endswith("|"):
        line = line[:-1]
    return [c.strip() for c in re.split(r"(?<!\\)\|", line)]


ITEM = re.compile(r"(\s*)([-*]|\d+\.)\s+(.*)")


def convert(md, image_exists):
    """Return (title, html body)."""
    missing = {m.group(1) for m in re.finditer(r"!\[[^\]]*\]\(([^)\s]+)\)", md)
               if not image_exists(m.group(1))}
    md = re.sub(r"<!--.*?-->", "", md, flags=re.S)
    lines = md.split("\n")
    out, title, i = [], None, 0
    used = {}
    while i < len(lines):
        ln = lines[i]
        if not ln.strip():
            i += 1
            continue
        if ln.startswith("```"):
            buf = []
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                buf.append(lines[i])
                i += 1
            i += 1
            out.append("<pre>%s</pre>" % html.escape("\n".join(buf)))
            continue
        m = re.match(r"(#{1,6})\s+(.*)", ln)
        if m:
            n, text = len(m.group(1)), m.group(2).strip()
            h = inline(text, missing)
            if title is None and n == 1:
                title = re.sub(r"<[^>]+>", "", h)
            a = slug(h)
            k = used.get(a, 0)
            used[a] = k + 1
            if k:
                a += "-%d" % k
            out.append('<a name="%s"></a><h%d>%s</h%d>' % (a, n, h, n))
            i += 1
            continue
        if ln.lstrip().startswith("|") and i + 1 < len(lines) \
                and re.match(r"\s*\|?[\s:\-|]+\|[\s:\-|]*$", lines[i + 1]):
            head = cells(ln)
            i += 2
            rows = []
            while i < len(lines) and lines[i].lstrip().startswith("|"):
                rows.append(cells(lines[i]))
                i += 1
            t = ['<table border="1" cellpadding="4" cellspacing="0">', "<tr>"]
            t += ["<th>%s</th>" % inline(c, missing) for c in head]
            t.append("</tr>")
            for r in rows:
                t.append("<tr>" + "".join(
                    "<td>%s</td>" % inline(c, missing) for c in r) + "</tr>")
            t.append("</table>")
            out.append("\n".join(t))
            continue
        lm = ITEM.match(ln)
        if lm:
            ordered = lm.group(2)[0].isdigit()
            items = []
            while i < len(lines):
                lm = ITEM.match(lines[i])
                if lm and not lines[i].startswith("    "):
                    items.append([lm.group(3)])
                    i += 1
                elif items and lines[i].startswith(" ") and lines[i].strip():
                    items[-1].append(lines[i].strip())
                    i += 1
                elif items and not lines[i].strip() and i + 1 < len(lines) \
                        and (ITEM.match(lines[i + 1])
                             or lines[i + 1].startswith("   ")):
                    i += 1
                else:
                    break
            tag = "ol" if ordered else "ul"
            out.append("<%s>\n%s\n</%s>" % (tag, "\n".join(
                "<li>%s</li>" % inline(" ".join(it), missing) for it in items),
                tag))
            continue
        buf = []
        while i < len(lines) and lines[i].strip() \
                and not re.match(r"(#{1,6}\s|```|\s*([-*]|\d+\.)\s)", lines[i]) \
                and not lines[i].lstrip().startswith("|"):
            buf.append(lines[i].strip())
            i += 1
        if not buf:
            buf.append(lines[i].strip())
            i += 1
        out.append("<p>%s</p>" % inline(" ".join(buf), missing))
    return title or "Help", "\n".join(out)


PAGE = """<html>
<head><meta charset="utf-8"><title>%(title)s</title></head>
<body>
%(body)s
</body>
</html>
"""


def render(src, out):
    os.makedirs(out, exist_ok=True)
    pages = sorted(f for f in os.listdir(src) if f.endswith(".md"))
    titles = {}
    for f in pages:
        with open(os.path.join(src, f), encoding="utf-8") as fh:
            md = fh.read()
        title, body = convert(
            md, lambda p: os.path.exists(os.path.join(src, p)))
        titles[f] = title
        with open(os.path.join(out, f[:-3] + ".html"), "w",
                  encoding="utf-8") as fh:
            fh.write(PAGE % {"title": html.escape(title), "body": body})
    img = os.path.join(src, "img")
    if os.path.isdir(img):
        shutil.copytree(img, os.path.join(out, "img"), dirs_exist_ok=True)
    order = []
    if "index.md" in titles:
        order.append("index.md")
        with open(os.path.join(src, "index.md"), encoding="utf-8") as fh:
            for m in re.finditer(r"\]\(([\w\-]+)\.md\)", fh.read()):
                f = m.group(1) + ".md"
                if f in titles and f not in order:
                    order.append(f)
    order += [f for f in pages if f not in order]
    with open(os.path.join(out, "toc.txt"), "w", encoding="utf-8") as fh:
        for f in order:
            fh.write("%s.html\t%s\n" % (f[:-3], titles[f]))
    return titles


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    render(sys.argv[1], sys.argv[2])
