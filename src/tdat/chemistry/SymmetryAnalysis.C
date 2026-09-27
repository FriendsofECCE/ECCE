#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <algorithm>

#include "tdat/SymmetryAnalysis.H"
#include "tdat/CharacterTable.H"
#include "tdat/ShellRotation.H"
#include "tdat/TGBSAngFunc.H"

#include <map>
#include <utility>
using std::map;

namespace {

  /**
   * Cyclic Jacobi eigendecomposition of a 3x3 SYMMETRIC matrix.
   *
   * Plain and small on purpose: this is only ever asked to diagonalise
   * a 3x3 (H^T H for alignFrames() below), so a full SVD library would
   * be a lot of new dependency for a shape that has a closed, simple
   * classical method.  A fixed sweep count is enough for 3x3 -- off-
   * diagonal elements are driven towards zero geometrically and a
   * handful of sweeps is the standard textbook figure for convergence
   * to machine precision at this size.
   *
   * @param a        the symmetric matrix, overwritten with garbage
   * @param eigval   filled with the 3 eigenvalues
   * @param eigvec   filled with the 3 eigenvectors, AS COLUMNS
   */
  void jacobiEigen3(double a[3][3], double eigval[3], double eigvec[3][3])
  {
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) eigvec[i][j] = (i == j) ? 1.0 : 0.0;

    for (int sweep = 0; sweep < 60; sweep++) {
      double off = fabs(a[0][1]) + fabs(a[0][2]) + fabs(a[1][2]);
      if (off < 1.0e-14) break;

      for (int p = 0; p < 3; p++) {
        for (int q = p+1; q < 3; q++) {
          if (fabs(a[p][q]) < 1.0e-300) continue;

          const double theta = (a[q][q] - a[p][p]) / (2.0*a[p][q]);
          const double sign = (theta >= 0.0) ? 1.0 : -1.0;
          const double t = sign / (fabs(theta) + sqrt(theta*theta + 1.0));
          const double c = 1.0 / sqrt(t*t + 1.0);
          const double s = t*c;

          const double app = a[p][p], aqq = a[q][q], apq = a[p][q];
          a[p][p] = app - t*apq;
          a[q][q] = aqq + t*apq;
          a[p][q] = a[q][p] = 0.0;

          for (int r = 0; r < 3; r++) {
            if (r == p || r == q) continue;
            const double arp = a[r][p], arq = a[r][q];
            a[r][p] = a[p][r] = c*arp - s*arq;
            a[r][q] = a[q][r] = s*arp + c*arq;
          }
          for (int r = 0; r < 3; r++) {
            const double vrp = eigvec[r][p], vrq = eigvec[r][q];
            eigvec[r][p] = c*vrp - s*vrq;
            eigvec[r][q] = s*vrp + c*vrq;
          }
        }
      }
    }

    for (int i = 0; i < 3; i++) eigval[i] = a[i][i];
  }

  /**
   * Reduce a character vector (one entry per TABLE class, already
   * counts-weighted the way CharacterTable::reduce() expects) and
   * accept it only as EXACTLY ONE irrep of a given dimension --
   * shared by SymmetryAnalysis::orbitalIrrep() and ::fullOrbitalIrrep(),
   * which differ in how chi is computed (s/p-only, orthonormal, vs. any
   * shell, S-weighted) but agree completely on what "cleanly one irrep"
   * means afterwards. See orbitalIrrep()'s header comment for why the
   * tolerance (0.1) exists at all: an SCF orbital is symmetry-adapted
   * only to the convergence it was run to, and CharacterTable::reduce()'s
   * strict integrality would reject nearly all of them.
   */
  bool reduceToOneIrrep(const vector<double>& chi, size_t numClasses,
                        const CharacterTable& table, int expectedDimension,
                        string& irrep)
  {
     const vector<string>& names = table.irreps();
     const vector<int>& counts = table.counts();
     if (counts.size() != numClasses) return false;

     int found = -1;
     double total2 = 0.0;
     for (size_t i = 0; i < names.size(); i++) {
        const vector<double> *chiI = table.characters(names[i]);
        if (chiI == 0 || chiI->size() != numClasses) return false;

        double sum = 0.0;
        for (size_t k = 0; k < numClasses; k++) {
           sum += counts[k]*(*chiI)[k]*chi[k];
        }
        const double n = sum/(double)table.order();
        const double rounded = (n < 0.0) ? -floor(-n + 0.5) : floor(n + 0.5);

        if (fabs(n - rounded) > 0.1) return false;
        if (rounded < -0.5) return false;
        if (rounded > 0.5) {
           if (found >= 0) return false;          // more than one irrep
           if (rounded > 1.5) return false;       // more than once
           found = (int)i;
        }
        total2 += rounded;
     }

     if (found < 0 || total2 < 0.5 || total2 > 1.5) return false;
     if (table.dimension(names[found]) != expectedDimension) return false;

     irrep = names[found];
     return true;
  }

  SymOp identityOp()
  {
    SymOp id;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) id.m[i][j] = (i == j) ? 1.0 : 0.0;
    return id;
  }

  /** Apply an operation to a point. */
  void apply(const SymOp& op, const double* in, double* out)
  {
    for (int i = 0; i < 3; i++) {
      out[i] = op.m[i][0]*in[0] + op.m[i][1]*in[1] + op.m[i][2]*in[2];
    }
  }

  SymOp multiply(const SymOp& a, const SymOp& b)
  {
    SymOp out;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        out.m[i][j] = 0.0;
        for (int k = 0; k < 3; k++) out.m[i][j] += a.m[i][k]*b.m[k][j];
      }
    }
    return out;
  }

  SymOp transpose(const SymOp& a)
  {
    SymOp out;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) out.m[i][j] = a.m[j][i];
    return out;
  }

  bool same(const SymOp& a, const SymOp& b, double tol = 1.0e-6)
  {
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (fabs(a.m[i][j] - b.m[i][j]) > tol) return false;
    return true;
  }

  /**
   * Which Cartesian axis an operation acts on, or -1 for none.
   *
   * For a mirror, the axis it reflects; for a two-fold rotation, the
   * axis it turns about.  This is what separates C2v's two mirrors and
   * D4h's two sets of C2 axes, which (determinant, trace, size) cannot.
   * Returns -1 for anything not aligned with x, y or z -- a diagonal
   * C2, say -- which is itself the distinction in D4h.
   */
  int actingAxis(const SymOp& op)
  {
    //  An operation aligned with the axes is diagonal.
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (i != j && fabs(op.m[i][j]) > 1.0e-6) return -1;
      }
    }

    const double d = op.determinant();
    if (d < 0.0) {
      //  A mirror: two axes preserved, one reversed.  The reversed one
      //  is the plane's normal; name the operation by it.
      for (int i = 0; i < 3; i++) if (op.m[i][i] < 0.0) return i;
    } else {
      //  A two-fold rotation: one axis preserved, two reversed.
      for (int i = 0; i < 3; i++) if (op.m[i][i] > 0.0) return i;
    }
    return -1;
  }

  /** The axis named in a class label like "sv_xz", "C2_z", or none. */
  int labelledAxis(const string& label)
  {
    const size_t underscore = label.find('_');
    if (underscore == string::npos) return -1;

    const string tag = label.substr(underscore+1);

    //  A mirror is named for its PLANE ("sv_xz"), a rotation for its
    //  AXIS ("C2_z").  Both end up as the one direction that names the
    //  operation: the plane's normal, or the rotation axis.
    if (tag.size() == 2) {
      bool has[3] = {false, false, false};
      for (size_t i = 0; i < tag.size(); i++) {
        if (tag[i] == 'x') has[0] = true;
        else if (tag[i] == 'y') has[1] = true;
        else if (tag[i] == 'z') has[2] = true;
      }
      for (int i = 0; i < 3; i++) if (!has[i]) return i;   // the normal
      return -1;
    }
    if (tag == "x") return 0;
    if (tag == "y") return 1;
    if (tag == "z") return 2;
    return -1;
  }
}


