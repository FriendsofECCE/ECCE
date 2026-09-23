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

if __name__ == "__main__":
    symbols = json.load(open(sys.argv[2]))
    symbols = {int(k): v for k, v in symbols.items()}
    print(convert(json.load(open(sys.argv[1])), symbols), end="")
