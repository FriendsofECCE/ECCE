#include <cmath>
#include <map>
#include <array>
#include <algorithm>

#include "tdat/EspField.H"
#include "tdat/CoulombIntegrals.H"

namespace {

  //  Closer than this to a nucleus the point-charge term diverges while
  //  the electronic term stays finite.  Clamped rather than skipped: a
  //  single infinity would take the whole colour range with it, since
  //  the ramp is scaled to the extreme value.
  const double NUCLEUS_CLAMP = 1.0e-3;

  /** The Coulomb integral between two basis functions at a point. */
  double pairIntegral(const EspBasisFunction& a, const EspBasisFunction& b,
                      const double* point)
  {
    double total = 0.0;

    for (size_t ta = 0; ta < a.angularCoef.size(); ta++) {
      const int la[3] = { a.powerX[ta], a.powerY[ta], a.powerZ[ta] };

      for (size_t tb = 0; tb < b.angularCoef.size(); tb++) {
        const int lb[3] = { b.powerX[tb], b.powerY[tb], b.powerZ[tb] };
        const double angular = a.angularCoef[ta]*b.angularCoef[tb];
        if (angular == 0.0) continue;

        for (size_t ia = 0; ia < a.exponent.size(); ia++) {
          for (size_t ib = 0; ib < b.exponent.size(); ib++) {
            const double coef = a.contraction[ia]*b.contraction[ib];
            if (coef == 0.0) continue;

            total += angular*coef
                     * CoulombIntegrals::potential(a.center, la,
                                                   a.exponent[ia],
                                                   b.center, lb,
                                                   b.exponent[ib], point);
          }
        }
      }
    }
    return total;
  }
}


double EspBasisFunction::value(double x, double y, double z) const
{
  const double dx = x - center[0];
  const double dy = y - center[1];
  const double dz = z - center[2];
  const double rr = dx*dx + dy*dy + dz*dz;

  double radial = 0.0;
  for (size_t i = 0; i < exponent.size(); i++) {
    const double ar = exponent[i]*rr;
    if (ar < 70.0) radial += contraction[i]*exp(-ar);
  }
  if (radial == 0.0) return 0.0;

  double angular = 0.0;
  for (size_t t = 0; t < angularCoef.size(); t++) {
    angular += angularCoef[t]
               * pow(dx, powerX[t]) * pow(dy, powerY[t]) * pow(dz, powerZ[t]);
  }
  return radial*angular;
}


void EspField::selectPairs(const vector<EspBasisFunction>& basis,
                           const vector<double>& density,
                           double cutoff, Pairs& pairs)
{
  pairs.mu.clear();
  pairs.nu.clear();
  pairs.weight.clear();

  const size_t nbas = basis.size();
  if (density.size() != nbas*nbas) return;

  double largest = 0.0;
  for (size_t i = 0; i < density.size(); i++) {
    const double magnitude = fabs(density[i]);
    if (magnitude > largest) largest = magnitude;
  }
  const double threshold = largest*cutoff;

  for (size_t mu = 0; mu < nbas; mu++) {
    if (basis[mu].empty()) continue;
    for (size_t nu = mu; nu < nbas; nu++) {
      if (basis[nu].empty()) continue;

      const double value = density[mu*nbas + nu];
      if (fabs(value) <= threshold) continue;

      pairs.mu.push_back(mu);
      pairs.nu.push_back(nu);
      //  The density matrix is symmetric, so an off-diagonal pair
      //  stands for two equal terms and is counted once, doubled.
      pairs.weight.push_back((mu == nu) ? value : 2.0*value);
    }
  }
}


double EspField::overlapOf(const EspBasisFunction& a,
                           const EspBasisFunction& b)
{
  double s = 0.0;
  for (size_t ta = 0; ta < a.angularCoef.size(); ta++) {
    const int la[3] = { a.powerX[ta], a.powerY[ta], a.powerZ[ta] };
    for (size_t tb = 0; tb < b.angularCoef.size(); tb++) {
      const int lb[3] = { b.powerX[tb], b.powerY[tb], b.powerZ[tb] };
      const double angular = a.angularCoef[ta]*b.angularCoef[tb];
      if (angular == 0.0) continue;
      for (size_t ia = 0; ia < a.exponent.size(); ia++) {
        for (size_t ib = 0; ib < b.exponent.size(); ib++) {
          s += angular*a.contraction[ia]*b.contraction[ib]
               * CoulombIntegrals::overlap(a.center, la, a.exponent[ia],
                                           b.center, lb, b.exponent[ib]);
        }
      }
    }
  }
  return s;
}