double SymOp::determinant(void) const
{
  return m[0][0]*(m[1][1]*m[2][2] - m[1][2]*m[2][1])
       - m[0][1]*(m[1][0]*m[2][2] - m[1][2]*m[2][0])
       + m[0][2]*(m[1][0]*m[2][1] - m[1][1]*m[2][0]);
}


bool SymmetryAnalysis::atomImages(const vector<double>& coords,
                                  const vector<string>& elements,
                                  const vector<SymOp>& ops,
                                  double tolerance,
                                  vector< vector<int> >& images)
{
  images.clear();

  const size_t numAtoms = elements.size();
  if (numAtoms == 0 || coords.size() != numAtoms*3) return false;

  for (size_t o = 0; o < ops.size(); o++) {
    vector<int> image(numAtoms, -1);

    for (size_t a = 0; a < numAtoms; a++) {
      double moved[3];
      apply(ops[o], &coords[a*3], moved);

      for (size_t b = 0; b < numAtoms; b++) {
        if (elements[b] != elements[a]) continue;

        const double dx = moved[0] - coords[b*3];
        const double dy = moved[1] - coords[b*3+1];
        const double dz = moved[2] - coords[b*3+2];
        if (sqrt(dx*dx + dy*dy + dz*dz) <= tolerance) {
          image[a] = (int)b;
          break;
        }
      }
      //  An atom with nowhere to go means the operations and the
      //  coordinates are not in the same frame.  That is the failure
      //  this analysis is most exposed to, so it is reported rather
      //  than worked around.
      if (image[a] < 0) return false;
    }
    images.push_back(image);
  }
  return true;
}


void SymmetryAnalysis::orbits(const vector< vector<int> >& images,
                              int numAtoms,
                              vector< vector<int> >& out)
{
  out.clear();
  vector<bool> placed(numAtoms, false);

  for (int a = 0; a < numAtoms; a++) {
    if (placed[a]) continue;

    vector<int> orbit;
    for (size_t o = 0; o < images.size(); o++) {
      const int b = images[o][a];
      if (b >= 0 && !placed[b]) {
        placed[b] = true;
        orbit.push_back(b);
      }
    }
    if (!placed[a]) { placed[a] = true; orbit.push_back(a); }
    if (!orbit.empty()) out.push_back(orbit);
  }
}


void SymmetryAnalysis::conjugacyClasses(const vector<SymOp>& ops,
                                        vector< vector<int> >& classes)
{
  classes.clear();
  const size_t n = ops.size();
  vector<bool> assigned(n, false);

  for (size_t r = 0; r < n; r++) {
    if (assigned[r]) continue;

    vector<int> members;
    for (size_t s = 0; s < n; s++) {
      //  Every point group operation is orthogonal, so the inverse is
      //  the transpose.
      const SymOp conj = multiply(multiply(ops[s], ops[r]), transpose(ops[s]));

      for (size_t t = 0; t < n; t++) {
        if (!assigned[t] && same(conj, ops[t])) {
          assigned[t] = true;
          members.push_back((int)t);
          break;
        }
      }
    }
    if (!members.empty()) classes.push_back(members);
  }
}


bool SymmetryAnalysis::matchClasses(const vector<SymOp>& ops,
                                    const vector< vector<int> >& classes,
                                    const CharacterTable& table,
                                    vector<int>& classOfOp)
{
  classOfOp.assign(ops.size(), -1);

  const vector<string>& names = table.classes();
  const vector<int>& counts = table.counts();
  if (names.size() != classes.size()) return false;

  vector<bool> used(names.size(), false);

  //  Two passes.  The first takes only classes that (determinant,
  //  trace, size) identifies uniquely; the second settles the rest by
  //  the axis the operations act on.  Done in that order because the
  //  axis convention is the part that can be wrong, and leaving it to
  //  the cases that need it keeps it out of the ones that do not.
  for (int pass = 0; pass < 2; pass++) {
    for (size_t c = 0; c < classes.size(); c++) {
      if (classes[c].empty()) continue;
      if (classOfOp[classes[c][0]] >= 0) continue;

      const SymOp& rep = ops[classes[c][0]];
      const double det = rep.determinant();
      const double tr = rep.trace();
      const int size = (int)classes[c].size();

      vector<int> candidates;
      for (size_t t = 0; t < names.size(); t++) {
        if (used[t] || counts[t] != size) continue;

        //  The class name gives the operation type, and the type fixes
        //  the determinant and trace: 1+2cos(theta) for a rotation,
        //  -1+2cos(theta) for an improper one, +1 for a mirror.
        const string& label = names[t];
        const string stem = label.substr(0, label.find('_'));

        bool ok = false;
        if (stem == "E" || stem == "e") {
          ok = (det > 0 && fabs(tr - 3.0) < 1e-6);
        } else if (stem == "i" || stem == "I") {
          ok = (det < 0 && fabs(tr + 3.0) < 1e-6);
        } else if (!stem.empty() && stem[0] == 's') {
          ok = (det < 0 && fabs(tr - 1.0) < 1e-6);
        } else if (!stem.empty() && (stem[0] == 'C' || stem[0] == 'S')) {
          //  "C5p2" is C5 SQUARED.  Joining every digit in the name
          //  made that a 52-fold rotation; the same shape broke the
          //  five-fold groups in the tests until it was fixed there
          //  too, so keep the two readings the same.
          int order = 2, power = 1;
          const size_t pIdx = stem.find('p');
          const string head = (pIdx == string::npos) ? stem.substr(1)
                                                     : stem.substr(1, pIdx-1);
          const string tail = (pIdx == string::npos) ? string()
                                                     : stem.substr(pIdx+1);
          if (!head.empty()) order = atoi(head.c_str());
          if (!tail.empty()) power = atoi(tail.c_str());
          if (order < 1) order = 2;
          const double angle = 2.0*M_PI*power/order;
          const double want = (stem[0] == 'C') ? 1.0 + 2.0*cos(angle)
                                               : -1.0 + 2.0*cos(angle);
          const double wantDet = (stem[0] == 'C') ? 1.0 : -1.0;
          ok = (det*wantDet > 0 && fabs(tr - want) < 1e-6);
        }
        if (ok) candidates.push_back((int)t);
      }

      if (candidates.empty()) return false;

      int chosen = -1;
      if (candidates.size() == 1) {
        //  Taken in either pass.  It must be taken in pass 1 too: a
        //  class that was ambiguous at the start can be left with one
        //  candidate once the other has been claimed, and skipping it
        //  there left it unassigned forever -- which is what C2v's two
        //  mirrors did, the only case in the suite that reaches pass 1
        //  at all.
        chosen = candidates[0];
      } else {
        if (pass == 0) continue;            // ambiguous ones wait
        //  Settle by the axis the operations act on.
        const int axis = actingAxis(rep);
        for (size_t k = 0; k < candidates.size() && chosen < 0; k++) {
          if (labelledAxis(names[candidates[k]]) == axis) {
            chosen = candidates[k];
          }
        }
        //  No axis in the label to match against: fall back to the
        //  convention that an axis-aligned class comes before a
        //  diagonal one, which is what the primes mean.
        if (chosen < 0) {
          for (size_t k = 0; k < candidates.size() && chosen < 0; k++) {
            const string& label = names[candidates[k]];
            const bool primed = label.find('\'') != string::npos;
            if ((axis >= 0) == !primed) chosen = candidates[k];
          }
        }
        if (chosen < 0) chosen = candidates[0];
      }

      used[chosen] = true;
      for (size_t k = 0; k < classes[c].size(); k++) {
        classOfOp[classes[c][k]] = chosen;
      }
    }
  }

  for (size_t o = 0; o < classOfOp.size(); o++) {
    if (classOfOp[o] < 0) return false;
  }
  return true;
}


