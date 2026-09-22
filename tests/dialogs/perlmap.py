r"""
Extract, from an ``ai.<code>`` input generator, the set of strings it will
actually recognise for each GUIValues key.

Why this is worth doing mechanically
------------------------------------
A ``scripts/codereg/*theory.py`` combo list and the ``ai.<code>`` subroutine
that translates its selections are two hand-maintained copies of one list.
Every translation sub ends in ``else { $result = ""; }``, so when the two
drift the generated ``##token##`` line is simply deleted and **the job runs
without the functional the user picked** -- no error, no warning, anywhere.
Nine instances of exactly this were found by eye in the ged{03,09,16}
dialogs in one afternoon (2026-09-21): letter ``O`` for zero (``MO6HF`` vs
``M06HF``), a missing closing paren, wrong case, a stray space, a dialog
entry with no generator case at all, and -- the nastiest -- a missing comma
in the Python list, which silently concatenated two adjacent string literals
into one nonsense entry and made both real functionals unselectable.

Finding them by eye does not scale and does not stay found.  Diffing the two
sets does both.

How the reads are traced
------------------------
``ai.<code>``'s ``readDict`` stores every dotted *suffix* of each key, so
``ES.Theory.DFT.XCFunctionals`` in the dialog is reachable in Perl as
``$AbiDict{"XCFunctionals"}``, ``$AbiDict{"DFT.XCFunctionals"}`` and so on.
Matching is therefore by suffix, not equality.

Four read shapes cover these ten generators, which all descend from one
original and are stylistically uniform:

    if ($AbiDict{"K"} eq "LIT")              direct comparison
    $result .= &Sub($AbiDict{"K"})           passed straight to a translator
    $v = $AbiDict{"K"}; ... &Sub($v)         via a scalar
    ##K## in <code>.tpl                      resolved by the template engine

The last is the important one and is easy to miss by reading the Perl alone:
``modifyInputFile`` resolves a template tag by calling a subroutine *named
after the tag* with the dictionary value as its argument --

    $subname = $key;  $subname =~ s/\./_/g;
    $fct = "&$subname('$value')";  $result = eval $fct;

-- so ``##XCFunctionals##`` in ``nwch.tpl`` is what invokes
``sub XCFunctionals`` in ``ai.nwchem``, which has no textual call site
anywhere.  Tracing that requires reading the ``.tpl``, which is why
``acceptedForKey`` takes the template's tag list.

Two translator shapes are recognised inside a resolver: the ``eq`` chain that
most of these scripts use, and the ``%map = (...)`` hash that the newer ones
(``ai.orca``) use.  They differ in what happens on a miss, and that
difference is the whole severity question -- see ``Fallback``.

For the two indirect shapes the argument position is mapped onto the callee's
``local($a, $b) = @_;`` parameter list and the comparisons against that
parameter are collected.  One level of indirection is deliberate: these
scripts do not nest translators deeper than that, and a tracer that silently
followed arbitrary depth would be harder to trust than to re-read.

Anything this module cannot trace is reported as untraced rather than
assumed empty -- an unreadable key must never look like a clean result.
"""

import re

# $AbiDict{"Key"} / $AbiDict{'Key'}
RE_ABIDICT = re.compile(r'\$AbiDict\{\s*["\']([^"\']+)["\']\s*\}')
# sub Name {
RE_SUB = re.compile(r'^\s*sub\s+(\w+)\s*\{', re.M)
# local($a, $b) = @_;   my ($a) = @_;
RE_PARAMS = re.compile(r'^\s*(?:local|my)\s*\(([^)]*)\)\s*=\s*@_\s*;', re.M)
# $var = $AbiDict{"Key"};
RE_ASSIGN = re.compile(
    r'\$(\w+)\s*=\s*\$AbiDict\{\s*["\']([^"\']+)["\']\s*\}\s*;')
# $x eq "literal"  /  $x ne "literal"
RE_CMP = re.compile(r'\$(\w+)\s*(?:eq|ne)\s*["\']([^"\']*)["\']')
# "literal" eq $x   (the reversed form; rare but present)
RE_CMP_REV = re.compile(r'["\']([^"\']*)["\']\s*(?:eq|ne)\s*\$(\w+)')
# $x =~ /regex/
RE_MATCH = re.compile(r'\$(\w+)\s*=~\s*[/m]')


def stripComments(text):
    """Remove Perl # comments, leaving string literals alone.

    Crude but adequate here: the only thing that matters is that a commented
    out ``eq "..."`` is not counted as a supported value, and that a ``#``
    inside a quoted string does not truncate the line.
    """
    out = []
    for line in text.splitlines():
        result = []
        quote = None
        index = 0
        while index < len(line):
            char = line[index]
            if quote:
                if char == "\\":
                    result.append(line[index:index + 2])
                    index += 2
                    continue
                if char == quote:
                    quote = None
            elif char in "\"'":
                quote = char
            elif char == "#":
                break
            result.append(char)
            index += 1
        out.append("".join(result))
    return "\n".join(out)


