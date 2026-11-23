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
    with the number of samples it lasted."""

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
                    rows = []
                    for r in range(0, h, self.STEP):
                        row = memoryview(raw[r * bpl:r * bpl + w * 4]).cast("I")
                        rows.append(row[::self.STEP].tobytes())
                    frame = b"".join(rows)
                    if self.states and self.states[-1][0] == frame:
                        self.states[-1][1] += 1
                    else:
                        self.states.append([frame, 1])
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
    for frame, count in states[1:-1]:
        d = min(difference(frame, first), difference(frame, last))
        if d > tolerance:
            bad += 1
            samples += count
            worst = max(worst, d)
    return bad, samples, worst