bool SymmetryAnalysis::orbitalCharacter(const vector<int>& orbit,
                                        const vector< vector<int> >& images,
                                        const vector<int>& classOfOp,
                                        int numClasses,
                                        vector<double>& chi)
{
  chi.assign(numClasses, 0.0);
  vector<int> seen(numClasses, 0);

  vector<bool> inOrbit;
  for (size_t i = 0; i < orbit.size(); i++) {
    if ((int)inOrbit.size() <= orbit[i]) inOrbit.resize(orbit[i]+1, false);
    inOrbit[orbit[i]] = true;
  }

  for (size_t o = 0; o < images.size(); o++) {
    const int c = classOfOp[o];
    if (c < 0 || c >= numClasses) return false;

    //  An s orbital carries no sign, so the character is simply how
    //  many of the orbit's atoms the operation leaves in place.
    int fixed = 0;
    for (size_t i = 0; i < orbit.size(); i++) {
      if (images[o][orbit[i]] == orbit[i]) fixed++;
    }

    //  Every operation in a class must agree; taking the first and
    //  checking the rest is a cheap guard on the class matching.
    if (seen[c] == 0) {
      chi[c] = fixed;
    } else if (fabs(chi[c] - fixed) > 1.0e-9) {
      return false;
    }
    seen[c]++;
  }

  for (int c = 0; c < numClasses; c++) if (seen[c] == 0) return false;
  return true;
}


bool SymmetryAnalysis::projectVectorOrbit(const vector<int>& orbit,
                                          const vector< vector<int> >& images,
                                          const vector<int>& classOfOp,
                                          const vector<SymOp>& ops,
                                          const CharacterTable& table,
                                          const string& irrep,
                                          vector< vector<double> >& vectors)
{
  vectors.clear();

  const vector<double>* chi = table.characters(irrep);
  if (chi == 0) return false;
  if (classOfOp.size() != images.size()) return false;
  if (ops.size() != images.size()) return false;

  const size_t n = orbit.size();
  if (n == 0) return false;
  const size_t width = 3*n;

  map<int, size_t> position;
  for (size_t i = 0; i < n; i++) position[orbit[i]] = i;

  const double dimension = table.dimension(irrep);
  const double h = table.order();
  if (h <= 0.0) return false;

  //  Project every basis function of the set in turn: three per atom,
  //  not one.  Most results repeat or vanish; Gram-Schmidt below keeps
  //  the independent ones.
  for (size_t a = 0; a < n; a++) {
    for (int k = 0; k < 3; k++) {

      vector<double> v(width, 0.0);
      for (size_t o = 0; o < ops.size(); o++) {
        const int cls = classOfOp[o];
        if (cls < 0 || (size_t)cls >= chi->size()) return false;

        map<int, size_t>::const_iterator it =
            position.find(images[o][orbit[a]]);
        if (it == position.end()) continue;

        //  AN OPERATION MIXES THE COMPONENTS.  p_k on this atom goes to
        //  sum_k' R[k'][k] p_k' on the image atom -- the same matrix
        //  that moves the atom, because p orbitals transform like the
        //  coordinates they are named for.  An s projection can leave
        //  this out; a p projection is nothing but this.
        for (int kp = 0; kp < 3; kp++) {
          v[3*it->second + kp] += (*chi)[cls]*ops[o].m[kp][k];
        }
      }
      for (size_t i = 0; i < width; i++) v[i] *= dimension/h;

      for (size_t j = 0; j < vectors.size(); j++) {
        double dot = 0.0;
        for (size_t i = 0; i < width; i++) dot += v[i]*vectors[j][i];
        for (size_t i = 0; i < width; i++) v[i] -= dot*vectors[j][i];
      }

      double norm = 0.0;
      for (size_t i = 0; i < width; i++) norm += v[i]*v[i];
      if (norm < 1.0e-10) continue;

      norm = sqrt(norm);
      for (size_t i = 0; i < width; i++) v[i] /= norm;
      vectors.push_back(v);
    }
  }
  return true;
}


bool SymmetryAnalysis::projectOrbit(const vector<int>& orbit,
                                    const vector< vector<int> >& images,
                                    const vector<int>& classOfOp,
                                    const CharacterTable& table,
                                    const string& irrep,
                                    vector< vector<double> >& vectors)
{
  vectors.clear();

  const vector<double>* chi = table.characters(irrep);
  if (chi == 0) return false;
  if (classOfOp.size() != images.size()) return false;

  const size_t n = orbit.size();
  if (n == 0) return false;

  //  Where each atom sits in the orbit, so an image can be turned back
  //  into a position in the coefficient vector.
  map<int, size_t> position;
  for (size_t i = 0; i < n; i++) position[orbit[i]] = i;

  const double dimension = table.dimension(irrep);
  const double h = table.order();
  if (h <= 0.0) return false;

  //  Project each orbital of the orbit in turn.  Most of the results
  //  are repeats of one another or zero; Gram-Schmidt below keeps the
  //  independent ones.
  for (size_t a = 0; a < n; a++) {

    vector<double> v(n, 0.0);
    for (size_t o = 0; o < images.size(); o++) {
      const int cls = classOfOp[o];
      if (cls < 0 || (size_t)cls >= chi->size()) return false;

      map<int, size_t>::const_iterator it = position.find(images[o][orbit[a]]);
      if (it == position.end()) continue;      // left the orbit: cannot happen
      v[it->second] += (*chi)[cls];
    }
    for (size_t i = 0; i < n; i++) v[i] *= dimension/h;

    //  Remove whatever is already spanned, then keep what is left if
    //  there is anything of it.
    for (size_t k = 0; k < vectors.size(); k++) {
      double dot = 0.0;
      for (size_t i = 0; i < n; i++) dot += v[i]*vectors[k][i];
      for (size_t i = 0; i < n; i++) v[i] -= dot*vectors[k][i];
    }

    double norm = 0.0;
    for (size_t i = 0; i < n; i++) norm += v[i]*v[i];
    if (norm < 1.0e-10) continue;              // nothing new

    norm = sqrt(norm);
    for (size_t i = 0; i < n; i++) v[i] /= norm;
    vectors.push_back(v);
  }

  return true;
}


namespace {

  /** Rotation angle and parity of an operation, from its matrix. */
  void angleAndParity(const SymOp& op, double& theta, bool& proper)
  {
    proper = (op.determinant() > 0.0);
    double c = proper ? (op.trace() - 1.0)/2.0 : (op.trace() + 1.0)/2.0;
    if (c >  1.0) c =  1.0;
    if (c < -1.0) c = -1.0;
    theta = acos(c);
  }

  /** Character of the angular momentum l representation. */
  double angularCharacter(int l, double theta, bool proper)
  {
    const double t = proper ? theta : theta - M_PI;
    double sum = 1.0;
    for (int m = 1; m <= l; m++) sum += 2.0*cos(m*t);
    if (!proper && (l % 2)) sum = -sum;
    return sum;
  }
}


