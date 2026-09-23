"""Convert a Basis Set Exchange JSON basis into ECCE's .BAS format.

Works from BSE's structured JSON rather than its NWChem text output.  The
2013 gbsNWChemConverter parses the text, and cannot: the retired EMSL BSE
emitted one block per element while the modern one emits a single block
for every element, which that parser has no branch for (see issue #116).
The JSON has no such ambiguity.
"""
import json, sys

AM = "SPDFGHIKLM"          # angular momentum 0,1,2,... -> shell letter
ELEM = {}                  # Z -> symbol, filled from the periodic table below

def shell_label(am):
    """[0]->S, [1]->P, [0,1]->SP, [2]->D ..."""
    return "".join(AM[a] for a in am)

def convert(data, symbols):
    out = []
    for z, el in sorted(data["elements"].items(), key=lambda kv: int(kv[0])):
        sym = symbols.get(int(z))
        if sym is None:
            continue
        out.append("atom=%s" % sym)
        for sh in el.get("electron_shells", []):
            exps = sh["exponents"]
            coefs = sh["coefficients"]           # list of coefficient columns
            label = shell_label(sh["angular_momentum"])
            #  A general contraction repeats the letter once per column, which
            #  is how the shipped library writes them (shell=DD, ncoef=2).
            if len(sh["angular_momentum"]) == 1 and len(coefs) > 1:
                label = label * len(coefs)
            out.append("contraction shell=%s num_primitives=%d num_coefficients=%d"
                       % (label, len(exps), len(coefs)))
            for i, e in enumerate(exps):
                row = [e] + [c[i] for c in coefs]
                out.append(" ".join(row))
    return "\n".join(out) + "\n"

SHELL = "spdfghi"


def convert_ecp(data, symbols):
    """Convert BSE ecp_potentials into ECCE's .POT format.

    The two line up directly.  BSE gives ecp_electrons and a list of
    potentials, the first being the local (highest angular momentum) term
    and the rest the l = 0, 1, ... projectors; ECCE writes ncore and lmax
    on the atom line and labels the same components "d", "s-d", "p-d".
    Both list the local term first, and each row is the same triple:
    r exponent, gaussian exponent, coefficient.

    Verified against the shipped library rather than assumed -- see the
    README.
    """
    out = []
    for z, el in sorted(data["elements"].items(), key=lambda kv: int(kv[0])):
        potentials = el.get("ecp_potentials")
        if not potentials:
            continue
        sym = symbols.get(int(z))
        if sym is None:
            continue
        lmax = max(p["angular_momentum"][0] for p in potentials)
        out.append("atom=%s ncore=%d lmax=%d"
                   % (sym, el.get("ecp_electrons", 0), lmax))
        #  Local term first, then the projectors in l order -- the order
        #  the shipped files use.
        ordered = ([p for p in potentials if p["angular_momentum"][0] == lmax]
                   + sorted((p for p in potentials
                             if p["angular_momentum"][0] != lmax),
                            key=lambda p: p["angular_momentum"][0]))
        for pot in ordered:
            l = pot["angular_momentum"][0]
            label = SHELL[l] if l == lmax else "%s-%s" % (SHELL[l], SHELL[lmax])
            rexp = pot["r_exponents"]
            gexp = pot["gaussian_exponents"]
            coef = pot["coefficients"][0]
            out.append("ecp_potential%%l=%d%%shell=%s potential%%num_exponents=%d"
                       % (l, label, len(gexp)))
            for i in range(len(gexp)):
                out.append("%s %s %s" % (rexp[i], gexp[i], coef[i]))
    return "\n".join(out) + "\n" if out else ""


if __name__ == "__main__":
    symbols = json.load(open(sys.argv[2]))
    symbols = {int(k): v for k, v in symbols.items()}
    data = json.load(open(sys.argv[1]))
    if len(sys.argv) > 3 and sys.argv[3] == "--ecp":
        print(convert_ecp(data, symbols), end="")
    else:
        print(convert(data, symbols), end="")