double EspField::electronCount(const vector<EspBasisFunction>& basis,
                               const Pairs& pairs)
{
  double total = 0.0;

  for (size_t i = 0; i < pairs.size(); i++) {
    total += pairs.weight[i]*overlapOf(basis[pairs.mu[i]],
                                       basis[pairs.nu[i]]);
  }
  return total;
}


double EspField::density(const vector<EspBasisFunction>& basis,
                         const Pairs& pairs, double x, double y, double z)
{
  double total = 0.0;
  for (size_t i = 0; i < pairs.size(); i++) {
    total += pairs.weight[i]
             * basis[pairs.mu[i]].value(x, y, z)
             * basis[pairs.nu[i]].value(x, y, z);
  }
  return total;
}


double EspField::electronicPotential(const vector<EspBasisFunction>& basis,
                                     const Pairs& pairs, const double* point)
{
  double total = 0.0;
  for (size_t i = 0; i < pairs.size(); i++) {
    total += pairs.weight[i]
             * pairIntegral(basis[pairs.mu[i]], basis[pairs.nu[i]], point);
  }
  //  Electrons are negative.
  return -total;
}


double EspField::potential(const vector<EspBasisFunction>& basis,
                           const Pairs& pairs,
                           const vector<EspNucleus>& nuclei,
                           const double* point)
{
  double total = 0.0;

  for (size_t a = 0; a < nuclei.size(); a++) {
    double r2 = 0.0;
    for (int k = 0; k < 3; k++) {
      const double d = point[k] - nuclei[a].center[k];
      r2 += d*d;
    }
    double r = sqrt(r2);
    if (r < NUCLEUS_CLAMP) r = NUCLEUS_CLAMP;
    total += nuclei[a].charge/r;
  }

  return total + electronicPotential(basis, pairs, point);
}


//  ---- the prepared (per-grid, not per-point) evaluation ----

namespace {

  //  Primitive pairs are keyed on exponent sum and product centre.
  //  Bitwise equality is deliberate: function pairs of one shell pair
  //  compute these from identical inputs, so they match exactly, and a
  //  near miss merely costs a term, never accuracy.
  typedef std::array<double,4> TermKey;

  struct TermAccum {
    int L;
    vector<double> lam;     // dense (L+1)^3
  };

  inline size_t cube(int L) { return (size_t)(L+1)*(L+1)*(L+1); }

  void growTo(TermAccum& t, int L)
  {
    if (L <= t.L) return;
    vector<double> bigger(cube(L), 0.0);
    for (int a = 0; a <= t.L; a++)
      for (int b = 0; b <= t.L; b++)
        for (int c = 0; c <= t.L; c++)
          bigger[((size_t)a*(L+1) + b)*(L+1) + c] =
            t.lam[((size_t)a*(t.L+1) + b)*(t.L+1) + c];
    t.lam.swap(bigger);
    t.L = L;
  }

  //  A term whose whole Hermite density is below this contributes less
  //  than it to the potential; far-apart tight primitives are the usual
  //  case.  Far below single precision, which is what the grid stores.
  const double PRUNE = 1.0e-18;

  //  Per-thread scratch for the Hermite Coulomb table R^n_{tuv}.
  struct Scratch {
    vector<double> R;
    vector<double> F;
  };
}


void EspField::prepare(const vector<EspBasisFunction>& basis,
                       const Pairs& pairs, Prepared& out)
{
  out = Prepared();
  std::map<TermKey, TermAccum> terms;

  for (size_t k = 0; k < pairs.size(); k++) {
    const EspBasisFunction& a = basis[pairs.mu[k]];
    const EspBasisFunction& b = basis[pairs.nu[k]];
    const double w = pairs.weight[k];

    for (size_t ia = 0; ia < a.exponent.size(); ia++) {
      for (size_t ib = 0; ib < b.exponent.size(); ib++) {
        const double coef = a.contraction[ia]*b.contraction[ib];
        if (coef == 0.0) continue;

        const double alpha = a.exponent[ia], beta = b.exponent[ib];
        const double p = alpha + beta;
        TermKey key = {{ p, 0, 0, 0 }};
        for (int c = 0; c < 3; c++) {
          key[c+1] = (alpha*a.center[c] + beta*b.center[c])/p;
        }
        const double scale = w*coef*2.0*3.14159265358979323846/p;

        TermAccum& term = terms[key];
        if (term.lam.empty()) { term.L = 0; term.lam.assign(1, 0.0); }

        for (size_t ta = 0; ta < a.angularCoef.size(); ta++) {
          const int la[3] = { a.powerX[ta], a.powerY[ta], a.powerZ[ta] };
          for (size_t tb = 0; tb < b.angularCoef.size(); tb++) {
            const int lb[3] = { b.powerX[tb], b.powerY[tb], b.powerZ[tb] };
            const double ang = a.angularCoef[ta]*b.angularCoef[tb];
            if (ang == 0.0) continue;

            const int Lc[3] = { la[0]+lb[0], la[1]+lb[1], la[2]+lb[2] };
            growTo(term, Lc[0]+Lc[1]+Lc[2]);
            const int S = term.L + 1;

            double E[3][32];
            for (int c = 0; c < 3; c++) {
              for (int t = 0; t <= Lc[c]; t++) {
                E[c][t] = CoulombIntegrals::hermiteE(
                  la[c], lb[c], t, a.center[c]-b.center[c], alpha, beta);
              }
            }
            for (int t = 0; t <= Lc[0]; t++)
              for (int u = 0; u <= Lc[1]; u++)
                for (int v = 0; v <= Lc[2]; v++)
                  term.lam[((size_t)t*S + u)*S + v] +=
                    scale*ang*E[0][t]*E[1][u]*E[2][v];
          }
        }
      }
    }
  }

  for (std::map<TermKey, TermAccum>::const_iterator it = terms.begin();
       it != terms.end(); ++it) {
    double big = 0.0;
    for (size_t i = 0; i < it->second.lam.size(); i++) {
      big = std::max(big, fabs(it->second.lam[i]));
    }
    if (big < PRUNE) continue;

    out.p.push_back(it->first[0]);
    for (int c = 0; c < 3; c++) out.center.push_back(it->first[c+1]);
    out.order.push_back(it->second.L);
    out.offset.push_back(out.lambda.size());
    out.lambda.insert(out.lambda.end(), it->second.lam.begin(),
                      it->second.lam.end());
  }
}


