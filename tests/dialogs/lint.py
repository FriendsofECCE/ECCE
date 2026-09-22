"""
Static lint for the ``scripts/codereg`` dialogs.

Catches the failures that need no dialog run at all -- currently one, the
bug class CLAUDE.md singles out:

    "Also check for missing commas in the Python list: adjacent string
     literals are implicitly concatenated, so `"N12SX (range)"
     "MN12SX (range)"` silently became one nonsense dropdown entry in all
     three dialogs while both real functionals became unselectable."

Python's own AST folds implicit concatenation into a single constant before
anything can look at it, so this works on the token stream instead: two
STRING tokens with no comma between them, inside a ``[...]`` list.

Long messages deliberately split across lines are the legitimate use of
implicit concatenation, and they are almost never inside a list literal --
which is why only list context is reported.  When one genuinely is, put it
in ``cases.KNOWN_CONCATENATION``.
"""

import io
import os
import re
import token
import tokenize


def concatenatedStrings(path):
    """[(line, first, second), ...] for implicit concatenation in a list."""
    with open(path, "rb") as handle:
        source = handle.read()
    found = []
    stack = []
    previous = None
    try:
        tokens = list(tokenize.tokenize(io.BytesIO(source).readline))
    except (tokenize.TokenError, SyntaxError) as exc:
        return [("?", "could not tokenise %s: %s" % (os.path.basename(path), exc),
                 "")]
    for tok in tokens:
        if tok.type in (token.NEWLINE, token.NL, token.COMMENT,
                        token.INDENT, token.DEDENT):
            continue
        if tok.type == token.OP:
            if tok.string in "([{":
                stack.append(tok.string)
            elif tok.string in ")]}" and stack:
                stack.pop()
        if (tok.type == token.STRING and previous is not None
                and previous.type == token.STRING
                and stack and stack[-1] == "["):
            found.append((tok.start[0], previous.string, tok.string))
        previous = tok
    return found


def lintDialogs(directory, known):
    """Yield (path, line, message) for every finding not in ``known``."""
    for name in sorted(os.listdir(directory)):
        if not name.endswith(".py"):
            continue
        path = os.path.join(directory, name)
        for line, first, second in concatenatedStrings(path):
            if (name, line) in known:
                continue
            yield (path, line,
                   "implicit string concatenation inside a list -- a missing "
                   "comma.\n      %s\n      %s\n      become the single entry "
                   "%s, so both intended entries are unselectable."
                   % (first, second, _joined(first, second)))


def _joined(first, second):
    try:
        return repr(eval(first) + eval(second))       # literals only
    except Exception:
        return "<the two concatenated>"

#  NWChem's range-separated functionals: "cam" is a dft directive of its own,
#  not part of the xc line.  ai.nwchem used to emit
#      XC xcamb88 1.00 lyp 0.81 vwn_5 0.19 hfexch 1.00 cam 0.33 cam_alpha ...
#  on one line, which nwchem rejects with "xc_input: invalid format" -- so
#  CAM-B3LYP and LC-wPBE aborted every job they were chosen for.  Verified
#  against nwchem 7.2.3; the fix puts cam on its own line, and this keeps it
#  there.
def camOnXcLine(path):
    """Yield (line, text) for an xc value that runs "cam" onto the xc line."""
    with open(path) as handle:
        for number, text in enumerate(handle, 1):
            match = re.search(r'\$result\s*=\s*"([^"]*)"', text)
            if not match:
                continue
            value = match.group(1)
            #  cam_alpha/cam_beta, not a bare "cam" -- "cam-s12g" and
            #  "cam-s12h" are functional NAMES that legitimately start with
            #  it, and both run fine on one line.
            if re.search(r'\bcam_(alpha|beta)\b', value) \
                    and "\\n" not in value:
                yield number, value