class Fallback(object):
    """What a resolver does with a string it does not recognise.

    This is the severity axis.  ``SILENT`` is the shape that has cost this
    project real debugging time: the user picks a functional, the generator
    recognises nothing, the tag resolves to empty, and the job runs happily
    with a different method than the one on screen.
    """

    SILENT = "silent"            # else { $result = ""; }  -> setting vanishes
    DEFAULTS_EMPTY = "defaults-empty"   # opt-in switch; empty IS the default
    PASSTHROUGH = "passthrough"  # $map{$x} || $x          -> forwarded verbatim
    DIES = "dies"                # else { die ... }        -> reported (since #92)
    INLINE = "inline"            # compared in place, no single resolver
    UNKNOWN = "unknown"


class Trace(object):
    """The result of tracing one dialog key into one generator."""

    def __init__(self, key):
        self.key = key
        self.literals = set()
        self.sites = set()
        self.fallbacks = set()
        self.untraced = []      # found a translator but cannot enumerate it
        self.freeform = []      # value is used without being enumerated

    def add(self, literal, site, fallback):
        if literal == "":
            # `$x ne ""` is an emptiness guard, not an accepted value; counting
            # it would make a purely free-form key look enumerable.
            return
        self.literals.add(literal)
        self.sites.add(site)
        self.fallbacks.add(fallback)

    @property
    def enumerable(self):
        """True when the generator has a finite, diffable accepted set.

        When it does not -- the value is interpolated verbatim, matched by
        regex, or has no resolver at all -- there is nothing to diff, and
        saying so is very different from reporting every offered value as
        unmapped.
        """
        return bool(self.literals) and not self.untraced

    @property
    def severity(self):
        if Fallback.DEFAULTS_EMPTY in self.fallbacks:
            return Fallback.DEFAULTS_EMPTY
        if Fallback.SILENT in self.fallbacks:
            return Fallback.SILENT
        if Fallback.PASSTHROUGH in self.fallbacks:
            return Fallback.PASSTHROUGH
        if Fallback.DIES in self.fallbacks:
            return Fallback.DIES
        return Fallback.UNKNOWN


