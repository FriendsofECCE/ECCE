"""Print the top frames of the faulting thread of macOS crash reports (.ips or .crash)."""
import json
import re
import sys


def ips(text):
    head, _, body = text.partition("\n")
    data = json.loads(body)
    images = data.get("usedImages", [])
    threads = data.get("threads", [])
    t = data.get("faultingThread", 0)
    exc = data.get("exception", {})
    term = data.get("termination", {})
    print("  %s %s  %s" % (exc.get("type", "?"), exc.get("signal", ""),
                           term.get("indicator", "")))
    frames = threads[t]["frames"][:12] if t < len(threads) else []
    for f in frames:
        img = images[f["imageIndex"]].get("name", "?") if "imageIndex" in f else "?"
        print("    %s  %s +%s" % (img, f.get("symbol", "?"), f.get("symbolLocation", "")))


def crash(text):
    m = re.search(r"Crashed Thread:\s+(\d+)", text)
    if not m:
        return
    block = re.search(r"Thread %s Crashed.*?:\n(.*?)\n\n" % m.group(1), text, re.S)
    for line in (block.group(1).splitlines() if block else [])[:12]:
        print("   ", line)


for path in sys.argv[1:]:
    print(path)
    try:
        text = open(path, errors="replace").read()
        (ips if path.endswith(".ips") else crash)(text)
    except Exception as e:  # a summary must never fail the step
        print("  (could not parse: %s)" % e)
