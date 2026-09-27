///////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: BasisFlatten.C
//
// The atomic-orbital basis flattening shared by ComputeMoCmd (the ESP
// field evaluator, src/viz/propsgcommands/ComputeMoCmd.C) and the
// full-point-group MO symmetry projection (SymmetryAnalysis, #147/#151).
//
// normalize(), getoddNormalize() and flatten() below are moved here
// VERBATIM from ComputeMoCmd::normalize(), ::getoddNormalize() and the
// body of ::buildEspBasis() -- same arithmetic, same order, on purpose:
// this is the "ONE implementation, not a copy" the two callers share,
// so an MO diagram and an ESP surface built from the same calculation
// can never silently disagree about what a basis function is.
// ComputeMoCmd's own methods of these names now just forward here.
///////////////////////////////////////////////////////////////////////////

#include "tdat/BasisFlatten.H"

#include <cmath>

#include "util/ErrMsg.H"

#include "tdat/TGBSAngFunc.H"
#include "tdat/EspField.H"

#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGBSGroup.H"
#include "dsm/JCode.H"

namespace {
  const double pi   = acos(-1.0);
  const double pi32 = pi * sqrt(pi);
}

namespace BasisFlatten {

vector<TGaussianBasisSet*> normalize(
    const string& atomID,
    const vector<const TGaussianBasisSet*>& gbslist,
    TGBSConfig& gbsConfig,
    const JCode* code)
{
  vector<double> alpha;
  vector<TGaussianBasisSet::AngularMomentum> funcTypes;
  unsigned long ialpha;
  vector<TGaussianBasisSet*> normalized;
  GBSToContInfoMap* infoMap;
  string uniqueKey;
  GBSToContInfoMap::iterator infoIt;
  ContractionInfo* contInfo;

  // TEST - Make a new config for testing:
  TGBSConfig test;
  test.optimize(true);
  TGBSGroup* group = new TGBSGroup();
  //

  // Do the normalization step
  vector<const TGaussianBasisSet*>::const_iterator gbscurs1;
  for (gbscurs1 = gbslist.begin();
        gbscurs1 != gbslist.end(); gbscurs1++)
  {
    const TGaussianBasisSet *gbs = *gbscurs1;

    // Get the delete/uncontract meta data for that element and basis set
    // (used in amica, but somebody else may have used it)
    infoMap = gbsConfig.getContractionInfoMap(atomID);
    uniqueKey = gbs->getUniqueKey();
    contInfo = 0;

    if (infoMap != 0) {
      infoIt = infoMap->find(uniqueKey);
      if (infoIt != infoMap->end()) {
        contInfo = &((*infoIt).second);
      }
    }

    // Make a new basis set that has the contractions correctly
    // optimized:
    TGaussianBasisSet *tmp = new TGaussianBasisSet(*gbs, atomID,
                                                   gbsConfig.optimize(),
                                                   code, contInfo);
    // TEST
    group->insertGBS(new TGaussianBasisSet(*tmp));
    //
    unsigned long numContractedSets = 0;
    numContractedSets = tmp->num_contracted_sets(atomID.c_str());

    // For each contracted basis set:
    for (unsigned long ics = 0; ics < numContractedSets; ics++)
    {
      alpha = tmp->exponents(atomID.c_str(), ics);
      int numAlpha = alpha.size();

      // funcTypes: rf. ChemTypes.h enum: s_shell,p_shell, etc.
      funcTypes = tmp->func_types(atomID.c_str(), ics);
      int numFuncTypes = funcTypes.size();
      Contraction_ *cont = tmp->getContraction(atomID.c_str(), ics);

      //
      // NORMP step; unnormalization of the primitive functions;
      // if contraction coefficients are given in normalized primitive
      // functions, change them for unnormalized primitives
      // This is generally true and so I do not check, may have
      // to check for generalized basis sets. The scale factor inside
      // the switch's sqrt comes from (2l-1)!!/pow(2.0,l) - TLW
      //
      int icol;
      for (icol = 0; icol < numFuncTypes; icol++) {
        for (ialpha = 0; ialpha < numAlpha; ialpha++) {

          double ee = 2 * alpha[ialpha];
          double facs = pi32 / (ee * sqrt(ee));

          switch (funcTypes[icol]) {
            case TGaussianBasisSet::s_shell:
              cont->coefficient(ialpha, icol,
                        cont->coefficient(ialpha, icol) / sqrt(facs));
              break;
            case TGaussianBasisSet::p_shell:
              cont->coefficient(ialpha, icol,
                       cont->coefficient(ialpha, icol) /
                       sqrt(0.5*facs/ee));
              break;
            case TGaussianBasisSet::d_shell:
              cont->coefficient(ialpha, icol,
                 cont->coefficient(ialpha, icol) / sqrt(0.75*facs/(ee*ee)));
              break;
            case TGaussianBasisSet::f_shell:
              cont->coefficient(ialpha, icol,
                 cont->coefficient(ialpha, icol) / sqrt(1.875*facs/pow(ee, 3)));
              break;
            case TGaussianBasisSet::g_shell:
              cont->coefficient(ialpha, icol,
                cont->coefficient(ialpha, icol)/sqrt(6.5625*facs/pow(ee, 4)));
              break;
            case TGaussianBasisSet::h_shell:
             cont->coefficient(ialpha, icol,
              cont->coefficient(ialpha, icol) / sqrt(29.5315*facs/pow(ee, 5)));
              break;
            case TGaussianBasisSet::i_shell:
              cont->coefficient(ialpha, icol,
                    cont->coefficient(ialpha, icol)/
                    sqrt(162.421875*facs/pow(ee, 6)));
              break;
            default:
              EE_RT_ASSERT(0, EE_FATAL, "Unrecognized funcType type");
              break;
          }  // switch
        }    // for ialpha
      }      // for icol

      double dum, snorm;
      // NORMF step; normalize the contracted basis functions
      for (icol = 0; icol < numFuncTypes; icol++) {
        snorm = 0.0;
        for (ialpha = 0; ialpha < numAlpha; ialpha++) {
          for (int ialpha2 = 0; ialpha2 <= ialpha; ialpha2++) {
            double ee = alpha[ialpha] + alpha[ialpha2];
            double fac = ee*sqrt(ee);
            switch (funcTypes[icol]) {
              case TGaussianBasisSet::s_shell:
                dum = cont->coefficient(ialpha, icol)*
                      cont->coefficient(ialpha2, icol)/fac;
                break;
              case TGaussianBasisSet::p_shell:
                dum = cont->coefficient(ialpha, icol)*
                      cont->coefficient(ialpha2, icol)/(2.0*fac*ee);
                break;
              case TGaussianBasisSet::d_shell:
                dum = cont->coefficient(ialpha, icol)*
                      cont->coefficient(ialpha2, icol)*3.0/
                      (4.0*fac*pow(ee, 2));
                break;
              case TGaussianBasisSet::f_shell:
                dum = cont->coefficient(ialpha, icol)*
                      cont->coefficient(ialpha2, icol)*15.0/
                      (8.0*fac*pow(ee, 3));
                break;
              case TGaussianBasisSet::g_shell:
                dum = cont->coefficient(ialpha, icol)*
                      cont->coefficient(ialpha2, icol)*105.0/
                      (16.0*fac*pow(ee, 4));
                break;
              case TGaussianBasisSet::h_shell:
                dum = cont->coefficient(ialpha, icol)*
                      cont->coefficient(ialpha2, icol)*945.0/
                      (32.0*fac*pow(ee, 5));
                break;
              case TGaussianBasisSet::i_shell:
                dum = cont->coefficient(ialpha, icol)*
                      cont->coefficient(ialpha2, icol)*10395.0/
                      (64.0*fac*pow(ee, 6));
                break;
              default:
                EE_RT_ASSERT(0, EE_FATAL, "Unrecognized funcType type");
                break;
            }  // switch
            if (ialpha != ialpha2) dum *= 2.0;
            snorm += dum;
          }
        }
        if (snorm < 1.0e-10)
          snorm = 0.0;
        else
          snorm = 1.0/sqrt(snorm*pi32);

        for (ialpha = 0; ialpha < numAlpha; ialpha++)
           cont->coefficient(ialpha, icol,
          cont->coefficient(ialpha, icol) * snorm);
      }

    }        // for ics

    // add the normalized TGaussianBasisSet back to the list
    normalized.push_back(tmp);
  }          // for gbs


  test.insertGBSGroup(atomID, group);
  return normalized;
}


double getoddNormalize(int shell_type, int index, TGBSAngFunc *angfunc)
{
   int l, m, n;
   if (angfunc->basisType() == TGBSAngFunc::Cartesian) {
      return 1.0;
   } else {
      // Evaluate numerical prefactor for spherical basis functions
      AngMomFunc func;
      switch (shell_type) {
         case TGaussianBasisSet::s_shell:
         case TGaussianBasisSet::p_shell:
            return 1.0;
            break;
         case TGaussianBasisSet::d_shell:
            angfunc->getMaxExponents(shell_type, index, l, m, n);
            if (n == 2) return 1.0;
            return sqrt(3.0);
            break;
         case TGaussianBasisSet::f_shell:
            angfunc->getMaxExponents(shell_type, index, l, m, n);
            func = angfunc->getFunc(shell_type, index);
            if (func.size() == 1) return sqrt(15.0);
            if (func.size() == 2) {
               if (l == 3 || m == 3) return sqrt(2.5);
               return sqrt(15.0);
            }
            if (func.size() == 3) {
               if (l == 3 || m == 3) return sqrt(1.5);
               return 1.0;
            }
            break;
            // This will need to be fixed when h and i are added. - TLW
         case TGaussianBasisSet::g_shell:
         case TGaussianBasisSet::h_shell:
         case TGaussianBasisSet::i_shell:
            return 1.0;
            break;
         default:
            EE_RT_ASSERT(0, EE_FATAL,
                  "Unrecognized funcType type");
            break;
      }
      // should never get here--makes the compiler happy to return a value
      return 1.0;
   }
}


bool flatten(const vector<string>& atomSymbols,
             const vector<double>& atomCoordsAng,
             TGBSConfig *gbsConfig,
             const JCode *code,
             TGBSAngFunc *angfunc,
             int maxShell, const int *length_shell,
             vector<EspBasisFunction>& basis)
{
  basis.clear();
  if (gbsConfig == 0 || angfunc == 0) return false;

  const double atob = 1/0.52917724924;

  const unsigned long numAtoms = atomSymbols.size();
  if (numAtoms == 0 || atomCoordsAng.size() != numAtoms*3) return false;

  for (unsigned long idxAtom = 0; idxAtom < numAtoms; idxAtom++) {

    const string atomID = atomSymbols[idxAtom];
    const unsigned long idxAtomCoord = idxAtom*3;

    double center[3];
    for (int k = 0; k < 3; k++)
      center[k] = atomCoordsAng[idxAtomCoord+k]*atob;

    vector<const TGaussianBasisSet*> gbslist = gbsConfig->getGBSList(atomID);
    vector<TGaussianBasisSet*> normalized =
      normalize(atomID, gbslist, *gbsConfig, code);

    const int gbsSize = normalized.size();

    for (int gbs_index = 0; gbs_index < gbsSize; gbs_index++) {
      const TGaussianBasisSet *gbs = normalized[gbs_index];
      if (gbs == 0) continue;

      const int numContractedSets = gbs->num_contracted_sets(atomID.c_str());

      for (int ics = 0; ics < numContractedSets; ics++) {

        vector<double> alpha = gbs->exponents(atomID.c_str(), ics);
        vector<TGaussianBasisSet::AngularMomentum> funcTypes =
          gbs->func_types(atomID.c_str(), ics);
        Contraction_ *cont = gbs->getContraction(atomID.c_str(), ics);
        if (cont == 0) return false;

        const int numAlpha = alpha.size();
        const int numFuncTypes = funcTypes.size();

        for (int icol = 0; icol < numFuncTypes; icol++) {

          const int shell_type = funcTypes[icol];

          //  Beyond what the angular table describes.  The field
          //  evaluation leaves these orbitals at zero and steps over
          //  their columns; do the same, so the indices still match.
          if (shell_type > maxShell-1) {
            for (int deg = 0; deg < length_shell[shell_type]; deg++) {
              basis.push_back(EspBasisFunction());
            }
            continue;
          }

          for (int deg = 0; deg < length_shell[shell_type]; deg++) {

            EspBasisFunction fn;
            for (int k = 0; k < 3; k++) fn.center[k] = center[k];

            const double oddNormalize =
              getoddNormalize(shell_type, deg, angfunc);

            AngMomFunc terms = angfunc->getFunc(shell_type, deg);
            for (size_t t = 0; t < terms.size(); t++) {
              //  An r^k factor is not a Cartesian Gaussian and cannot go
              //  through the Coulomb integrals as one.  No shipped
              //  MOOrdering uses one; refuse rather than silently drop
              //  the term if that ever changes.
              if (terms[t].m_k != 0) return false;

              fn.powerX.push_back(terms[t].m_l);
              fn.powerY.push_back(terms[t].m_m);
              fn.powerZ.push_back(terms[t].m_n);
              fn.angularCoef.push_back(terms[t].m_coefficient*oddNormalize);
            }

            for (int ia = 0; ia < numAlpha; ia++) {
              fn.exponent.push_back(alpha[ia]);
              fn.contraction.push_back(cont->coefficient(ia, icol));
            }

            basis.push_back(fn);
          }
        }
      }
    }

    //  ComputeMoCmd::buildEspBasis() (which this is moved from) never
    //  freed the per-atom normalize() output either -- an existing
    //  leak, preserved rather than silently fixed here so this stays a
    //  pure move.  See #<followup> if it's ever worth closing.
  }

  return !basis.empty();
}

}  // namespace BasisFlatten
