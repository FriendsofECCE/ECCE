"""csh -> POSIX sh for the shell snippets of ECCE's job-script configuration
(setup, wrapup, <Code>Command, <Code>_loophole in submit.site and CONFIG.*).

Used by ecce-csh2sh (structured mode: also rewrites if/foreach/while and
source).  scripts/gensub, which is Perl, still carries its own copy of these
rules in CshToSh.pm; tests/csh2sh compares the two so they cannot drift.  A
construct is converted only when its sh form means exactly the same;
everything else is left and reported.

Text is handled as latin-1 so that file bytes pass through unchanged, and the
regular expressions are ASCII-only, as Perl's are on byte strings.
"""

import os
import re

_A = re.ASCII

# Constructs with no mechanical translation, each with the sh form to write.
# First match wins.  Guessing wrong here runs the wrong job.
CSH_ONLY = [
    (r'^if\s*\(', 'if [ ... ]; then ... fi'),
    (r'^else\s+if\b', 'elif [ ... ]; then'),
    (r'^(?:endif|endsw|end)\s*$', 'fi, done or esac'),
    (r'^foreach\b', 'for v in ...; do ... done'),
    (r'^while\s*\(', 'while [ ... ]; do ... done'),
    (r'^(?:switch|case|default:|breaksw)\b', 'case ... in pattern) ... ;; esac'),
    (r'^repeat\b', 'a for loop'),
    (r'^@\s', 'x=$((expression))'),
    (r'^set\s+\w+\s*=\s*\(', 'a string, e.g. x="a b" (PATH="dir:$PATH" for path)'),
    (r'^set\s+\w+\s*=\s*\S+\s+\S', 'x="value with spaces"'),
    (r'^set\s+\w+\s*$', 'x=""'),
    (r'^setenv\b', 'export NAME=value'),
    (r'^source\s', '. file   (the file itself must be sh syntax)'),
    (r'^(?:limit|unlimit)\b', 'ulimit'),
    (r'^(?:alias|unalias|rehash|onintr|goto)\b', "the sh equivalent (alias n='cmd', trap)"),
    (r'\$\??\{?status\b', '$?'),
    (r'\$\{?\?\w', '${NAME+set} (or ${NAME:+set} for non-empty)'),
    (r'\$#?argv\b', '$1, $#, "$@"'),
    (r'\$\w+:[htre](?![\w])', '${x%/*} for :h, ${x##*/} for :t'),
    (r'\$\{\w+:[htre]\}', '${x%/*} for :h, ${x##*/} for :t'),
    (r'\$#\w+', 'sh has no arrays; ${#x} is a string length (use "$#" for argument counts)'),
    (r'\$\{?\w+\}?\[', 'sh has no arrays; keep one value per variable or loop over a list'),
]
CSH_ONLY = [(re.compile(p, _A), advice) for p, advice in CSH_ONLY]

_ONE_WORD = r'''(?:"[^"]*"|'[^']*'|`[^`]*`|[^\s"'`]\S*)'''

_SETENV2 = re.compile(r'^setenv\s+(\w+)\s+(%s)$' % _ONE_WORD, _A)
_SETENV1 = re.compile(r'^setenv\s+(\w+)$', _A)
_UNSETENV = re.compile(r'^unsetenv\s+(\w+)$', _A)
_SET = re.compile(r'^set\s+(\w+)\s*=\s*(%s)\s*(;?\s*)$' % _ONE_WORD, _A)
_EXIT = re.compile(r'^exit\s*\(\s*(\d+)\s*\)$', _A)


def strip(s):
    return re.sub(r'^\s+|\s+$', '', s, flags=_A)


def refusal(body):
    """The advice for a (stripped) csh-only statement, or None."""
    for rx, advice in CSH_ONLY:
        if rx.search(body):
            return advice
    return None


