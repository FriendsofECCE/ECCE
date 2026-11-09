#!/usr/bin/env python3
"""embed-manifest.py MANIFEST EXE...: replaces the application manifest of
already-built programs (the MSYS2 mosquitto tools) with MANIFEST.

mosquitto and mosquitto_passwd open files with the ANSI API.  Without the
UTF-8 code-page manifest that every ECCE program has (#922), a profile path
holding a letter outside the machine's code page (C:\\Users\\Łukasz) turns
into "Lukasz" and the broker cannot start.  Windows only; uses kernel32's
resource-update API, so no SDK tool is needed.
"""
import ctypes
import sys
from ctypes import wintypes

RT_MANIFEST = 24
LOAD_LIBRARY_AS_DATAFILE = 0x2

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
k32.LoadLibraryExW.restype = wintypes.HMODULE
k32.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
k32.FreeLibrary.argtypes = [wintypes.HMODULE]
k32.BeginUpdateResourceW.restype = wintypes.HANDLE
k32.BeginUpdateResourceW.argtypes = [wintypes.LPCWSTR, wintypes.BOOL]
k32.UpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPVOID,
                                wintypes.WORD, wintypes.LPVOID, wintypes.DWORD]
k32.EndUpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.BOOL]
NameProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, wintypes.LPVOID,
                              wintypes.LPVOID, wintypes.LPARAM)
LangProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, wintypes.LPVOID,
                              wintypes.LPVOID, wintypes.WORD, wintypes.LPARAM)
k32.EnumResourceNamesW.argtypes = [wintypes.HMODULE, wintypes.LPVOID, NameProc, wintypes.LPARAM]
k32.EnumResourceLanguagesW.argtypes = [wintypes.HMODULE, wintypes.LPVOID, wintypes.LPVOID,
                                       LangProc, wintypes.LPARAM]


def existing(exe):
    """(id, language) of each manifest resource; numeric ids only (the
    loader reads no other)."""
    found = []
    h = k32.LoadLibraryExW(exe, None, LOAD_LIBRARY_AS_DATAFILE)
    if not h:
        raise OSError(ctypes.get_last_error(), "cannot load " + exe)

    def lang(_m, _t, name, lid, _p):
        found.append((name, lid))
        return True

    def name(_m, _t, n, _p):
        if n is not None and n < 0x10000:
            k32.EnumResourceLanguagesW(h, RT_MANIFEST, n, LangProc(lang), 0)
        return True

    k32.EnumResourceNamesW(h, RT_MANIFEST, NameProc(name), 0)
    k32.FreeLibrary(h)
    return found


def embed(manifest, exe):
    old = existing(exe)
    u = k32.BeginUpdateResourceW(exe, False)
    if not u:
        raise OSError(ctypes.get_last_error(), "cannot update " + exe)
    for n, lid in old:
        k32.UpdateResourceW(u, RT_MANIFEST, n, lid, None, 0)
    buf = ctypes.create_string_buffer(manifest, len(manifest))
    ok = k32.UpdateResourceW(u, RT_MANIFEST, 1, 0, buf, len(manifest))
    if not ok or not k32.EndUpdateResourceW(u, False):
        raise OSError(ctypes.get_last_error(), "cannot write the manifest into " + exe)
    print("embed-manifest: %s (replaced %d)" % (exe, len(old)))


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    # A console in the machine's code page cannot print every path.
    sys.stdout.reconfigure(errors="replace")
    with open(argv[0], "rb") as f:
        manifest = f.read()
    for exe in argv[1:]:
        embed(manifest, exe)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
