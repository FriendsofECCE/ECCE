#!/usr/bin/env python3
"""JCAMP-DX reader for infrared and Raman spectra (#986), wx-free.

Reads XYDATA=(X++(Y..Y)) in AFFN and the ASDF compressions (SQZ, DIF, DUP,
with the DIF Y-check), XYPOINTS=(XY..XY) and PEAK TABLE=(XY..XY).  The x axis
is returned in cm-1.  Used by ecce-spectrum (so by the Spectrum panel's
Experimental... button); `jcampdx.py FILE` prints what was read.
"""
import re
import sys


class JcampError(ValueError):
    pass


def _norm(label):
    return re.sub(r"[^A-Z0-9$]", "", label.upper())


def _ldrs(text):
    """-> [(normalised label, value)] with $$ comments removed."""
    out = []
    cur = None
    for raw in text.replace("\r\n", "\n").replace("\r", "\n").lstrip("﻿").split("\n"):
        if raw.startswith("##"):
            lab, _, val = raw[2:].partition("=")
            cur = [_norm(lab), val.split("$$")[0].strip()]
            out.append(cur)
        elif cur is not None:
            cur[1] += "\n" + raw.split("$$")[0].rstrip()
    return [(a, b.strip()) for a, b in out]


def _blocks(ldrs):
    """Flat list of blocks (dicts of label -> value); a LINK block has no data."""
    blocks, cur = [], None
    for lab, val in ldrs:
        if lab == "TITLE":
            cur = {"TITLE": val}
            blocks.append(cur)
        elif cur is not None and lab != "END":
            cur.setdefault(lab, val)
    return blocks


_AFFN = r"[+-]?(?:\d+\.?\d*|\.\d+)(?:[Ee][+-]?\d+)?"
_SQZ = {c: (d, 1) for d, c in enumerate("@ABCDEFGHI")}
_SQZ.update({c: (d, -1) for d, c in enumerate("abcdefghi", 1)})
_DIF = {c: (d, 1) for d, c in enumerate("%JKLMNOPQR")}
_DIF.update({c: (d, -1) for d, c in enumerate("jklmnopqr", 1)})
_DUP = {c: d for d, c in enumerate("STUVWXYZ", 1)}
_DUP["s"] = 9
_TOKEN = re.compile(r"[\s,]*(%s|[@A-Ia-i%%J-Rj-r][0-9]*\.?[0-9]*|[S-Zs][0-9]*)" % _AFFN)


def _decode_xydata(body):
    """-> y values (unscaled) from the (X++(Y..Y)) lines."""
    ys = []
    last_dif = False
    for ln in body.split("\n")[1:]:
        ln = ln.strip()
        if not ln:
            continue
        m = re.match(_AFFN, ln)
        if m is None:
            raise JcampError("Unreadable XYDATA line: %s" % ln[:30])
        pos = m.end()
        first = True
        prev_tok = None          # last Y token as ("val"|"dif", number)
        while pos < len(ln):
            m = _TOKEN.match(ln, pos)
            if m is None:
                if ln[pos:].strip(" ,") == "":
                    break
                raise JcampError("Unreadable XYDATA value: %s" % ln[pos:pos + 20])
            tok = m.group(1)
            pos = m.end()
            c = tok[0]
            if c in _DUP:
                if prev_tok is None:
                    raise JcampError("Repeat count without a value in XYDATA.")
                for _ in range(int(str(_DUP[c]) + tok[1:]) - 1):
                    if prev_tok[0] == "dif":
                        ys.append(ys[-1] + prev_tok[1])
                    else:
                        ys.append(prev_tok[1])
                continue
            if c in _DIF:
                d, s = _DIF[c]
                v = s * float(str(d) + tok[1:])
                prev_tok = ("dif", v)
                if not ys:
                    raise JcampError("Difference form with no start value in XYDATA.")
                ys.append(ys[-1] + v)
                first = False
                continue
            if c in _SQZ:
                d, s = _SQZ[c]
                v = s * float(str(d) + tok[1:])
            else:
                v = float(tok)
            prev_tok = ("val", v)
            if first and last_dif:
                first = False    # the Y-check repeats the previous line's last point
                continue
            first = False
            ys.append(v)
        last_dif = prev_tok is not None and prev_tok[0] == "dif"
    return ys


def _pairs(body):
    nums = [float(t) for t in re.findall(_AFFN, body.split("\n", 1)[1] if "\n" in body else "")]
    if len(nums) % 2:
        raise JcampError("Odd number of values in an (XY..XY) table.")
    return nums[0::2], nums[1::2]


def _num(b, key, default=None):
    try:
        return float(b[key])
    except (KeyError, ValueError):
        return default


def _label(b):
    parts = [b.get("TITLE", "").strip()]
    for k in ("STATE", "ORIGIN"):
        if b.get(k, "").strip():
            parts.append(b[k].strip())
    return ", ".join(p for p in parts if p)


