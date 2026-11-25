"""Sample a rectangle of an X screen every ~10 ms from outside the program
(#1210), and count the states that are neither the picture before a change
nor the picture after it: what the user sees half-drawn.

Uses libX11 through ctypes (XGetImage on the root window), so it works on
any Xvfb display without extra packages.
"""

import ctypes
import ctypes.util
import threading
import time

_X = None


class _XImage(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int),
                ("xoffset", ctypes.c_int), ("format", ctypes.c_int),
                ("data", ctypes.c_void_p), ("byte_order", ctypes.c_int),
                ("bitmap_unit", ctypes.c_int),
                ("bitmap_bit_order", ctypes.c_int),
                ("bitmap_pad", ctypes.c_int), ("depth", ctypes.c_int),
                ("bytes_per_line", ctypes.c_int),
                ("bits_per_pixel", ctypes.c_int)]


def _lib():
    global _X
    if _X is None:
        _X = ctypes.CDLL(ctypes.util.find_library("X11") or "libX11.so.6")
        _X.XOpenDisplay.restype = ctypes.c_void_p
        _X.XOpenDisplay.argtypes = [ctypes.c_char_p]
        _X.XDefaultRootWindow.restype = ctypes.c_ulong
        _X.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
        _X.XGetImage.restype = ctypes.POINTER(_XImage)
        _X.XGetImage.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int,
                                 ctypes.c_int, ctypes.c_uint, ctypes.c_uint,
                                 ctypes.c_ulong, ctypes.c_int]
        _X.XFree.argtypes = [ctypes.c_void_p]
        _X.XCloseDisplay.argtypes = [ctypes.c_void_p]
    return _X


class Sampler:
    """Samples (x, y, w, h) of `display` (":N") until stop(); keeps one
    sub-sampled copy (every STEP-th pixel) per distinct consecutive state,
    with the number of samples it lasted and the time it was first seen."""

    STEP = 4

    def __init__(self, display, rect, period=0.01):
        self.display, self.rect, self.period = display, rect, period
        self.states = []          # [bytes, samples]
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self):
        self._thread.start()
        return self

    def stop(self):
        self._stop.set()
        self._thread.join(10)
        return self.states

    def _run(self):
        X = _lib()
        dpy = X.XOpenDisplay(self.display.encode())
        if not dpy:
            return
        root = X.XDefaultRootWindow(dpy)
        x, y, w, h = self.rect
        try:
            while not self._stop.is_set():
                img = X.XGetImage(dpy, root, x, y, w, h, 0xffffffff, 2)
                if img:
                    im = img.contents
                    bpl = im.bytes_per_line
                    raw = ctypes.string_at(im.data, bpl * h)
                    X.XFree(ctypes.c_void_p(im.data))
                    X.XFree(img)
                    pad = 3 if im.byte_order == 0 else 0
                    rows = []
                    for r in range(0, h, self.STEP):
                        row = memoryview(raw[r * bpl:r * bpl + w * 4]).cast("I")
                        rows.append(row[::self.STEP].tobytes())
                    # A depth-24 visual keeps 32-bit pixels whose top byte is
                    # never shown and differs between GTK's and cairo's
                    # drawing of the same colour: compare the colour only.
                    frame = bytearray(b"".join(rows))
                    frame[pad::4] = bytes(len(frame) // 4)
                    frame = bytes(frame)
                    if self.states and self.states[-1][0] == frame:
                        self.states[-1][1] += 1
                    else:
                        self.states.append([frame, 1, time.time()])
                time.sleep(self.period)
        finally:
            X.XCloseDisplay(dpy)


def difference(a, b):
    """Fraction of sampled pixels that differ."""
    ma, mb = memoryview(a).cast("I"), memoryview(b).cast("I")
    n = len(ma)
    return sum(1 for p, q in zip(ma, mb) if p != q) / float(n or 1)


def halfDrawn(states, tolerance=0.03):
    """States that are neither the first (the picture before the change)
    nor the last (the finished picture), within `tolerance` of the pixels.
    Answers (distinct states, samples, worst difference) of those."""
    if len(states) < 2:
        return 0, 0, 0.0
    first, last = states[0][0], states[-1][0]
    bad = samples = 0
    worst = 0.0
    for frame, count, *_ in states[1:-1]:
        d = min(difference(frame, first), difference(frame, last))
        if d > tolerance:
            bad += 1
            samples += count
            worst = max(worst, d)
    return bad, samples, worst


def describe(states, tolerance=0.03):
    """The half-drawn states as "start+samples:dFirst/dLast/flat" (percent;
    flat is the share of the most common colour), start in samples."""
    out = []
    if len(states) < 2:
        return out
    first, last = states[0][0], states[-1][0]
    at = states[0][1]
    for frame, count, *_ in states[1:-1]:
        a, b = difference(frame, first), difference(frame, last)
        if min(a, b) > tolerance:
            px = memoryview(frame).cast("I")
            counts = {}
            for p in px:
                counts[p] = counts.get(p, 0) + 1
            flat = max(counts.values()) / float(len(px) or 1)
            out.append("%d+%d:%.0f/%.0f/%.0f" % (at, count, 100 * a, 100 * b,
                                                100 * flat))
        at += count
    return out


def since(states, t):
    """The states from the one on the screen at time `t` on: what came
    before `t` (a status line changing before the command) is not part of
    the change."""
    i = 0
    for j, st in enumerate(states):
        if st[2] <= t:
            i = j
    return states[i:]


def timeline(states, t0):
    """Every state as "+ms:samples:dFirst/dLast" (percent), ms from t0."""
    if not states:
        return ""
    first, last = states[0][0], states[-1][0]
    return " ".join("%+d:%d:%.0f/%.0f" % (1000 * (t - t0), n,
                                         100 * difference(f, first),
                                         100 * difference(f, last))
                    for f, n, t in states)