bool SymmetryAnalysis::basisCharacter(const vector< vector<int> >& shells,
                                      const vector< vector<int> >& images,
                                      const vector<int>& classOfOp,
                                      const vector<SymOp>& ops,
                                      int numClasses,
                                      bool cartesian,
                                      vector<double>& chi)
{
  chi.assign(numClasses, 0.0);
  vector<int> seen(numClasses, 0);

  if (classOfOp.size() != images.size() || ops.size() != images.size())
    return false;

  for (size_t o = 0; o < ops.size(); o++) {
    const int cls = classOfOp[o];
    if (cls < 0 || cls >= numClasses) return false;

    double theta;
    bool proper;
    angleAndParity(ops[o], theta, proper);

    double total = 0.0;
    for (size_t a = 0; a < shells.size() && a < images[o].size(); a++) {
      //  Only an atom left in place contributes: an orbital moved onto
      //  a different atom has no diagonal element.
      if (images[o][a] != (int)a) continue;

      for (size_t s = 0; s < shells[a].size(); s++) {
        const int l = shells[a][s];
        if (cartesian) {
          //  A Cartesian shell of degree l spans D_l + D_{l-2} + ...
          for (int k = l; k >= 0; k -= 2) {
            total += angularCharacter(k, theta, proper);
          }
        } else {
          total += angularCharacter(l, theta, proper);
        }
      }
    }

    //  Every operation in a class must agree.
    if (seen[cls] == 0) {
      chi[cls] = total;
    } else if (fabs(chi[cls] - total) > 1.0e-9) {
      return false;
    }
    seen[cls]++;
  }

  for (int c = 0; c < numClasses; c++) if (seen[c] == 0) return false;
  return true;
}


bool SymmetryAnalysis::orbitalIrrep(const vector< vector<double> >& orbitals,
                                    const vector<int>& perAtom,
                                    const vector<int>& shellOf,
                                    const vector< vector<int> >& images,
                                    const vector<int>& classOfOp,
                                    const vector<SymOp>& ops,
                                    const CharacterTable& table,
                                    string& irrep,
                                    const int* componentOrder)
{
   //  Which basis function is x, which is y, which is z, within a p
   //  shell.  The identity unless a caller has established otherwise.
   static const int STRAIGHT[3] = { 0, 1, 2 };
   const int *order = (componentOrder != 0) ? componentOrder : STRAIGHT;

   irrep.clear();
   if (orbitals.empty() || perAtom.empty() || ops.empty()) return false;
   if (classOfOp.size() != ops.size()) return false;
   if (images.size() != ops.size()) return false;

   //  Where each atom's functions start, and a check that the layout
   //  and the coefficients describe the same basis.
   vector<int> base(perAtom.size(), 0);
   int total = 0;
   for (size_t a = 0; a < perAtom.size(); a++) {
      base[a] = total;
      total += perAtom[a];
   }
   if ((int)shellOf.size() != total) return false;
   for (size_t k = 0; k < orbitals.size(); k++) {
      if ((int)orbitals[k].size() != total) return false;
   }

   //  A WARNING THAT COST A NIGHT ELSEWHERE.  The coefficients and
   //  the coordinates must be in the SAME FRAME.  A code is free to
   //  reorient a molecule for its own SCF and report its vectors in
   //  that orientation, and the symmetrised coordinates this analysis
   //  runs on come from autosym -- so the two can differ by a
   //  rotation.  Water's a1 orbitals then classify cleanly, because a
   //  rotation about z leaves s and p_z alone, while its b1 and b2
   //  come out as fractional mixtures of each other, because that
   //  rotation mixes p_x and p_y.  Fractions are what the check below
   //  refuses, so this fails safe rather than labelling wrongly; but
   //  a caller wanting labels for such a calculation has to rotate
   //  the coefficients into this frame first.
   //
   //  s AND p ONLY.  A d shell transforms by its own five-by-five
   //  matrix, which is not derived here, and guessing it would be
   //  worse than declining.
   for (int i = 0; i < total; i++) {
      if (shellOf[i] != 0 && shellOf[i] != 1) return false;
   }

   //  The atoms have to carry the same layout for an operation to map
   //  a function onto a function: offset o on an atom must be the same
   //  kind of function as offset o on its image.  True when equivalent
   //  atoms are the same element, which atomImages() already enforces,
   //  but checked rather than assumed.
   for (size_t op = 0; op < ops.size(); op++) {
      for (size_t a = 0; a < perAtom.size(); a++) {
         const int to = images[op][a];
         if (to < 0 || (size_t)to >= perAtom.size()) return false;
         if (perAtom[to] != perAtom[a]) return false;
      }
   }

   const size_t numClasses = table.classes().size();
   vector<double> chi(numClasses, 0.0);
   vector<bool> filled(numClasses, false);

   for (size_t op = 0; op < ops.size(); op++) {
      const int klass = classOfOp[op];
      if (klass < 0 || (size_t)klass >= numClasses) return false;
      if (filled[klass]) continue;

      double character = 0.0;

      for (size_t k = 0; k < orbitals.size(); k++) {
         const vector<double>& c = orbitals[k];

         //  R applied to this orbital, expanded in the same basis.
         vector<double> moved(total, 0.0);

         for (size_t a = 0; a < perAtom.size(); a++) {
            const int to = images[op][a];
            int offset = 0;
            while (offset < perAtom[a]) {
               const int from = base[a] + offset;
               const int l = shellOf[from];

               if (l == 0) {
                  moved[base[to] + offset] += c[from];
                  offset += 1;
               } else {
                  //  p transforms as x, y, z: R p_a = sum_b M[b][a] p_b,
                  //  so the image picks up M[b][a] times this one.
                  if (offset + 2 >= perAtom[a]) return false;
                  for (int b = 0; b < 3; b++) {
                     double sum = 0.0;
                     for (int aa = 0; aa < 3; aa++) {
                        sum += ops[op].m[b][aa]
                             * c[base[a] + offset + order[aa]];
                     }
                     moved[base[to] + offset + order[b]] += sum;
                  }
                  offset += 3;
               }
            }
         }

         for (int i = 0; i < total; i++) character += c[i]*moved[i];
      }

      chi[klass] = character;
      filled[klass] = true;
   }

   for (size_t k = 0; k < numClasses; k++) if (!filled[k]) return false;

   //  REDUCED WITH A TOLERANCE, BECAUSE THESE ARE COMPUTED ORBITALS.
   //  See reduceToOneIrrep() (shared with fullOrbitalIrrep() below) for
   //  why: an SCF orbital is symmetry-adapted only to the convergence
   //  it was run to, so CharacterTable::reduce()'s strict integrality
   //  would reject nearly all of them -- the strict test rejected five
   //  of water's six orbitals, the one it accepted being the
   //  non-bonding lone pair, exactly symmetric by having nothing to
   //  mix with.
   return reduceToOneIrrep(chi, numClasses, table, (int)orbitals.size(), irrep);
}