class Generator(object):
    """One parsed ``ai.<code>`` script."""

    def __init__(self, path):
        self.path = path
        with open(path, "r", errors="replace") as handle:
            self.text = stripComments(handle.read())
        self.subs = self._splitSubs()
        self.keys = sorted(set(RE_ABIDICT.findall(self.text)))

    # -- structure ---------------------------------------------------------

    def _splitSubs(self):
        """name -> {'body', 'params', 'start'} by brace matching."""
        subs = {}
        for match in RE_SUB.finditer(self.text):
            name = match.group(1)
            depth = 0
            index = match.end() - 1
            while index < len(self.text):
                if self.text[index] == "{":
                    depth += 1
                elif self.text[index] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                index += 1
            body = self.text[match.end():index]
            paramMatch = RE_PARAMS.search(body)
            params = []
            if paramMatch:
                params = [p.strip().lstrip("$")
                          for p in paramMatch.group(1).split(",")
                          if p.strip()]
            # A sub redefined later in the file wins in Perl; keep the last.
            subs[name] = {"body": body, "params": params,
                          "start": match.start()}
        return subs

    def _lineOf(self, offset):
        return self.text.count("\n", 0, offset) + 1

    # -- tracing -----------------------------------------------------------

    def acceptedForKey(self, key, tags=()):
        """What this generator recognises for the dialog key ``key``.

        ``tags`` is the set of ``##tag##`` names in the code's ``.tpl``; it is
        what lets the template engine's implicit "sub named after the tag"
        call be followed.  Returns a ``Trace``.
        """
        trace = Trace(key)
        # Longest alias first, and stop at the first one that yields anything.
        # readDict() stores EVERY dotted suffix, so `ES.Theory.NWPW.XCFunctionals`
        # and `ES.Theory.DFT.XCFunctionals` both also define
        # $AbiDict{"XCFunctionals"} -- unioning the aliases would credit each
        # key with the other's accepted values.  The generator's own choice of
        # alias is the specific one.
        for alias in self._aliases(key):
            self._traceAlias(alias, tags, trace)
            if trace.literals or trace.untraced:
                break
            if alias in tags:
                # The generator names this key in its own template, so this is
                # the alias it uses.  Stop even when nothing enumerable came
                # back -- a shorter alias that happens to be another key's
                # name is not this key's mapping.
                break
        return trace

    def _aliases(self, key):
        """Perl-visible names for a dotted dialog key, per readDict()."""
        parts = key.split(".")
        return [".".join(parts[i:]) for i in range(len(parts))]

    def _traceAlias(self, alias, tags, trace):
        pattern = (r'\$AbiDict\{\s*["\']' + re.escape(alias) + r'["\']\s*\}')

        # (a) compared in place: $AbiDict{"K"} eq "LIT"
        for match in re.finditer(
                pattern + r'\s*(?:eq|ne)\s*["\']([^"\']*)["\']', self.text):
            trace.add(match.group(1),
                      "%s:%d inline" % (self.name, self._lineOf(match.start())),
                      Fallback.INLINE)

        # A key ALSO matched with =~ is matched by substring, not equality
        # (ai.nwchem's Cosmo() mixes `eq "Water"` with `=~ "Methanol"` against
        # dialog strings like "Methanol: 33.0 Debye").  An exact-set diff is
        # simply the wrong question for such a key, so do not pretend to one.
        if re.search(pattern + r'\s*=~', self.text):
            trace.untraced.append(
                "%s is matched with =~ (substring), not compared for equality "
                "-- an exact-set diff does not apply" % alias)

        # Concatenated into the output anywhere means the else branch forwards
        # unrecognised values verbatim (ai.gauss16:1092 does
        # `$options .= "Solvent=".$AbiDict{"ES.Theory.SCF.Solvent"}` after an
        # eq chain that only special-cases "Manual" and ""), so the eq cases
        # are special handling, not an accepted-value list.  Truthiness tests
        # and `&Sub($AbiDict{...})` calls are deliberately NOT value uses.
        if (re.search(r'\.\s*' + pattern, self.text)
                or re.search(pattern + r'\s*\.[^=]', self.text)):
            trace.untraced.append(
                "%s is concatenated into the generated input directly, so "
                "unrecognised values are forwarded verbatim rather than "
                "enumerated" % alias)

        # (b) passed straight into a sub: &Sub(..., $AbiDict{"K"}, ...)
        for sub, position in self._callsWithArg(pattern):
            self._collectFromSub(sub, position, trace,
                                 "&%s($AbiDict{\"%s\"})" % (sub, alias))

        # (c) via a scalar: $v = $AbiDict{"K"}; ... &Sub($v)
        for match in RE_ASSIGN.finditer(self.text):
            if match.group(2) != alias:
                continue
            var = match.group(1)
            before = len(trace.literals)
            # Only within the subroutine holding the assignment: these scripts
            # reuse generic scalar names ($result, $value) everywhere, and a
            # file-wide search credits a key with comparisons that have nothing
            # to do with it.
            scope, scopeStart = self._scopeOf(match.start())
            for check in (RE_CMP, RE_CMP_REV):
                for cmp_ in check.finditer(scope):
                    groups = cmp_.groups()
                    name, value = ((groups[0], groups[1]) if check is RE_CMP
                                   else (groups[1], groups[0]))
                    if name == var:
                        trace.add(value, "%s:%d via $%s"
                                  % (self.name,
                                     self._lineOf(scopeStart + cmp_.start()),
                                     var),
                                  Fallback.INLINE)
            for sub, position in self._callsWithArg(r'\$' + re.escape(var) + r'\b'):
                self._collectFromSub(sub, position, trace,
                                     "&%s($%s)" % (sub, var))
            if len(trace.literals) == before:
                trace.freeform.append(
                    '%s:%d $%s = $AbiDict{"%s"}, never compared'
                    % (self.name, self._lineOf(match.start()), var, alias))

        # (d) the template engine's implicit call: ##alias## -> &alias_with_
        #     underscores($value).  See modifyInputFile().
        if alias in tags:
            sub = alias.replace(".", "_")
            if sub in self.subs:
                self._collectFromSub(sub, 0, trace,
                                     "##%s## -> &%s()" % (alias, sub))
            else:
                # No resolver: the engine substitutes the raw dialog string
                # into the deck verbatim, so every offered value "works".
                trace.freeform.append(
                    "##%s## has no &%s() resolver; the dialog string is "
                    "substituted verbatim" % (alias, sub))

    def _scopeOf(self, offset):
        """The subroutine body containing ``offset``, and where it starts."""
        best = None
        for info in self.subs.values():
            start = self.text.index(info["body"], info["start"])
            end = start + len(info["body"])
            if start <= offset < end:
                if best is None or start > best[1]:
                    best = (info["body"], start)
        if best is None:
            return self.text, 0
        return best

    def _callsWithArg(self, argPattern):
        """Yield (subName, argumentIndex) for &Sub(...) calls matching."""
        seen = set()
        for match in re.finditer(r'&?(\w+)\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\)',
                                 self.text):
            name = match.group(1)
            if name not in self.subs:
                continue
            for index, arg in enumerate(_splitArgs(match.group(2))):
                if re.search(argPattern, arg) and (name, index) not in seen:
                    seen.add((name, index))
                    yield name, index

    def _collectFromSub(self, subName, position, trace, via):
        """Collect the finite set of values ``subName`` recognises.

        Handles both translator shapes and records which one, because the
        shape determines what a miss costs:

          eq chain ending in ``else { $result = ""; }``
              the tag resolves to empty, the line is dropped, and the job
              silently runs without the setting the user chose;
          ``%map`` with ``|| $param``
              the unrecognised string is passed through verbatim, which the
              code itself will then accept or reject loudly.
        """
        info = self.subs.get(subName)
        if info is None:
            return
        if position >= len(info["params"]):
            trace.untraced.append(
                "&%s() argument %d has no matching parameter in its "
                "`local(...) = @_` list" % (subName, position + 1))
            return
        param = info["params"][position]
        body = info["body"]
        found = False

        # eq / ne chain
        for check in (RE_CMP, RE_CMP_REV):
            for match in check.finditer(body):
                groups = match.groups()
                name, value = ((groups[0], groups[1]) if check is RE_CMP
                               else (groups[1], groups[0]))
                if name == param:
                    trace.add(value, via, self._fallbackOf(subName))
                    found = True

        # %map = ( "key" => "value", ... ); ... $map{$param}
        for hashName, keys in self._hashTables(body):
            if re.search(r'\$' + re.escape(hashName) + r'\{\s*\$'
                         + re.escape(param) + r'\b', body):
                for key in keys:
                    trace.add(key, via + " %%%s" % hashName,
                              self._fallbackOf(subName))
                found = True

        if found:
            return
        if re.search(r'\$' + re.escape(param) + r'\s*=~', body):
            trace.untraced.append(
                "&%s() matches $%s by regex, not against literals -- no "
                "finite accepted set to diff" % (subName, param))
        else:
            trace.freeform.append(
                "&%s() uses $%s without comparing it to literals" % (subName, param))

    def _hashTables(self, body):
        """[(hashName, [key, ...]), ...] for ``my %h = ( "k" => v, ... )``."""
        found = []
        for match in re.finditer(r'(?:my|local|our)?\s*%(\w+)\s*=\s*\(', body):
            start = match.end() - 1
            depth = 0
            index = start
            while index < len(body):
                if body[index] == "(":
                    depth += 1
                elif body[index] == ")":
                    depth -= 1
                    if depth == 0:
                        break
                index += 1
            inner = body[start + 1:index]
            keys = re.findall(r'["\']([^"\']*)["\']\s*=>', inner)
            if keys:
                found.append((match.group(1), keys))
        return found

    def _fallbackOf(self, subName):
        """What ``subName`` does with a value it does not recognise."""
        info = self.subs.get(subName)
        if info is None:
            return Fallback.UNKNOWN
        body = info["body"]
        param = info["params"][0] if info["params"] else ""
        if param and re.search(r'\|\|\s*\$' + re.escape(param) + r'\b', body):
            return Fallback.PASSTHROUGH
        if param and re.search(r'else\s*\{[^{}]*\$\w+\s*=\s*\$'
                               + re.escape(param) + r'\b', body):
            return Fallback.PASSTHROUGH
        if re.search(r'else\s*\{[^{}]*\bdie\b', body):
            return Fallback.DIES
        # An opt-in switch, not a lookup table: the result is initialised to
        # "" and a keyword is emitted only for the values that need one, with
        # no else clause anywhere. ai.gauss16's StationaryPointType is the
        # pattern -- "Minimum" is Gaussian's default and correctly produces
        # nothing, so reporting it as a dropped setting is wrong. A real
        # lookup table always ends in an else.
        if (not re.search(r'\belse\b', body)
                and re.search(r'(?:local|my)\s*\(?\$\w+\)?\s*=\s*""\s*;',
                              body)):
            return Fallback.DEFAULTS_EMPTY
        if re.search(r'else\s*\{\s*\$\w+\s*\.?=\s*["\']["\']\s*;', body):
            return Fallback.SILENT
        if not re.search(r'\belse\b', body):
            return Fallback.SILENT
        return Fallback.UNKNOWN

    @property
    def name(self):
        import os
        return os.path.basename(self.path)


def _splitArgs(text):
    args = []
    depth = 0
    current = ""
    quote = None
    for char in text:
        if quote:
            current += char
            if char == quote:
                quote = None
            continue
        if char in "\"'":
            quote = char
        elif char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        elif char == "," and depth == 0:
            args.append(current)
            current = ""
            continue
        current += char
    if current.strip():
        args.append(current)
    return args
