#!/usr/bin/env python3
"""Unit tests for scripts/ecce_rates.py (#872).

The Eckart transmission probability is checked against an independent oracle:
the Schroedinger equation (hbar = mu = 1) for the Eckart potential, integrated
by RK4 from the exit side, with the barrier curvature fixing the length scale.
"""
import cmath
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts"))
import ecce_rates as R  # noqa: E402

fails = []


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else " " + str(info)))
    if not cond:
        fails.append(name)


def close(a, b, rel):
    return abs(a - b) <= rel * abs(b)


# --- Eckart potential and numerical scattering ---------------------------

def make_potential(V1, V2, omega):
    A = V1 - V2
    B = (math.sqrt(V1) + math.sqrt(V2)) ** 2

    def v(x, L):
        y = math.exp(max(min(2 * math.pi * x / L, 700), -700))
        return A * y / (1 + y) + B * y / (1 + y) ** 2

    # Maximum by ternary search at L = 1, then L from the curvature -V'' = omega^2.
    lo, hi = -3.0, 3.0
    for _ in range(200):
        m1, m2 = lo + (hi - lo) / 3, hi - (hi - lo) / 3
        if v(m1, 1.0) < v(m2, 1.0):
            lo = m1
        else:
            hi = m2
    xs = (lo + hi) / 2
    h = 1e-4
    d2 = (v(xs + h, 1.0) - 2 * v(xs, 1.0) + v(xs - h, 1.0)) / h ** 2
    L = math.sqrt(-d2 / omega ** 2)
    return (lambda x: v(x, L)), L, A, v(xs, 1.0)


def scatter_T(vf, L, A, E):
    """Transmission probability at E (> max(0, A)) by integrating from x = +X to -X."""
    k1, k2 = math.sqrt(2 * E), math.sqrt(2 * (E - A))
    X = 12 * L
    n = int(2 * X / min(L / 150, 0.02 / max(k1, k2, 1e-3)))
    dx = -2 * X / n
    x, psi, dpsi = X, cmath.exp(1j * k2 * X), 1j * k2 * cmath.exp(1j * k2 * X)

    def f(xx, p, dp):
        return dp, 2 * (vf(xx) - E) * p

    for _ in range(n):
        a1, b1 = f(x, psi, dpsi)
        a2, b2 = f(x + dx / 2, psi + dx / 2 * a1, dpsi + dx / 2 * b1)
        a3, b3 = f(x + dx / 2, psi + dx / 2 * a2, dpsi + dx / 2 * b2)
        a4, b4 = f(x + dx, psi + dx * a3, dpsi + dx * b3)
        psi += dx / 6 * (a1 + 2 * a2 + 2 * a3 + a4)
        dpsi += dx / 6 * (b1 + 2 * b2 + 2 * b3 + b4)
        x += dx
    inc = (psi + dpsi / (1j * k1)) / 2 * cmath.exp(-1j * k1 * x)
    return (k2 / k1) / abs(inc) ** 2


def numeric_kappa(vf, L, A, V1, kT, n=100):
    e0 = max(0.0, A)
    smax = math.sqrt(max(V1, e0) + 30 * kT - e0)
    h = smax / n
    tot = 0.0
    for i in range(n + 1):
        s = i * h
        E = e0 + s * s + 1e-9
        t = scatter_T(vf, L, A, E) if i else 0.0
        tot += (1 if i in (0, n) else 4 if i % 2 else 2) * 2 * s * t * math.exp((V1 - E) / kT) / kT
    return tot * h / 3


def test_eckart():
    for V1, V2, omega in [(8.0, 8.0, 2.0), (10.0, 4.0, 1.5), (6.0, 14.0, 2.5)]:
        vf, L, A, vmax = make_potential(V1, V2, omega)
        check("eckart potential max = V1 (%g,%g)" % (V1, V2), close(vmax, V1, 1e-6), vmax)
        worst = 0.0
        for E in [max(0, A) + d for d in (0.3, 1.0, 3.0)] + [V1 - 1.0, V1, V1 + 1.0, V1 + 5.0]:
            if E <= max(0, A):
                continue
            tn = scatter_T(vf, L, A, E)
            ta = R.eckart_transmission(E, V1, V2, omega)
            worst = max(worst, abs(tn - ta))
        check("eckart T(E) vs Schroedinger (%g,%g,w=%g): max diff %.1e" % (V1, V2, omega, worst),
              worst < 2e-4, worst)
    # Boltzmann average for one unsymmetric case (energy unit: hbar omega = 1.5).
    V1, V2, omega, kT = 10.0, 4.0, 1.5, 1.2
    vf, L, A, _ = make_potential(V1, V2, omega)
    kn = numeric_kappa(vf, L, A, V1, kT)
    # The module works in kJ/mol and cm-1: rescale the unit so omega -> hnu.
    unit = R.KJ_PER_CM * 100.0 / omega     # one test unit in kJ/mol with nu = 100 cm-1
    T = kT * unit / R.R_KJ
    km = R.eckart_kappa(T, 100.0, V1 * unit, V2 * unit)
    check("eckart kappa vs Schroedinger Boltzmann integral: %.4f vs %.4f" % (km, kn),
          close(km, kn, 5e-3), (km, kn))