def translate_line(body):
    """The plain forms: setenv, set N = V, unsetenv, exit (n), csh redirections."""
    m = _SETENV2.match(body)
    if m:
        body = "export %s=%s" % (m.group(1), m.group(2))
    else:
        m = _SETENV1.match(body)
        if m:
            body = "export %s=" % m.group(1)
        else:
            m = _UNSETENV.match(body)
            if m:
                body = "unset %s" % m.group(1)
            else:
                m = _SET.match(body)
                if m and not m.group(2).startswith("("):
                    body = "%s=%s" % (m.group(1), m.group(2))
                else:
                    m = _EXIT.match(body)
                    if m:
                        body = "exit %s" % m.group(1)
    body = re.sub(r'>>&\s*(?![0-9]\b)(\S+)', lambda m: ">> " + m.group(1) + " 2>&1",
                  body, flags=_A)
    body = re.sub(r'(?<![0-9&])>&!?\s*(?![0-9]\b)(\S+)',
                  lambda m: "> " + m.group(1) + " 2>&1", body, flags=_A)
    body = body.replace("|&", "2>&1 |")
    body = body.replace(">>!", ">>")
    body = body.replace(">!", ">")
    return body


def sh_twin(path):
    """The sh file a csh init file is shipped beside, by naming convention:
    x.csh -> x.sh, and modules' init/csh -> init/sh."""
    i = path.rfind("/")
    d, base = path[:i + 1], path[i + 1:]
    m = re.match(r'^(.+)\.csh$', base, _A)
    if m:
        return d + m.group(1) + ".sh"
    if base == "csh":
        return d + "sh"
    return None


def _balanced(s):
    """"(...)" at the start of s, parentheses balanced, quotes respected.
    Returns (inner, rest) or None."""
    if not re.match(r'^\s*\(', s, _A):
        return None
    s = re.sub(r'^\s*', '', s, count=1, flags=_A)
    depth, i, n = 0, 0, len(s)
    while i < n:
        c = s[i]
        if c in '"\'`':
            j = s.find(c, i + 1)
            if j < 0:
                return None
            i = j
        elif c == '\\':
            i += 1
        elif c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return s[1:i], s[i + 1:]
        i += 1
    return None


# --- conditions: csh expression -> sh test commands ------------------------------

class _CondError(Exception):
    pass


_FILETEST = {'e': '-e', 'd': '-d', 'f': '-f', 'r': '-r', 'w': '-w',
             'x': '-x', 's': '-s', 'l': '-L'}
_NUMOP = {'<': '-lt', '>': '-gt', '<=': '-le', '>=': '-ge'}

_TOK_OP = re.compile(r'\|\||&&|==|!=|=~|!~|<=|>=|<|>|\(|\)|!')
_TOK_WORD = re.compile(
    r'''(?:"(?:[^"\\]|\\.)*"|'[^']*'|`[^`]*`|[^\s()&|!=<>"'`]+)+''', _A)
_TOK_SPACE = re.compile(r'\s+', _A)


def _tokens(s):
    """-> (tokens, None) or (None, reason)."""
    t = []
    pos = 0
    while pos < len(s):
        m = _TOK_SPACE.match(s, pos)
        if m:
            pos = m.end()
            continue
        m = _TOK_OP.match(s, pos)
        if m:
            t.append(('op', m.group(0)))
            pos = m.end()
            continue
        m = _TOK_WORD.match(s, pos)
        if m:
            t.append(('w', m.group(0)))
            pos = m.end()
            continue
        return None, "cannot read the condition near '%s'" % s[pos:pos + 12]
    return t, None


_PART = re.compile(r'''("(?:[^"\\]|\\.)*"|'[^']*'|`[^`]*`|[^"'`]+)''', _A)


