#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cctype>

#include "tdat/SymmetryAnalysis.H"
#include "tdat/CharacterTable.H"

#include <map>
using std::map;

namespace {

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