int SymmetryAnalysis::labelSpectrum(const vector< vector<double> >& orbitals,
                                    const vector<double>& energies,
                                    const vector<string>& reported,
                                    const vector<int>& perAtom,
                                    const vector<int>& shellOf,
                                    const vector< vector<int> >& images,
                                    const vector<int>& classOfOp,
                                    const vector<SymOp>& ops,
                                    const CharacterTable& table,
                                    vector<string>& derived)
{
   derived.assign(orbitals.size(), string());
   if (orbitals.empty()) return 0;

   //  Degenerate sets have to be classified together: only the set
   //  has a character.  Grouped by energy, which is what degeneracy
   //  means in a spectrum.
   vector< vector<size_t> > sets;
   for (size_t i = 0; i < orbitals.size(); ) {
      size_t j = i;
      //  A COMPUTED DEGENERACY IS NOT AN EXACT ONE.  Orbitals that
      //  symmetry makes degenerate come out of a diagonalisation
      //  agreeing to however many digits it converged to, not to all
      //  of them: ammonia's e pair differs in the fourth, and at a
      //  tolerance of a millionth they were classified separately,
      //  each giving a fractional reduction and no label.
      while (j + 1 < orbitals.size() && j + 1 < energies.size() &&
             fabs(energies[j+1] - energies[i]) < 1.0e-3) {
         j++;
      }
      vector<size_t> one;
      for (size_t k = i; k <= j; k++) one.push_back(k);
      sets.push_back(one);
      i = j + 1;
   }

   //  THE SIX ORDERS THE THREE p FUNCTIONS CAN BE WRITTEN IN.
   //
   //  Finite, so this terminates: every candidate is tried once, the
   //  best is kept, and if none fits the answer is that none fits.
   //  Getting the order wrong does not fail loudly -- it returns
   //  orbitals that are fractional mixtures of two irreps, which is
   //  exactly what water's b1 and b2 did against the assumed order.
   static const int CANDIDATES[6][3] = {
      { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 },
      { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 }
   };

   int bestCandidate = -1, bestLabelled = -1, bestAgreed = -1;

   for (int k = 0; k < 6; k++) {
      int labelled = 0, agreed = 0, clashed = 0;

      for (size_t s = 0; s < sets.size(); s++) {
         vector< vector<double> > set;
         for (size_t m = 0; m < sets[s].size(); m++) {
            set.push_back(orbitals[sets[s][m]]);
         }

         string irrep;
         if (!orbitalIrrep(set, perAtom, shellOf, images, classOfOp, ops,
                           table, irrep, CANDIDATES[k])) {
            continue;
         }
         labelled += (int)sets[s].size();

         if (!reported.empty() && sets[s][0] < reported.size()) {
            const string& said = reported[sets[s][0]];
            if (!said.empty()) {
               //  Compared without case, since no two sources agree
               //  on it.
               string a, b;
               for (size_t c = 0; c < said.size(); c++) {
                  a += (char)toupper((unsigned char)said[c]);
               }
               for (size_t c = 0; c < irrep.size(); c++) {
                  b += (char)toupper((unsigned char)irrep[c]);
               }
               if (a == b) agreed++; else clashed++;
            }
         }
      }

      //  With reported labels to check against, a candidate that
      //  contradicts them is wrong however much it labels.
      if (!reported.empty() && clashed > agreed) continue;

      if (agreed > bestAgreed ||
          (agreed == bestAgreed && labelled > bestLabelled)) {
         bestAgreed = agreed;
         bestLabelled = labelled;
         bestCandidate = k;
      }
   }

   if (bestCandidate < 0) return 0;

   int labelled = 0;
   for (size_t s = 0; s < sets.size(); s++) {
      vector< vector<double> > set;
      for (size_t m = 0; m < sets[s].size(); m++) {
         set.push_back(orbitals[sets[s][m]]);
      }
      string irrep;
      if (!orbitalIrrep(set, perAtom, shellOf, images, classOfOp, ops,
                        table, irrep, CANDIDATES[bestCandidate])) {
         continue;
      }
      for (size_t m = 0; m < sets[s].size(); m++) {
         derived[sets[s][m]] = irrep;
         labelled++;
      }
   }
   return labelled;
}