def _spectrum(b):
    dtype = b.get("DATATYPE", "").upper()
    if dtype and "INFRARED" not in dtype and "RAMAN" not in dtype:
        raise JcampError("Not an infrared or Raman spectrum (JCAMP-DX DATA TYPE %s)." % b["DATATYPE"])
    xu = re.sub(r"\s", "", b.get("XUNITS", "1/CM").upper())
    if xu in ("1/CM", "CM-1", "CM^-1", "WAVENUMBER", "RAMANSHIFT"):
        xmul, xinv = 1.0, False
    elif xu in ("MICROMETERS", "MICROMETRES", "MICRONS", "UM"):
        xmul, xinv = 1e4, True
    elif xu in ("NANOMETERS", "NANOMETRES", "NM"):
        xmul, xinv = 1e7, True
    else:
        raise JcampError("Unsupported JCAMP-DX XUNITS %s (need 1/CM or MICROMETERS)." % b.get("XUNITS"))
    xf, yf = _num(b, "XFACTOR", 1.0), _num(b, "YFACTOR", 1.0)
    if "XYDATA" in b:
        form = "XYDATA"
        if not re.match(r"\(X\+\+\(Y\.\.Y\)\)", b["XYDATA"].replace(" ", "")):
            raise JcampError("Only XYDATA=(X++(Y..Y)) is supported.")
        ys = [y * yf for y in _decode_xydata(b["XYDATA"])]
        n = len(ys)
        fx, lx = _num(b, "FIRSTX"), _num(b, "LASTX")
        if fx is not None and lx is not None and n > 1:
            step = (lx - fx) / (n - 1)
        elif fx is not None and _num(b, "DELTAX") is not None:
            step = _num(b, "DELTAX")
        else:
            raise JcampError("XYDATA needs FIRSTX and LASTX (or DELTAX).")
        xs = [fx + i * step for i in range(n)]
    elif "XYPOINTS" in b or "PEAKTABLE" in b:
        form = "XYPOINTS" if "XYPOINTS" in b else "PEAKTABLE"
        xs, ys = _pairs(b[form])
        xs, ys = [x * xf for x in xs], [y * yf for y in ys]
    else:
        raise JcampError("No XYDATA, XYPOINTS or PEAK TABLE in the block.")
    if not xs or (len(xs) < 2 and form != "PEAKTABLE"):
        raise JcampError("The spectrum has fewer than two points.")
    yu = b.get("YUNITS", "").upper()
    percent = "%" in yu or "PERCENT" in yu
    if "TRANSMI" in yu or (percent and "ABSORB" not in yu):
        kind = "transmittance"
    elif "ABSORB" in yu:
        kind = "absorbance"
    else:
        kind = "auto"
    xs = [xmul / x if xinv and x else x for x in xs]
    percent = percent or (kind == "transmittance" and max(ys) > 1.5)
    if form == "PEAKTABLE":
        # Draw each peak as a narrow triangle on a flat baseline.
        base = (100.0 if percent else 1.0) if kind == "transmittance" else 0.0
        px, py = [], []
        for x, y in sorted(zip(xs, ys)):
            px += [x - 2.0, x, x + 2.0]
            py += [base, y, base]
        xs, ys = px, py
    pts = sorted(zip(xs, ys))
    return {"title": b.get("TITLE", ""), "state": b.get("STATE", ""),
            "origin": b.get("ORIGIN", ""), "datatype": b.get("DATATYPE", ""),
            "form": form, "kind": kind, "percent": percent, "label": _label(b),
            "x": [p[0] for p in pts], "y": [p[1] for p in pts]}


def parse(text):
    """-> list of spectrum dicts, one per block that holds data."""
    ldrs = _ldrs(text)
    if not ldrs or ldrs[0][0] != "TITLE":
        raise JcampError("Not a JCAMP-DX file (no ##TITLE first).")
    if any(a == "NTUPLES" for a, _ in ldrs):
        raise JcampError("JCAMP-DX NTUPLES files are not supported.")
    out = []
    for b in _blocks(ldrs):
        if any(k in b for k in ("XYDATA", "XYPOINTS", "PEAKTABLE")):
            out.append(_spectrum(b))
    if not out:
        dt = next((v for a, v in ldrs if a == "DATATYPE"), "")
        raise JcampError("No infrared or Raman spectrum found in the JCAMP-DX file%s." %
                         (" (DATA TYPE %s)" % dt if dt else ""))
    return out


def read(path):
    """-> (first spectrum, number of spectra in the file)."""
    with open(path, errors="replace") as f:
        blocks = parse(f.read())
    return blocks[0], len(blocks)


def is_jcamp(path):
    if path.lower().endswith((".jdx", ".dx", ".jcamp", ".jcm")):
        return True
    with open(path, errors="replace") as f:
        for ln in f:
            if ln.strip():
                return ln.lstrip("﻿").upper().startswith("##TITLE")
    return False


if __name__ == "__main__":
    try:
        for s in parse(open(sys.argv[1], errors="replace").read()):
            print("%s\t%s\t%s\t%d points\t%.2f..%.2f cm-1" % (
                s["label"], s["form"], s["kind"], len(s["x"]), s["x"][0], s["x"][-1]))
    except (JcampError, OSError, IndexError) as e:
        sys.exit("jcampdx: %s" % e)