def _operand(w, status_ok):
    """A word as an sh operand, always quoted.  Raises _CondError with the reason
    when its meaning in csh is not certain."""
    out = ''
    for p in _PART.findall(w):
        if p.startswith('"') or p.startswith("'"):
            if p.startswith('"') and re.search(r'\\!|\$\??\{?status\b', p, _A):
                raise _CondError("csh history or status syntax inside quotes")
            out += p
        elif re.match(r'^`(.*)`$', p, _A | re.S):
            out += '"$(' + p[1:-1] + ')"'
        elif p == '$status' or p == '${status}':
            if not status_ok:
                raise _CondError("$status here is not certain to mean the previous command")
            out += '"$?"'
        elif re.match(r'^[\w$/.{}+:,@%^-]+$', p, _A):
            if re.search(r'\$\{?\w+\}?(?::[a-z]|\[)', p, _A) or '$#' in p:
                raise _CondError("a modifier or array form in '%s'" % p)
            out += '"%s"' % p if '$' in p else p
        else:
            raise _CondError("the word '%s' has a pattern, tilde or special character "
                             "whose csh meaning is not certain" % p)
    return out


def _numeric_operand(w):
    if re.match(r'^\d+$', w, _A):
        return w
    if re.match(r'^\$\w+$', w, _A) or re.match(r'^\$\{\w+\}$', w, _A):
        return '"%s"' % w
    if re.match(r'^("\$\w+"|"\$\{\w+\}")$', w, _A):
        return w
    raise _CondError("'%s' is not a number or a plain variable" % w)


def _parse_cond(toks, status_ok):
    st = {'pos': 0}

    def at(i):
        return toks[i] if i < len(toks) else None

    def is_op(t, v):
        return t is not None and t[0] == 'op' and t[1] == v

    def primary():
        t = at(st['pos'])
        if t is None:
            raise _CondError("the condition ends too soon")
        if t[0] == 'op' and t[1] == '(':
            st['pos'] += 1
            n = or_()
            c = at(st['pos'])
            st['pos'] += 1
            if not (c is not None and c[0] == 'op' and c[1] == ')'):
                raise _CondError("missing )")
            return n
        if t[0] == 'op':
            raise _CondError("unexpected '%s'" % t[1])
        w = t[1]
        nxt = at(st['pos'] + 1)
        m = re.match(r'^-([a-z])$', w, _A)
        if m and nxt is not None and nxt[0] == 'w':
            k = m.group(1)
            if k not in _FILETEST:
                raise _CondError("file test -%s has no certain sh equivalent here" % k)
            st['pos'] += 2
            op = _operand(nxt[1], status_ok)
            if re.search(r'[*?\[]', nxt[1]):
                raise _CondError("file test on a pattern")
            return ('atom', "[ %s %s ]" % (_FILETEST[k], op))
        if nxt is not None and nxt[0] == 'op' and nxt[1] in ('==', '!=', '<=', '>=', '<', '>'):
            op = nxt[1]
            rt = at(st['pos'] + 2)
            if not (rt is not None and rt[0] == 'w'):
                raise _CondError("a comparison without a right-hand side")
            st['pos'] += 3
            if op in ('==', '!='):
                if re.search(r'(?<![\\])[*?\[]', rt[1]) and not re.match(r'''^["'`]''', rt[1]):
                    raise _CondError("a pattern on the right of %s" % op)
                l = _operand(w, status_ok)
                r = _operand(rt[1], status_ok)
                return ('atom', "[ %s %s %s ]" % (l, '=' if op == '==' else '!=', r))
            return ('atom', "[ %s %s %s ]" % (_numeric_operand(w), _NUMOP[op],
                                              _numeric_operand(rt[1])))
        if nxt is not None and nxt[0] == 'op' and nxt[1] in ('=~', '!~'):
            raise _CondError("pattern match (%s) has no certain sh equivalent" % nxt[1])
        st['pos'] += 1
        m = re.match(r'^\$\?\{?(\w+)\}?$', w, _A)
        if m:
            return ('atom', '[ -n "${%s+set}" ]' % m.group(1))
        if re.match(r'^\d+$', w, _A):
            return ('atom', 'false' if int(w) == 0 else 'true')
        if w == '$status':
            if not status_ok:
                raise _CondError("$status here is not certain to mean the previous command")
            return ('atom', '[ "$?" -ne 0 ]')
        raise _CondError("'%s' alone is not a condition that means the same in sh" % w)

    def not_():
        t = at(st['pos'])
        if is_op(t, '!'):
            st['pos'] += 1
            return ('not', not_())
        return primary()

    def and_():
        n = [not_()]
        while is_op(at(st['pos']), '&&'):
            st['pos'] += 1
            n.append(not_())
        return n[0] if len(n) == 1 else ('and',) + tuple(n)

    def or_():
        n = [and_()]
        while is_op(at(st['pos']), '||'):
            st['pos'] += 1
            n.append(and_())
        return n[0] if len(n) == 1 else ('or',) + tuple(n)

    tree = or_()
    if st['pos'] < len(toks):
        raise _CondError("unexpected '%s'" % toks[st['pos']][1])
    return tree