bool SymmetryAnalysis::alignFrames(const vector<double>& from,
                                   const vector<double>& to,
                                   SymOp& R,
                                   double& rmsd)
{
   R = identityOp();
   rmsd = 0.0;

   //  Two atoms are enough: a diatomic is collinear, the fit fixes only
   //  its axis, and the spin about that axis the degenerate-direction
   //  fill-in below picks is as good as any other, because a linear
   //  molecule is symmetric under all of them.  Requiring three atoms
   //  refused every diatomic (CO, N2) outright.
   if (from.size() != to.size() || from.size() < 6 || from.size() % 3 != 0)
      return false;
   const size_t n = from.size() / 3;

   double cFrom[3] = { 0.0, 0.0, 0.0 };
   double cTo[3]   = { 0.0, 0.0, 0.0 };
   for (size_t i = 0; i < n; i++) {
      for (int k = 0; k < 3; k++) {
         cFrom[k] += from[i*3+k];
         cTo[k]   += to[i*3+k];
      }
   }
   for (int k = 0; k < 3; k++) { cFrom[k] /= n; cTo[k] /= n; }

   //  H = sum_i (from_i - cFrom) (to_i - cTo)^T, a 3x3 covariance --
   //  the Kabsch cross-covariance matrix.
   double H[3][3] = { {0,0,0}, {0,0,0}, {0,0,0} };
   for (size_t i = 0; i < n; i++) {
      double p[3], q[3];
      for (int k = 0; k < 3; k++) {
         p[k] = from[i*3+k] - cFrom[k];
         q[k] = to[i*3+k]   - cTo[k];
      }
      for (int a = 0; a < 3; a++)
         for (int b = 0; b < 3; b++)
            H[a][b] += p[a]*q[b];
   }

   //  R = V * diag(sign) * U^T from the SVD H = U*Sigma*V^T.  Rather
   //  than a general SVD, diagonalise the 3x3 symmetric H^T H = V *
   //  Sigma^2 * V^T (Jacobi, above), then recover U's columns as
   //  H*v_i / sigma_i wherever sigma_i is not degenerately small.
   double HtH[3][3];
   for (int a = 0; a < 3; a++)
      for (int b = 0; b < 3; b++) {
         double s = 0.0;
         for (int k = 0; k < 3; k++) s += H[k][a]*H[k][b];
         HtH[a][b] = s;
      }

   double sigma2[3], V[3][3];
   jacobiEigen3(HtH, sigma2, V);

   //  Sort descending by singular value -- Jacobi returns them in
   //  whatever order the sweeps left them, and the smallest one is the
   //  one a near-planar molecule can leave numerically ambiguous, so it
   //  must be LAST, not wherever it happened to land.
   int order[3] = { 0, 1, 2 };
   for (int a = 0; a < 3; a++)
      for (int b = a+1; b < 3; b++)
         if (sigma2[order[b]] > sigma2[order[a]]) std::swap(order[a], order[b]);

   double U[3][3];
   double sigma[3];
   for (int oi = 0; oi < 3; oi++) {
      const int i = order[oi];
      sigma[oi] = sqrt(sigma2[i] > 0.0 ? sigma2[i] : 0.0);
      double col[3] = { 0.0, 0.0, 0.0 };
      for (int a = 0; a < 3; a++)
         for (int b = 0; b < 3; b++) col[a] += H[a][b]*V[b][i];

      if (sigma[oi] > 1.0e-9) {
         for (int a = 0; a < 3; a++) U[a][oi] = col[a]/sigma[oi];
      } else {
         for (int a = 0; a < 3; a++) U[a][oi] = 0.0;   // filled in below
      }
   }
   //  Degenerate directions (planar or linear input): complete U to an
   //  orthonormal basis.  The fill-in has to be chosen AGAINST the
   //  columns already there, not by column index -- the old code put
   //  e_x/e_y/e_z into columns 0/1/2, so a diatomic stored along z (how
   //  ECCE stores CO and N2) had e_z Gram-Schmidted to the zero vector,
   //  giving a SINGULAR R that still fitted both atoms exactly.  Every
   //  operation conjugated through it was then garbage and ORCA's linear
   //  molecules labelled nothing.  So: the second column, if missing, is
   //  the coordinate axis least parallel to the first, orthogonalised;
   //  the third, if missing, is the cross product of the first two.
   //  (sigma is sorted descending, so a missing column is never
   //  followed by a present one, and column 0 is present for any input
   //  that is not all atoms on one point.)
   if (sigma[0] <= 1.0e-9) return false;
   if (sigma[1] > 1.0e-9) {
      //  Present but possibly small (near-linear input): keep it
      //  orthonormal to column 0 regardless.
      double dot = 0.0, norm = 0.0;
      for (int a = 0; a < 3; a++) dot += U[a][1]*U[a][0];
      for (int a = 0; a < 3; a++) { U[a][1] -= dot*U[a][0]; norm += U[a][1]*U[a][1]; }
      norm = sqrt(norm);
      if (norm < 1.0e-6) sigma[1] = 0.0;
      else for (int a = 0; a < 3; a++) U[a][1] /= norm;
   }
   if (sigma[1] <= 1.0e-9) {
      int best = 0;
      for (int a = 1; a < 3; a++) if (fabs(U[a][0]) < fabs(U[best][0])) best = a;
      double e[3] = { 0.0, 0.0, 0.0 };
      e[best] = 1.0;
      const double dot = e[0]*U[0][0] + e[1]*U[1][0] + e[2]*U[2][0];
      double norm = 0.0;
      for (int a = 0; a < 3; a++) { U[a][1] = e[a] - dot*U[a][0]; norm += U[a][1]*U[a][1]; }
      norm = sqrt(norm);
      for (int a = 0; a < 3; a++) U[a][1] /= norm;
   }
   if (sigma[2] <= 1.0e-9 || sigma[1] <= 1.0e-9) {
      U[0][2] = U[1][0]*U[2][1] - U[2][0]*U[1][1];
      U[1][2] = U[2][0]*U[0][1] - U[0][0]*U[2][1];
      U[2][2] = U[0][0]*U[1][1] - U[1][0]*U[0][1];
   }

   double Vsorted[3][3];
   for (int oi = 0; oi < 3; oi++)
      for (int a = 0; a < 3; a++) Vsorted[a][oi] = V[a][order[oi]];

   //  d = sign needed to make R a PROPER rotation (det +1) rather than
   //  a reflection -- the Kabsch algorithm's one well-known gotcha.
   double detU = U[0][0]*(U[1][1]*U[2][2]-U[1][2]*U[2][1])
               - U[0][1]*(U[1][0]*U[2][2]-U[1][2]*U[2][0])
               + U[0][2]*(U[1][0]*U[2][1]-U[1][1]*U[2][0]);
   double detV = Vsorted[0][0]*(Vsorted[1][1]*Vsorted[2][2]-Vsorted[1][2]*Vsorted[2][1])
               - Vsorted[0][1]*(Vsorted[1][0]*Vsorted[2][2]-Vsorted[1][2]*Vsorted[2][0])
               + Vsorted[0][2]*(Vsorted[1][0]*Vsorted[2][1]-Vsorted[1][1]*Vsorted[2][0]);
   const double d = (detU*detV < 0.0) ? -1.0 : 1.0;

   for (int a = 0; a < 3; a++) {
      for (int b = 0; b < 3; b++) {
         double s = 0.0;
         for (int k = 0; k < 3; k++) {
            const double sign = (k == 2) ? d : 1.0;
            s += Vsorted[a][k]*sign*U[b][k];
         }
         R.m[a][b] = s;
      }
   }

   //  COLLINEAR: the fit fixes only the axis (u0 -> v0), and the spin
   //  about it that the Kabsch completion above lands on is whatever
   //  the degenerate eigenvectors happened to be.  Take the SMALLEST
   //  rotation instead -- the identity when the two axes already agree,
   //  which is the common case (ORCA and autosym both put a linear
   //  molecule on z).  Any spin fits the atoms equally well, but only
   //  this one leaves the coefficient frame's own x and y where the
   //  code put them, which the subgroup cross-check (ORCA's C2v/D2h
   //  labels, built from diagonal operations in the stored frame)
   //  needs: a spin of, say, 30 degrees gives a C4v whose mirrors are
   //  not ORCA's, and the cross-check fails for no chemical reason.
   if (sigma[1] <= 1.0e-9) {
      const double a[3] = { U[0][0], U[1][0], U[2][0] };
      const double b[3] = { Vsorted[0][0], Vsorted[1][0], Vsorted[2][0] };
      const double c = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
      if (c > -1.0 + 1.0e-9) {
         //  Rodrigues: R = I + [v]x + [v]x^2 / (1 + c), v = a x b.
         const double v[3] = { a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2],
                               a[0]*b[1]-a[1]*b[0] };
         const double K[3][3] = { {  0.0, -v[2],  v[1] },
                                  {  v[2],  0.0, -v[0] },
                                  { -v[1],  v[0],  0.0 } };
         for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
               double k2 = 0.0;
               for (int k = 0; k < 3; k++) k2 += K[i][k]*K[k][j];
               R.m[i][j] = (i == j ? 1.0 : 0.0) + K[i][j] + k2/(1.0 + c);
            }
      } else {
         //  Antiparallel: a half-turn about the coordinate axis least
         //  parallel to a, made perpendicular to it.
         int best = 0;
         for (int i = 1; i < 3; i++) if (fabs(a[i]) < fabs(a[best])) best = i;
         double k[3] = { 0.0, 0.0, 0.0 };
         k[best] = 1.0;
         const double dot = k[0]*a[0] + k[1]*a[1] + k[2]*a[2];
         double norm = 0.0;
         for (int i = 0; i < 3; i++) { k[i] -= dot*a[i]; norm += k[i]*k[i]; }
         norm = sqrt(norm);
         for (int i = 0; i < 3; i++) k[i] /= norm;
         for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
               R.m[i][j] = 2.0*k[i]*k[j] - (i == j ? 1.0 : 0.0);
      }
   }

   double sse = 0.0;
   for (size_t i = 0; i < n; i++) {
      double p[3], q[3], rp[3];
      for (int k = 0; k < 3; k++) {
         p[k] = from[i*3+k] - cFrom[k];
         q[k] = to[i*3+k]   - cTo[k];
      }
      apply(R, p, rp);
      for (int k = 0; k < 3; k++) sse += (rp[k]-q[k])*(rp[k]-q[k]);
   }
   rmsd = sqrt(sse / n);

   return true;
}


SymOp SymmetryAnalysis::conjugate(const SymOp& R, const SymOp& Q)
{
   return multiply(transpose(Q), multiply(R, Q));
}


namespace {

  /** One atom's shells: consecutive runs of shellTypeOf sharing one
   *  value, angfunc->numFuncs(value) functions wide -- exactly how
   *  BasisFlatten::flatten() emits them. */
  struct AtomShell { int offset; int shellType; int width; };

  bool segmentAtomShells(const vector<int>& shellTypeOf, int base, int width,
                        TGBSAngFunc *angfunc, vector<AtomShell>& shells)
  {
     shells.clear();
     int offset = 0;
     while (offset < width) {
        const int st = shellTypeOf[base + offset];
        const int m = angfunc->numFuncs(st);
        if (m <= 0 || offset + m > width) return false;
        for (int t = 1; t < m; t++)
           if (shellTypeOf[base + offset + t] != st) return false;
        AtomShell s; s.offset = offset; s.shellType = st; s.width = m;
        shells.push_back(s);
        offset += m;
     }
     return true;
  }
}


