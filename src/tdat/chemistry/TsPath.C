#include <cmath>
#include <cstdio>

#include "tdat/TsPath.H"

const double TsPath::minDistance = 0.5;
const double TsPath::linearTolerance = 0.1;

bool TsPath::isLinear(const std::vector<double>& x)
{
   const size_t n = x.size() / 3;
   if (n < 3) return true;
   size_t a = 0, b = 0;
   double best = -1;
   for (size_t i = 0; i < n; i++)
      for (size_t j = i + 1; j < n; j++) {
         double d = 0;
         for (int k = 0; k < 3; k++)
            d += (x[3*i+k]-x[3*j+k]) * (x[3*i+k]-x[3*j+k]);
         if (d > best) { best = d; a = i; b = j; }
      }
   if (best < 1e-12) return true;
   double u[3], len = sqrt(best);
   for (int k = 0; k < 3; k++) u[k] = (x[3*b+k]-x[3*a+k]) / len;
   for (size_t i = 0; i < n; i++) {
      double v[3], t = 0;
      for (int k = 0; k < 3; k++) { v[k] = x[3*i+k]-x[3*a+k]; t += v[k]*u[k]; }
      double p = 0;
      for (int k = 0; k < 3; k++) p += (v[k]-t*u[k]) * (v[k]-t*u[k]);
      if (sqrt(p) > linearTolerance) return false;
   }
   return true;
}

static void scanLeg(const std::vector<double>& p, const std::vector<double>& q,
                    TsPathReport& r)
{
   const size_t n = p.size() / 3;
   const int steps = 100;
   for (int s = 0; s <= steps; s++) {
      const double t = (double)s / steps;
      for (size_t i = 0; i < n; i++)
         for (size_t j = i + 1; j < n; j++) {
            double d = 0;
            for (int k = 0; k < 3; k++) {
               const double xi = p[3*i+k] + t*(q[3*i+k]-p[3*i+k]);
               const double xj = p[3*j+k] + t*(q[3*j+k]-p[3*j+k]);
               d += (xi-xj) * (xi-xj);
            }
            d = sqrt(d);
            if (d < r.closest) { r.closest = d; r.atomA = (int)i; r.atomB = (int)j; }
         }
   }
}

TsPathReport TsPath::check(const std::vector<double>& rea,
                           const std::vector<double>& prod,
                           const std::vector<double>* guess)
{
   TsPathReport r;
   if (rea.size() != prod.size() || (guess && guess->size() != rea.size()))
      return r;
   r.collinear = rea.size() / 3 >= 3 && isLinear(rea) && isLinear(prod)
              && (!guess || isLinear(*guess));
   // A picked guess defines the path; the straight line is then not what runs.
   if (!guess) scanLeg(rea, prod, r);
   r.passes = r.closest < minDistance;
   return r;
}

std::string TsPath::warning(const TsPathReport& r,
                            const std::vector<std::string>& el)
{
   // One line per distinct problem: a linear pair that also runs an atom
   // through another is one message, not two.
   std::string out;
   const char* tail = "pick a bent guess (TS Search \xE2\x86\x92 Guess).";
   const bool through = r.passes && r.atomA >= 0 && r.atomB >= 0
                        && r.atomB < (int)el.size();
   if (through) {
      char buf[160];
      snprintf(buf, sizeof buf, "Straight path passes %s%d through %s%d%s: ",
               el[r.atomA].c_str(), r.atomA + 1, el[r.atomB].c_str(),
               r.atomB + 1, r.collinear ? " (linear reactant and product)" : "");
      out = std::string(buf) + tail;
   } else if (r.collinear) {
      out = std::string("Linear reactant and product: the straight path cannot "
                        "bend; ") + tail;
   }
   return out;
}