def test_limits():
    # Wigner: closed form, and Eckart tends to it for a high barrier and small u.
    check("wigner closed form", close(R.wigner(300.0, 600.0),
          1 + (R.KJ_PER_CM * 600 / (R.R_KJ * 300)) ** 2 / 24, 1e-12))
    kw, ke = R.wigner(400.0, 150.0), R.eckart_kappa(400.0, 150.0, 400.0)
    check("eckart -> wigner at small u, high barrier (%.5f, %.5f)" % (ke, kw),
          close(ke, kw, 1e-3), (ke, kw))
    check("eckart kappa >= 1 and rises with V2 asymmetry cut-off",
          R.eckart_kappa(298.15, 1200.0, 50.0, 50.0) > 1.0)
    check("T(E) -> 1 far above the barrier", R.eckart_transmission(400.0, 40.0, 40.0, 3.0) > 0.9999)
    check("T(E) = 0 below the product asymptote", R.eckart_transmission(5.0, 40.0, 10.0, 3.0) == 0.0)


def test_tst():
    T, dG = 298.15, 80.0
    k1 = R.tst_rate(T, dG, 1, "1M")
    ref = 1.380649e-23 * T / 6.62607015e-34 * math.exp(-dG * 1000 / (8.314462618 * T))
    check("TST k (unimolecular) = kB T/h exp(-dG/RT): %.4e" % k1, close(k1, ref, 1e-9))
    check("sigma and kappa multiply", close(R.tst_rate(T, dG, 1, "1M", sigma=2, kappa=1.5), 3 * k1, 1e-12))
    check("1M bimolecular has the same number in M^-1 s^-1", close(R.tst_rate(T, dG, 2, "1M"), k1, 1e-12))
    c = 101325.0 / (1.380649e-23 * T) * 1e-6
    check("1 atm bimolecular: c0 = p0/kT in molecule cm-3 = %.4e" % c,
          close(R.tst_rate(T, dG, 2, "1atm"), k1 / c, 1e-9))
    check("units", (R.rate_unit(1, "1M"), R.rate_unit(2, "1M"), R.rate_unit(2, "1atm"),
                    R.rate_unit(3, "1atm")) == ("s^-1", "M^-1 s^-1", "cm^3 molecule^-1 s^-1",
                                                "cm^6 molecule^-2 s^-1"))
    check("half life m=1", close(R.half_life(0.01, 1, T, "1M"), math.log(2) / 0.01, 1e-12))
    check("half life m=2 at 1 M", close(R.half_life(0.01, 2, T, "1M"), math.log(2) / 0.01, 1e-12))
    check("half life m=3 undefined", R.half_life(0.01, 3, T, "1M") is None)


def test_fits():
    Ts = [250 + 25 * i for i in range(7)]
    A, Ea = 3.2e11, 55.0
    ks = [A * math.exp(-Ea * 1000 / (8.314462618 * t)) for t in Ts]
    a, e = R.arrhenius_fit(Ts, ks)
    check("Arrhenius fit recovers A, Ea: %.4g, %.4f" % (a, e), close(a, A, 1e-6) and close(e, Ea, 1e-9))
    for m, ss in [(1, "1M"), (2, "1M"), (2, "1atm")]:
        dH, dS, sig = 62.0, -35.0, 2
        ks = [sig * R.KB * t / R.H_PLANCK * R.c0(t, ss) ** (1 - m)
              * math.exp(dS / 8.314462618) * math.exp(-dH * 1000 / (8.314462618 * t)) for t in Ts]
        h, s = R.eyring_fit(Ts, ks, m, ss, sig)
        check("Eyring fit recovers dH, dS (m=%d, %s): %.5f, %.4f" % (m, ss, h, s),
              close(h, dH, 1e-9) and abs(s - dS) < 1e-6, (h, s))


test_tst()
test_fits()
test_limits()
test_eckart()
print("FAILED: %s" % ", ".join(fails) if fails else "all passed")
sys.exit(1 if fails else 0)