bool SymmetryAnalysis::fullOrbitalIrrep(const vector< vector<double> >& orbitals,
                                        const vector<int>& perAtom,
                                        const vector<int>& shellTypeOf,
                                        const vector< vector<double> >& overlap,
                                        const vector< vector<int> >& images,
                                        const vector<int>& classOfOp,
                                        const vector<SymOp>& opsInCoeffFrame,
                                        TGBSAngFunc *angfunc,
                                        const CharacterTable& table,
                                        double shellResidualTol,
                                        string& irrep)
{
   irrep.clear();
   if (angfunc == 0) return false;
   if (orbitals.empty() || perAtom.empty() || opsInCoeffFrame.empty()) return false;
   if (classOfOp.size() != opsInCoeffFrame.size()) return false;
   if (images.size() != opsInCoeffFrame.size()) return false;

   vector<int> base(perAtom.size(), 0);
   int total = 0;
   for (size_t a = 0; a < perAtom.size(); a++) { base[a] = total; total += perAtom[a]; }

   if ((int)shellTypeOf.size() != total) return false;
   if (overlap.size() != (size_t)total) return false;
   for (int i = 0; i < total; i++) if ((int)overlap[i].size() != total) return false;
   for (size_t k = 0; k < orbitals.size(); k++)
      if ((int)orbitals[k].size() != total) return false;

   //  Segment every atom's functions into shells, and check the
   //  operations map one atom's shell sequence onto its image's --
   //  the general-shell analogue of orbitalIrrep()'s perAtom[to] check.
   vector< vector<AtomShell> > shells(perAtom.size());
   for (size_t a = 0; a < perAtom.size(); a++) {
      if (!segmentAtomShells(shellTypeOf, base[a], perAtom[a], angfunc, shells[a]))
         return false;
   }
   for (size_t op = 0; op < opsInCoeffFrame.size(); op++) {
      for (size_t a = 0; a < perAtom.size(); a++) {
         const int to = images[op][a];
         if (to < 0 || (size_t)to >= perAtom.size()) return false;
         if (shells[to].size() != shells[a].size()) return false;
         for (size_t s = 0; s < shells[a].size(); s++) {
            if (shells[to][s].offset != shells[a][s].offset ||
                shells[to][s].shellType != shells[a][s].shellType ||
                shells[to][s].width != shells[a][s].width) return false;
         }
      }
   }

   //  D(R) for each (operation, shell type) actually used -- computed
   //  ONCE per pair, since a shell's angular transformation is atom-
   //  and contraction-independent (see ShellRotation.H).
   map< std::pair<int,int>, vector< vector<double> > > dcache;
   for (size_t op = 0; op < opsInCoeffFrame.size(); op++) {
      for (size_t a = 0; a < shells.size(); a++) {
         for (size_t s = 0; s < shells[a].size(); s++) {
            const int st = shells[a][s].shellType;
            std::pair<int,int> key((int)op, st);
            if (dcache.find(key) != dcache.end()) continue;
            vector< vector<double> > D;
            if (!ShellRotation::buildD(st, angfunc, opsInCoeffFrame[op],
                                       shellResidualTol, D))
               return false;
            dcache[key] = D;
         }
      }
   }

   const size_t numClasses = table.classes().size();
   vector<double> chi(numClasses, 0.0);
   vector<bool> filled(numClasses, false);

   for (size_t op = 0; op < opsInCoeffFrame.size(); op++) {
      const int klass = classOfOp[op];
      if (klass < 0 || (size_t)klass >= numClasses) return false;
      if (filled[klass]) continue;

      double numerator = 0.0, denominator = 0.0;

      for (size_t k = 0; k < orbitals.size(); k++) {
         const vector<double>& c = orbitals[k];
         vector<double> moved(total, 0.0);

         for (size_t a = 0; a < shells.size(); a++) {
            const int to = images[op][a];
            for (size_t s = 0; s < shells[a].size(); s++) {
               const AtomShell& sh = shells[a][s];
               const vector< vector<double> >& D =
                  dcache[std::pair<int,int>((int)op, sh.shellType)];
               for (int row = 0; row < sh.width; row++) {
                  double sum = 0.0;
                  for (int col = 0; col < sh.width; col++)
                     sum += D[row][col]*c[base[a]+sh.offset+col];
                  moved[base[to]+sh.offset+row] += sum;
               }
            }
         }

         //  c^T S moved, and c^T S c for the normalisation -- see
         //  fullOrbitalIrrep()'s header comment for why the latter is
         //  a no-op for properly normalised orbitals and a safety net
         //  otherwise.
         for (int i = 0; i < total; i++) {
            if (c[i] == 0.0) continue;
            for (int j = 0; j < total; j++) {
               numerator   += c[i]*overlap[i][j]*moved[j];
               denominator += c[i]*overlap[i][j]*c[j];
            }
         }
      }

      if (fabs(denominator) < 1.0e-10) return false;
      chi[klass] = numerator/denominator * (double)orbitals.size();
      filled[klass] = true;
   }

   for (size_t k = 0; k < numClasses; k++) if (!filled[k]) return false;

   return reduceToOneIrrep(chi, numClasses, table, (int)orbitals.size(), irrep);
}


int SymmetryAnalysis::fullLabelSpectrum(const vector< vector<double> >& orbitals,
                                        const vector<double>& energies,
                                        const vector<int>& perAtom,
                                        const vector<int>& shellTypeOf,
                                        const vector< vector<double> >& overlap,
                                        const vector< vector<int> >& images,
                                        const vector<int>& classOfOp,
                                        const vector<SymOp>& opsInCoeffFrame,
                                        TGBSAngFunc *angfunc,
                                        const CharacterTable& table,
                                        double shellResidualTol,
                                        vector<string>& derived)
{
   derived.assign(orbitals.size(), string());
   if (orbitals.empty()) return 0;

   //  Degenerate sets, grouped by energy exactly as labelSpectrum()
   //  does -- see that function's comment for why the tolerance is
   //  1e-3 and not tighter.
   vector< vector<size_t> > sets;
   for (size_t i = 0; i < orbitals.size(); ) {
      size_t j = i;
      while (j + 1 < orbitals.size() && j + 1 < energies.size() &&
             fabs(energies[j+1] - energies[i]) < 1.0e-3) {
         j++;
      }
      vector<size_t> one;
      for (size_t k = i; k <= j; k++) one.push_back(k);
      sets.push_back(one);
      i = j + 1;
   }

   int labelled = 0;
   for (size_t s = 0; s < sets.size(); s++) {
      vector< vector<double> > set;
      for (size_t m = 0; m < sets[s].size(); m++) set.push_back(orbitals[sets[s][m]]);

      string irrepName;
      if (fullOrbitalIrrep(set, perAtom, shellTypeOf, overlap, images, classOfOp,
                           opsInCoeffFrame, angfunc, table, shellResidualTol,
                           irrepName)) {
         for (size_t m = 0; m < sets[s].size(); m++) {
            derived[sets[s][m]] = irrepName;
            labelled++;
         }
         continue;
      }

      //  The set is not one irrep.  An energy window groups orbitals
      //  that are only NEARLY degenerate -- N2's 1sigma_g/1sigma_u core
      //  pair is 0.9 mEh apart -- and a stand-in group splits what the
      //  real group keeps together: C4v has delta as B1 + B2.  In both
      //  cases each orbital on its own spans a one-dimensional irrep,
      //  so label them one at a time.  This cannot mislabel a genuinely
      //  degenerate set: a component of an irrep of dimension >= 2
      //  never reduces to a one-dimensional irrep (fullOrbitalIrrep()
      //  samples one operation from every class, those generate the
      //  whole group, and a vector each of them sends to +/- itself
      //  would span an invariant line), so such a set stays declined.
      if (sets[s].size() < 2) continue;
      for (size_t m = 0; m < sets[s].size(); m++) {
         vector< vector<double> > one(1, orbitals[sets[s][m]]);
         string single;
         if (!fullOrbitalIrrep(one, perAtom, shellTypeOf, overlap, images,
                               classOfOp, opsInCoeffFrame, angfunc, table,
                               shellResidualTol, single))
            continue;
         derived[sets[s][m]] = single;
         labelled++;
      }
   }
   return labelled;
}


