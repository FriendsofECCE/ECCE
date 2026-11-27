#!/usr/bin/env python3
# Standard orientation wins over the held Input frame: orbitals and modes
# use that frame. Side files also hand the initial structure to the IRC.
import io
import os
import re
import sys

from pertab import atomicSymbol


def configure_streams():
    # Job output can contain stray bytes; Python 3.6 has no reconfigure().
    for name in ("stdin", "stdout"):
        stream = getattr(sys, name)
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
        else:
            setattr(sys, name, io.TextIOWrapper(stream.buffer, encoding="utf-8",
                                               errors="replace"))


# Perl matched bytes: \s is ASCII whitespace there, and a field that is not
# a number reads as its numeric prefix or 0 instead of raising (#1331).
_WS = " \t\n\r\f\v"
_NUM = re.compile(r"[ \t\n\r\f\v]*([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)", re.ASCII)


def _num(text):
    m = _NUM.match(text)
    return float(m.group(1)) if m else 0.0


def _fields(text):
    # Perl's split(" ", ...): ASCII whitespace runs, leading ones ignored.
    return [x for x in re.split(r"[ \t\n\r\f\v]+", text) if x]


def read_block():
    rows, labels = [], []
    for line in sys.stdin:
        if not re.match(r"^\s+\d+", line, re.ASCII):
            break
        fields = re.split(r" +", line[:-1].lstrip(_WS).replace("D", "e"))
        z = int(_num(fields[1])) if len(fields) > 1 else 0
        if z < 0:
            continue
        rows.append(" ".join((fields + [""] * 6)[3:6]))
        symbol = atomicSymbol[z] if z < len(atomicSymbol) else ""
        labels.append("{}-{}".format(len(rows), symbol))
    return rows, labels


def same_frame(a, b):
    a, b = _fields(a), _fields(b)
    return len(a) == len(b) and all(_num(x) == _num(y) for x, y in zip(a, b))


def read_lines(path):
    try:
        with open(path, encoding="utf-8", errors="replace", newline="") as fh:
            return fh.readlines()
    except OSError:
        return []


def write_file(path, text):
    try:
        with open(path, "w", encoding="utf-8", errors="replace", newline="") as fh:
            fh.write(text)
    except OSError:
        pass


def emit_frame(key, rows, labels):
    if not rows:
        return
    sig = " ".join(rows)
    path = os.path.join(key, "g16LastGeom")
    last = read_lines(path)
    if last and same_frame(last[0].rstrip("\n"), sig):
        return
    write_file(path, sig + "\n")
    print("key: GEOMTRACE\nsize:\n1 {} 3\nrowlabels:\n {}".format(
        len(rows), " ".join(labels)))
    print("columnlabels:\nX Y Z\nvalues:")
    for row in rows:
        print(" " + row)
    print("units:\nAngstrom\nEND", flush=True)


def stash_file(key):
    return os.path.join(key, "g16InputGeom")


def save_stash(key, rows, labels):
    if rows:
        write_file(stash_file(key), " ".join(labels) + "\n" +
                   "".join(row + "\n" for row in rows))


def take_stash(key):
    lines = read_lines(stash_file(key))
    try:
        os.remove(stash_file(key))
    except OSError:
        pass
    if not lines:
        return [], []
    return [line[:-1] for line in lines[1:]], lines[0][:-1].split(" ")


def save_irc_input(key, rows):
    if rows:
        write_file(os.path.join(key, "g16IrcInput"), ",".join(rows) + "\n")
