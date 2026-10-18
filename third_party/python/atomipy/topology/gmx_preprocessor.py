"""
Minimal GROMACS topology preprocessor (decision §14.3).

Handles the directives real `.itp` files use — ``#include`` (with a search
path), ``#define``/``#undef``, ``#ifdef``/``#ifndef``/``#else``/``#endif`` — driven
by a caller-supplied ``defines`` set, mirroring atomipy's existing ``defines=[…]``
API. Object-like ``#define NAME value`` macros are substituted by whole-word
token replacement (covers ``#define gb_21 0.1 463`` style bonded macros).

NOT a full cpp: no function-like macros, no ``#if`` arithmetic. atomipy's own FF
also ships as JSON with the ``#ifdef`` k-variants pre-expanded, so this is only
needed for arbitrary external `.itp`.
"""
from __future__ import annotations

import os
import re
from typing import Dict, Iterable, List, Optional, Tuple

_TOKEN = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def _strip_comment(line: str) -> str:
    # GROMACS comments start with ';'
    i = line.find(";")
    return line if i < 0 else line[:i]


def preprocess(path: str, defines: Optional[Iterable[str] | Dict[str, str]] = None,
               include_dirs: Optional[List[str]] = None,
               max_depth: int = 50) -> List[str]:
    """Return the flattened, directive-free list of content lines (comments
    stripped, macros substituted)."""
    if isinstance(defines, dict):
        defs: Dict[str, str] = dict(defines)
    else:
        defs = {str(d): "" for d in (defines or [])}

    search = list(include_dirs or [])
    out: List[str] = []
    # cond stack entries: [active_here, any_branch_taken, seen_else]
    cond: List[List[bool]] = []

    def active() -> bool:
        return all(c[0] for c in cond)

    def find_include(name: str, cur_dir: str) -> Optional[str]:
        for d in [cur_dir] + search:
            cand = os.path.join(d, name)
            if os.path.isfile(cand):
                return cand
        # also try bare name relative to cwd
        return name if os.path.isfile(name) else None

    def subst(line: str) -> str:
        if not defs:
            return line

        def repl(m):
            tok = m.group(0)
            v = defs.get(tok)
            return v if v else tok
        return _TOKEN.sub(repl, line)

    def process(fp: str, depth: int):
        if depth > max_depth:
            raise RecursionError(f"#include too deep at {fp}")
        cur_dir = os.path.dirname(os.path.abspath(fp))
        with open(fp, "r", encoding="utf-8", errors="replace") as fh:
            for raw in fh:
                s = raw.strip()
                if s.startswith("#"):
                    directive = s[1:].strip()
                    head = directive.split(None, 1)
                    key = head[0] if head else ""
                    rest = head[1].strip() if len(head) > 1 else ""
                    if key == "ifdef":
                        cond.append([active() and rest in defs, rest in defs, False])
                    elif key == "ifndef":
                        cond.append([active() and rest not in defs, rest not in defs, False])
                    elif key == "else":
                        if cond:
                            c = cond[-1]
                            c[0] = (all(x[0] for x in cond[:-1])) and (not c[1])
                            c[1] = True
                            c[2] = True
                    elif key == "endif":
                        if cond:
                            cond.pop()
                    elif not active():
                        continue
                    elif key == "include":
                        name = rest.strip().strip('"').strip("<>").strip()
                        inc = find_include(name, cur_dir)
                        if inc:
                            process(inc, depth + 1)
                        # silently skip unresolved includes (e.g. forcefield.itp not shipped)
                    elif key == "define":
                        parts = rest.split(None, 1)
                        if parts:
                            defs[parts[0]] = parts[1].strip() if len(parts) > 1 else ""
                    elif key == "undef":
                        defs.pop(rest.strip(), None)
                    # ignore #if / #pragma / unknown
                    continue
                if not active():
                    continue
                body = _strip_comment(raw).rstrip()
                if body.strip():
                    out.append(subst(body))

    process(path, 0)
    return out


def sections(lines: List[str]) -> List[Tuple[str, List[str]]]:
    """Group preprocessed lines into [section_name] -> data lines."""
    result: List[Tuple[str, List[str]]] = []
    cur_name: Optional[str] = None
    cur: List[str] = []
    for ln in lines:
        m = re.match(r"\s*\[\s*([A-Za-z0-9_]+)\s*\]\s*$", ln)
        if m:
            if cur_name is not None:
                result.append((cur_name, cur))
            cur_name = m.group(1).lower()
            cur = []
        elif cur_name is not None:
            if ln.strip():
                cur.append(ln)
    if cur_name is not None:
        result.append((cur_name, cur))
    return result