namespace {

  SymOp diagOp(double x, double y, double z)
  {
     SymOp op;
     for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) op.m[i][j] = 0.0;
     op.m[0][0] = x; op.m[1][1] = y; op.m[2][2] = z;
     return op;
  }

  /**
   * The D2h-family operations, in the STORED frame, for one of the six
   * ways to assign the three physical C2 axes (z,y,x in some order) to
   * the character table's three ambiguous C2 (and, for D2H, sigma)
   * columns.  perm is a permutation of {0,1,2} indexing (C2z,C2y,C2x).
   *
   * Builds D2h and every subgroup of it (D2, C2v, C2h, C2, Cs, Ci) --
   * the groups an abelian-only code can report.  Returns false for
   * anything else.
   */
  bool d2hFamilyOps(const string& groupName, const int perm[3],
                    vector<SymOp>& ops)
  {
     ops.clear();
     SymOp C2[3];
     C2[0] = diagOp(-1, -1,  1);   // C2(z)
     C2[1] = diagOp(-1,  1, -1);   // C2(y)
     C2[2] = diagOp( 1, -1, -1);   // C2(x)

     string g = groupName;
     for (size_t i = 0; i < g.size(); i++) g[i] = toupper((unsigned char)g[i]);

     if (g == "D2") {
        ops.push_back(diagOp(1,1,1));
        for (int k = 0; k < 3; k++) ops.push_back(C2[perm[k]]);
        return true;
     }
     if (g == "D2H") {
        SymOp inv = diagOp(-1,-1,-1);
        ops.push_back(diagOp(1,1,1));
        for (int k = 0; k < 3; k++) ops.push_back(C2[perm[k]]);
        ops.push_back(inv);
        //  sigma_k = i * C2_k (elementwise, since these are diagonal):
        //  the mirror PAIRED with that C2 axis, which is what makes
        //  the class order agree with the table's own B1g/B2g/B3g
        //  pairing of one C2 class with one sigma class.
        for (int k = 0; k < 3; k++) {
           const SymOp& c = C2[perm[k]];
           ops.push_back(diagOp(inv.m[0][0]*c.m[0][0],
                                inv.m[1][1]*c.m[1][1],
                                inv.m[2][2]*c.m[2][2]));
        }
        return true;
     }

     //  The rest of D2h's subgroups, which is where ORCA lands whenever
     //  the structure it was given was not quite symmetric (benzene
     //  built by hand came out Cs, #151).  perm[0] picks the principal
     //  C2 axis -- or, for Cs, the axis the mirror is perpendicular to --
     //  and perm[1], perm[2] order the two vertical mirrors of C2v; the
     //  caller tries every assignment, as for D2.
     SymOp E = diagOp(1,1,1);
     SymOp inv = diagOp(-1,-1,-1);
     SymOp sigma[3];                // sigma_k: the mirror perpendicular to C2_k
     for (int k = 0; k < 3; k++)
        sigma[k] = diagOp(-C2[k].m[0][0], -C2[k].m[1][1], -C2[k].m[2][2]);

     if (g == "C2") {
        ops.push_back(E); ops.push_back(C2[perm[0]]);
        return true;
     }
     if (g == "CI") {
        ops.push_back(E); ops.push_back(inv);
        return true;
     }
     if (g == "CS") {
        ops.push_back(E); ops.push_back(sigma[perm[0]]);
        return true;
     }
     if (g == "C2H") {
        ops.push_back(E); ops.push_back(C2[perm[0]]);
        ops.push_back(inv); ops.push_back(sigma[perm[0]]);
        return true;
     }
     if (g == "C2V") {
        //  The two mirrors CONTAIN the C2 axis, i.e. each is
        //  perpendicular to one of the other two axes.
        ops.push_back(E); ops.push_back(C2[perm[0]]);
        ops.push_back(sigma[perm[1]]); ops.push_back(sigma[perm[2]]);
        return true;
     }
     return false;
  }

  //  Case-folded, and with ORCA's and Gaussian's double-quote spelling
  //  of a double prime (A") made the character table's A''.  Without
  //  that every A'' orbital of a Cs calculation read as a disagreement.
  string upper(const string& s)
  {
     string r;
     for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '"') r += "''";
        else r += (char)toupper((unsigned char)s[i]);
     }
     return r;
  }
}


bool SymmetryAnalysis::subgroupOperations(const string& groupName,
                                          const int perm[3],
                                          vector<SymOp>& ops)
{
   return d2hFamilyOps(groupName, perm, ops);
}


int SymmetryAnalysis::subgroupLabelSpectrum(
    const vector< vector<double> >& orbitals,
    const vector<string>& reported,
    const vector<int>& perAtom,
    const vector<int>& shellTypeOf,
    const vector< vector<double> >& overlap,
    const vector<string>& elements,
    const vector<double>& storedCoords,
    TGBSAngFunc *angfunc,
    const CharacterTable& subgroupTable,
    double atomTolerance,
    double shellResidualTol,
    vector<string>& derived,
    string& axisNote)
{
   derived.assign(orbitals.size(), string());
   axisNote.clear();

   static const int PERMS[6][3] = {
      {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0}
   };
   static const char* AXISNAME[3] = { "z", "y", "x" };

   int bestPerm = -1, bestAgreed = -1, bestLabelled = -1;
   vector<string> bestDerived;

   for (int p = 0; p < 6; p++) {
      vector<SymOp> ops;
      if (!d2hFamilyOps(subgroupTable.name(), PERMS[p], ops)) return -1;
      if ((int)ops.size() != subgroupTable.order()) continue;

      vector< vector<int> > images;
      if (!SymmetryAnalysis::atomImages(storedCoords, elements, ops,
                                        atomTolerance, images))
         continue;

      vector< vector<int> > classes;
      SymmetryAnalysis::conjugacyClasses(ops, classes);
      vector<int> classOfOp;
      if (!SymmetryAnalysis::matchClasses(ops, classes, subgroupTable, classOfOp))
         continue;

      vector<string> candidate(orbitals.size());
      int labelled = 0, agreed = 0, clashed = 0;
      for (size_t k = 0; k < orbitals.size(); k++) {
         vector< vector<double> > one(1, orbitals[k]);
         string irrep;
         if (!fullOrbitalIrrep(one, perAtom, shellTypeOf, overlap, images,
                              classOfOp, ops, angfunc, subgroupTable,
                              shellResidualTol, irrep))
            continue;
         candidate[k] = irrep;
         labelled++;

         if (k < reported.size() && !reported[k].empty()) {
            if (upper(reported[k]) == upper(irrep)) agreed++; else clashed++;
         }
      }

      if (!reported.empty() && clashed > agreed) continue;
      if (agreed > bestAgreed || (agreed == bestAgreed && labelled > bestLabelled)) {
         bestAgreed = agreed;
         bestLabelled = labelled;
         bestPerm = p;
         bestDerived = candidate;
      }
   }

   if (bestPerm < 0) return 0;

   derived = bestDerived;
   char msg[160];
   snprintf(msg, sizeof(msg),
            "%s's C2 axes assigned as (1st table C2)=%s, (2nd)=%s, (3rd)=%s "
            "-- chosen by agreement with the code's own labels",
            subgroupTable.name().c_str(), AXISNAME[PERMS[bestPerm][0]],
            AXISNAME[PERMS[bestPerm][1]], AXISNAME[PERMS[bestPerm][2]]);
   axisNote = msg;

   int labelled = 0;
   for (size_t k = 0; k < derived.size(); k++) if (!derived[k].empty()) labelled++;
   return labelled;
}
