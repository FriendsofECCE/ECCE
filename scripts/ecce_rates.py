#!/usr/bin/env python3
"""Transition-state-theory rate formulas (#872): wx-free, no ECCE session.

Energies are kJ/mol, T in K, frequencies in cm-1 unless a name says otherwise.
The thermochemistry (G, H, S) comes from ecce-thermo; this module only turns
barriers into rate constants, tunnelling factors and Arrhenius/Eyring fits.
"""
import math

KB = 1.380649e-23           # J/K
H_PLANCK = 6.62607015e-34   # J s
NA = 6.02214076e23
C_CM = 29979245800.0        # cm/s
R_KJ = KB * NA / 1000.0     # kJ/mol/K
P0 = 101325.0               # Pa, the 1 atm standard state of ecce-thermo
KJ_PER_CM = H_PLANCK * C_CM * NA / 1000.0   # h c N_A per cm-1, kJ/mol


def c0(T, standard_state):
    """Standard concentration: mol/L for '1M', molecule/cm3 for '1atm' (p0/kT)."""
    if standard_state == "1M":
        return 1.0
    if standard_state == "1atm":
        return P0 / (KB * T) * 1e-6
    raise ValueError("standard state must be 1M or 1atm")


_SUP = str.maketrans("-0123456789", "\u207b\u2070\u00b9\u00b2\u00b3\u2074\u2075\u2076\u2077\u2078\u2079")


def sup(n):
    """n as superscript characters, e.g. -1 -> \u207b\u00b9."""
    return str(n).translate(_SUP)


def rate_unit(m, standard_state):
    """Unit of a rate constant of molecularity m, with real superscripts."""
    n = m - 1
    if n == 0:
        return "s" + sup(-1)
    if standard_state == "1M":
        return "M%s s%s" % (sup(-n), sup(-1))
    return "cm%s molecule%s s%s" % (sup(3 * n), sup(-n), sup(-1))


def tst_rate(T, dG_act, m, standard_state, sigma=1, kappa=1.0):
    """k = kappa sigma (kB T/h) c0^(1-m) exp(-dG_act/RT); dG_act in kJ/mol at the
    same standard state, so that for '1M' c0 = 1 and k is in M^(1-m) s^-1."""
    return (kappa * sigma * KB * T / H_PLANCK * c0(T, standard_state) ** (1 - m)
            * math.exp(-dG_act / (R_KJ * T)))


def wigner(T, nu):
    """kappa = 1 + (h nu / kB T)^2 / 24, nu the magnitude of the imaginary frequency."""
    u = KJ_PER_CM * abs(nu) / (R_KJ * T)
    return 1.0 + u * u / 24.0


def eckart_transmission(E, V1, V2, hnu):
    """Transmission probability of the unsymmetric Eckart barrier (Johnston and
    Heicklen, J. Phys. Chem. 66, 532 (1962)) at energy E above the reactant
    asymptote.  V1, V2: forward and reverse barrier heights; hnu: h|nu| of the
    imaginary frequency; all in one energy unit.  Zero below the higher
    asymptote max(0, V1 - V2)."""
    if V1 <= 0 or V2 <= 0 or hnu <= 0:
        raise ValueError("Eckart needs V1 > 0, V2 > 0 and hnu > 0")
    if E <= max(0.0, V1 - V2):
        return 0.0
    a1 = 2 * math.pi * V1 / hnu
    a2 = 2 * math.pi * V2 / hnu
    s = 1 / math.sqrt(a1) + 1 / math.sqrt(a2)
    xi = E / V1
    a = 2 * math.sqrt(a1 * xi) / s
    b = 2 * math.sqrt((xi - 1) * a1 + a2) / s
    p = a2 * a1 - math.pi ** 2 / 4
    # Divide numerator and denominator by exp(a+b) so large arguments cannot overflow.
    e = math.exp(-(a + b))
    num = 1 + e * e - math.exp(-2 * b) - math.exp(-2 * a)
    if p >= 0:
        d = 2 * math.sqrt(p)
        if d - (a + b) > 700:
            return 0.0
        den = 1 + e * e + math.exp(d - (a + b)) + math.exp(-d - (a + b))
    else:
        den = 1 + e * e + 2 * e * math.cos(2 * math.sqrt(-p))
    return num / den


def eckart_kappa(T, nu, V1, V2=None, npts=4000):
    """Eckart tunnelling factor: exp(V1/kT) * int T(E) exp(-E/kT) dE/kT from the
    higher asymptote.  V1, V2 in kJ/mol are the ZPE-corrected forward and reverse
    barriers; V2=None gives the symmetric barrier (V2 = V1)."""
    if V2 is None:
        V2 = V1
    kT = R_KJ * T
    hnu = KJ_PER_CM * abs(nu)
    e0 = max(0.0, V1 - V2)
    emax = max(V1, e0) + 40 * kT
    # E = e0 + s^2 removes the square-root start of T(E); Simpson in s.
    smax = math.sqrt(emax - e0)
    n = npts - npts % 2
    h = smax / n
    tot = 0.0
    for i in range(n + 1):
        s = i * h
        E = e0 + s * s
        f = 2 * s * eckart_transmission(E, V1, V2, hnu) * math.exp((V1 - E) / kT) / kT
        tot += f * (1 if i in (0, n) else 4 if i % 2 else 2)
    return tot * h / 3.0


def linfit(x, y):
    """Least-squares line y = a + b x -> (a, b)."""
    n = len(x)
    mx, my = sum(x) / n, sum(y) / n
    sxx = sum((xi - mx) ** 2 for xi in x)
    if n < 2 or sxx == 0:
        raise ValueError("need at least two distinct temperatures")
    b = sum((xi - mx) * (yi - my) for xi, yi in zip(x, y)) / sxx
    return my - b * mx, b


def arrhenius_fit(Ts, ks):
    """ln k = ln A - Ea/RT -> (A in the unit of k, Ea in kJ/mol)."""
    a, b = linfit([1.0 / t for t in Ts], [math.log(k) for k in ks])
    return math.exp(a), -b * R_KJ


def eyring_fit(Ts, ks, m=1, standard_state="1M", sigma=1):
    """ln(k c0^(m-1)/(sigma T)) = ln(kB/h) + dS/R - dH/(R T) -> (dH in kJ/mol,
    dS in J/mol/K).  Any tunnelling factor left in k lands in dH and dS."""
    y = [math.log(k * c0(t, standard_state) ** (m - 1) / (sigma * t))
         for t, k in zip(Ts, ks)]
    a, b = linfit([1.0 / t for t in Ts], y)
    return -b * R_KJ, (a - math.log(KB / H_PLANCK)) * R_KJ * 1000.0


def half_life(k, m, T, standard_state):
    """ln2/k for m = 1; for m = 2 the other reactant is held at the standard
    concentration (1 M or 1 atm); None for m > 2."""
    if k <= 0 or m > 2:
        return None
    return math.log(2) / (k if m == 1 else k * c0(T, standard_state))