double EspField::electronicPotential(const Prepared& prep,
                                     const double* point)
{
  static thread_local Scratch scratch;
  vector<double>& R = scratch.R;
  vector<double>& F = scratch.F;

  double total = 0.0;

  for (size_t k = 0; k < prep.size(); k++) {
    const int L = prep.order[k];
    const int S = L + 1;
    const double p = prep.p[k];
    const double PCx = prep.center[3*k]   - point[0];
    const double PCy = prep.center[3*k+1] - point[1];
    const double PCz = prep.center[3*k+2] - point[2];
    const double* lam = &prep.lambda[prep.offset[k]];

    if (F.size() < (size_t)S) F.resize(S);
    CoulombIntegrals::boys(L, p*(PCx*PCx + PCy*PCy + PCz*PCz), &F[0]);

    if (L == 0) {
      total += lam[0]*F[0];
      continue;
    }

    //  R[n][t][u][v], filled by total order N = t+u+v; entry (n,..) is
    //  needed for n <= L-N only.
    const size_t S3 = (size_t)S*S*S;
    if (R.size() < (size_t)S*S3) R.resize((size_t)S*S3);
    #define RI(n,t,u,v) R[(((size_t)(n)*S + (t))*S + (u))*S + (v)]

    double factor = 1.0;
    for (int n = 0; n <= L; n++) {
      RI(n,0,0,0) = factor*F[n];
      factor *= -2.0*p;
    }
    for (int N = 1; N <= L; N++) {
      for (int t = 0; t <= N; t++) {
        for (int u = 0; u <= N-t; u++) {
          const int v = N-t-u;
          for (int n = 0; n <= L-N; n++) {
            double val;
            if (t > 0) {
              val = PCx*RI(n+1,t-1,u,v);
              if (t > 1) val += (t-1)*RI(n+1,t-2,u,v);
            } else if (u > 0) {
              val = PCy*RI(n+1,t,u-1,v);
              if (u > 1) val += (u-1)*RI(n+1,t,u-2,v);
            } else {
              val = PCz*RI(n+1,t,u,v-1);
              if (v > 1) val += (v-1)*RI(n+1,t,u,v-2);
            }
            RI(n,t,u,v) = val;
          }
        }
      }
    }

    double sum = 0.0;
    for (int t = 0; t <= L; t++)
      for (int u = 0; u <= L-t; u++)
        for (int v = 0; v <= L-t-u; v++)
          sum += lam[((size_t)t*S + u)*S + v]*RI(0,t,u,v);
    total += sum;
    #undef RI
  }
  return -total;
}


double EspField::potential(const Prepared& prep,
                           const vector<EspNucleus>& nuclei,
                           const double* point)
{
  double total = 0.0;
  for (size_t a = 0; a < nuclei.size(); a++) {
    double r2 = 0.0;
    for (int k = 0; k < 3; k++) {
      const double d = point[k] - nuclei[a].center[k];
      r2 += d*d;
    }
    double r = sqrt(r2);
    if (r < NUCLEUS_CLAMP) r = NUCLEUS_CLAMP;
    total += nuclei[a].charge/r;
  }
  return total + electronicPotential(prep, point);
}