def _emit(n):
    """sh's && and || have equal precedence, csh's do not: mixed levels get braces."""
    k, c = n[0], n[1:]
    if k == 'atom':
        return c[0]
    if k == 'not':
        inner = c[0]
        if inner[0] != 'not':
            return '! ' + (inner[1] if inner[0] == 'atom' else '{ ' + _emit(inner) + '; }')
        return '! { ' + _emit(inner) + '; }'
    other = 'or' if k == 'and' else 'and'
    join = ' && ' if k == 'and' else ' || '
    return join.join('{ ' + _emit(x) + '; }' if x[0] == other else _emit(x) for x in c)


def convert_condition(expr, status_ok=False):
    """csh expression text -> (sh command, None) or (None, reason)."""
    toks, err = _tokens(expr)
    if toks is None:
        return None, err
    if not toks:
        return None, "an empty condition"
    try:
        tree = _parse_cond(toks, status_ok)
    except _CondError as e:
        return None, str(e)
    return _emit(tree), None


# --- snippets -------------------------------------------------------------------

_SOURCE = re.compile(r'^source\s+(\S+)\s*(#.*)?$', _A)
_SOURCE_PATH = re.compile(r'^[\w$/.{}~+:,@%-]+$', _A)
_STATUS = re.compile(r'\$\{?status\b', _A)
_STATUS_SUB = re.compile(r'\$\{?status\}?(?![\w])', _A)
_WORD_BAD = re.compile(r'\$#|\$\?|\$\{?\w+\}?(?::[a-z]|\[)|\$status|\$argv', _A)


def snippet(text, structured=False, assume_twins=False, exists=None):
    """Convert one snippet.  structured: also convert if/foreach/while, $status
    and source.  Returns a dict {text, changes: [{line, from, to}], issues:
    [{line, text, want, why}]} with one output line per input line, so line
    numbers survive."""
    res = {'text': text, 'changes': [], 'issues': []}
    if text is None or text == "":
        return res
    if exists is None:
        exists = lambda p: os.path.exists(p.encode('latin-1'))

    lines = text.split("\n")
    new = list(lines)
    skip = {}
    why = {}
    stack = []
    heredoc = None
    after_opener = False

    def bad(e):
        e['ok'] = False
        e['edits'] = []

    def simple(body, first):
        if structured:
            m = _SOURCE.match(body)
            if m:
                path = m.group(1)
                cmt = " " + m.group(2) if m.group(2) is not None else ""
                twin = sh_twin(path) if _SOURCE_PATH.match(path) else None
                if twin is None:
                    return body, "no sh twin is known for this file name; write the sh version of its contents"
                lit = not re.search(r'[$`]', path)
                if assume_twins or (lit and exists(
                        (os.environ.get('HOME') or '') + twin[1:] if twin.startswith('~') else twin)):
                    if '/' not in twin:
                        if '$' in twin:
                            return body, "give the sh twin with a path (. ./%s)" % twin
                        twin = "./" + twin
                    return ". %s%s" % (twin, cmt), None
                if lit:
                    return body, ("%s not found on this machine; if it exists where the job runs, "
                                  "use: . %s (or run ecce-csh2sh with --assume-sh-twins)" % (twin, twin))
                return body, ("the path has a variable, so %s cannot be checked; if it exists where "
                              "the job runs, use: . %s (or --assume-sh-twins)" % (twin, twin))
            if _STATUS.search(body):
                if first or "'" in body:
                    return body, ("$status is only converted in a command that follows another, "
                                  "never as the first line of a block")
                body = _STATUS_SUB.sub(lambda m: "$?", body)
        return translate_line(body), None

    def open_(kind, ok):
        e = {'kind': kind, 'ok': ok, 'edits': []}
        stack.append(e)
        return e

    for i, line in enumerate(lines):
        if heredoc is not None:
            skip[i] = True
            if re.match(r'^\s*%s\s*$' % re.escape(heredoc), line, _A):
                heredoc = None
            continue
        if re.match(r'^\s*(?:#|$)', line, _A):
            skip[i] = True
            continue

        m = re.match(r'^(\s*)(.*?)\s*$', line, _A)
        ind, body = m.group(1), m.group(2)
        first = after_opener
        after_opener = False
        m = re.search(r'''<<-?\s*['"]?(\w+)['"]?''', body, _A)
        if m:
            heredoc = m.group(1)

        if structured:
            top = stack[-1] if stack else None

            # A block statement continued over lines is not rewritten, only reported.
            if body.endswith("\\"):
                m = re.match(r'^(if|else\s+if|while|foreach)\b', body, _A)
                if m:
                    kw = m.group(1)
                    why[i] = "continued over several lines; ecce-csh2sh does not rewrite those"
                    if kw == 'else if':
                        if top:
                            bad(top)
                    elif kw == 'if':
                        if re.search(r'\bthen\b', body, _A):
                            open_('if', False)
                    else:
                        open_(kw, False)
                    after_opener = True
                    continue

            m = re.match(r'^(if|else\s+if|while)\s*\(', body, _A)
            if m:
                kw = re.sub(r'\s+', ' ', m.group(1), count=1, flags=_A)
                b = _balanced(re.sub(r'^(?:if|else\s+if|while)\s*', '', body, count=1, flags=_A))
                if b is None:
                    why[i] = "unbalanced parentheses"
                    if kw in ('if', 'while'):
                        open_('while' if kw == 'while' else 'if', False)
                    elif top:
                        bad(top)
                    after_opener = True
                    continue
                inner, rest = b
                rest = re.sub(r'^\s+', '', rest, count=1, flags=_A)
                mb = re.match(r'^then\s*(#.*)?$', rest, _A)
                is_block = mb is not None
                cmt = " " + mb.group(1) if (is_block and mb.group(1) is not None) else ""
                if kw == 'while':
                    e = open_('while', True)
                    c, r = convert_condition(inner, status_ok=False)
                    if c is None or (rest != '' and not rest.startswith('#')):
                        why[i] = r if r is not None else "text after the condition"
                        bad(e)
                    else:
                        e['edits'].append((i, ind + "while %s; do" % c +
                                           (" " + rest if rest.startswith('#') else "")))
                    after_opener = True
                    continue
                if kw == 'else if':
                    if not top or top['kind'] != 'if':
                        why[i] = "else if outside an if block"
                        after_opener = True
                        continue
                    if not is_block:
                        why[i] = "else if without then"
                        bad(top)
                        after_opener = True
                        continue
                    c, r = convert_condition(inner, status_ok=False)
                    if c is None:
                        why[i] = r
                        bad(top)
                    elif top['ok']:
                        top['edits'].append((i, ind + "elif %s; then%s" % (c, cmt)))
                    after_opener = True
                    continue
                # kw == 'if'
                if is_block:
                    e = open_('if', True)
                    c, r = convert_condition(inner, status_ok=not first)
                    if c is None:
                        why[i] = r
                        bad(e)
                    else:
                        e['edits'].append((i, ind + "if %s; then%s" % (c, cmt)))
                    after_opener = True
                    continue
                # if (expr) command
                if (rest == '' or re.match(r'^[;#]', rest) or re.match(r'^then\b', rest, _A)
                        or re.search(r'[;|]|&&', rest)):
                    why[i] = "the command after the condition is not a single simple command"
                    continue
                c, r = convert_condition(inner, status_ok=not first)
                if c is None:
                    why[i] = r
                    continue
                cmd, cwhy = simple(rest, True)
                if cwhy or refusal(cmd) is not None:
                    why[i] = cwhy or "the command is csh syntax"
                    continue
                new[i] = ind + "if %s; then %s; fi" % (c, cmd)
                continue

            m = re.match(r'^else\s*(#.*)?$', body, _A)
            if m:
                after_opener = True
                if not top or top['kind'] != 'if':
                    why[i] = "else outside an if block"
                continue

            m = re.match(r'^endif\s*(#.*)?$', body, _A)
            if m:
                cmt = " " + m.group(1) if m.group(1) is not None else ""
                if top and top['kind'] == 'if':
                    stack.pop()
                    if top['ok']:
                        for k, v in top['edits']:
                            new[k] = v
                        new[i] = ind + "fi" + cmt
                else:
                    why[i] = "endif without a matching if"
                continue

            m = re.match(r'^foreach\s+(\w+)\s*(\(.*)$', body, _A)
            if m:
                var = m.group(1)
                b = _balanced(m.group(2))
                e = open_('foreach', True)
                after_opener = True
                mr = re.match(r'^\s*(#.*)?$', b[1], _A) if b else None
                if b is None or mr is None:
                    why[i] = "cannot read the word list"
                    bad(e)
                    continue
                cmt = " " + mr.group(1) if mr.group(1) is not None else ""
                toks, err = _tokens("x " + b[0])    # words only
                words = []
                fail = err
                if not fail:
                    for t in toks[1:]:
                        if t[0] != 'w':
                            fail = "the word list contains '%s'" % t[1]
                            break
                        w = t[1]
                        if _WORD_BAD.search(w):
                            fail = "'%s' uses a csh-only form" % w
                            break
                        if w.startswith('`') and re.match(r'^`(.*)`$', w, _A | re.S):
                            words.append(w)
                        elif re.match(r'''^["']''', w):
                            words.append(w)
                        elif re.search(r'[*?\[]', w):
                            words.append(w)
                        elif '$' in w:
                            words.append('"%s"' % w)
                        else:
                            words.append(w)
                    if not fail and not words:
                        fail = "an empty word list"
                if fail:
                    why[i] = fail
                    bad(e)
                    continue
                e['edits'].append((i, ind + "for %s in %s; do%s" % (var, ' '.join(words), cmt)))
                continue

            m = re.match(r'^end\s*(#.*)?$', body, _A)
            if m:
                cmt = " " + m.group(1) if m.group(1) is not None else ""
                if top and top['kind'] in ('foreach', 'while'):
                    stack.pop()
                    if top['ok']:
                        for k, v in top['edits']:
                            new[k] = v
                        new[i] = ind + "done" + cmt
                else:
                    why[i] = "end without a matching foreach or while"
                continue

        nb, w = simple(body, first)
        if w is not None:
            why[i] = w
        if nb != body:
            new[i] = ind + nb

    # Edits of blocks never closed stay unapplied; their lines are reported below.
    issues = []
    for i in range(len(lines)):
        if skip.get(i):
            continue
        body = re.match(r'^(\s*)(.*?)\s*$', new[i], _A).group(2)
        want = refusal(body)
        if want is None:
            continue
        issues.append({'line': i + 1, 'text': strip(lines[i]), 'want': want, 'why': why.get(i)})
    changes = []
    for i in range(len(lines)):
        if new[i] == lines[i]:
            continue
        changes.append({'line': i + 1, 'from': strip(lines[i]), 'to': strip(new[i])})
    return {'text': "\n".join(new), 'changes': changes, 'issues': issues}
