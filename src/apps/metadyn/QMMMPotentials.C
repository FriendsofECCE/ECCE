#include "util/BrowserHelp.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "tdat/Fragment.H"
#include "tdat/TBond.H"
#include "tdat/QMMMParams.H"
#include "tdat/QMMMModel.H"

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/ResourceType.H"

#include "wxgui/ewxGrid.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/GridModel.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxGridView.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "MetaEd.H"
#include "QMMMPotentials.H"



/** Used to subscribe to messaging, publish, identify */
static const string WHOAMI = ATOMTYPES;


IMPLEMENT_CLASS(QMMMPotentials, wxDialog)


/*!
 * QMMMPotentials constructors
 */
QMMMPotentials::QMMMPotentials(MetaEd *metaed,
                     wxWindow* parent,
                     wxWindowID id,
                     const wxString& caption,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style ):
  QMMMPotentialsGUI(parent, id, caption, pos, size, style)
{
  connectEventHandlers();

  p_metaEd = metaed;

  p_soluteAtomsTable = 0;
  p_selectedTable = 0;

  p_soluteAtomsSizer->SetMinSize(200, 40);
  p_solventAtomsSizer->SetMinSize(200, 40);
  p_solventPseudPotSizer->SetMinSize(200, 40);
  p_bondStretchSizer->SetMinSize(200, 40);
  p_angleBendSizer->SetMinSize(200, 40);

  p_visibleRows = 6;

  createTables();

  initDisplay();

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


QMMMPotentials::~QMMMPotentials( )
{
  if (p_msgDispatcher != 0)
    delete p_msgDispatcher;
}


void QMMMPotentials::createTables() 
{
  p_shakeEvent = false;
  initQMMMLists();
  createSoluteAtomTable();
  createSolventAtomTable();
  createPseudPotTable();
  createBondStretchTable();
  createAngleBendTable();
  if (p_solventNumAtoms == 3) {
    ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->Enable();
  } else {
    ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->Disable();
  }
  bool checked = ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->IsChecked();
  setShakeConfig(checked);
}


void QMMMPotentials::initQMMMLists()
{
  Fragment *frag = p_metaEd->getFrag();
  p_solventNumAtoms = frag->getSolventNumAtoms();
  string QMType;
  vector<QMMMParams*> *shakeModel;
  bool useShake;
  if (p_shakeEvent) {
    if ( ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->IsChecked()) {
      useShake = true;
    } else {
      useShake = false;
    }
  } else {
    if ( ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->IsChecked()) {
      useShake = true;
    } else {
      QMType = "shake";
      shakeModel = frag->getQMMMModel()->getQMMMTerms(QMType);
      if (shakeModel->size() > 0) {
        useShake = true;
      } else {
        useShake = false;
      }
      delete shakeModel;
    }
    if (p_solventNumAtoms != 3) {
      useShake = false;
    }
    ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->SetValue(useShake);
  }
  // Get rid of individual bond and angle parameterizations
  QMType = "bond_set";
  frag->getQMMMModel()->purgeParams(QMType);
  QMType = "angle_set";
  frag->getQMMMModel()->purgeParams(QMType);
  if (!useShake) {
    QMType = "shake";
    frag->getQMMMModel()->purgeParams(QMType);
  }
  int i, size;
  // Clear old data
  vector<string> soluteAtoms;
  vector<string> solventAtoms;
  vector<string> pseudPotAtoms;
  vector<string> bonds1, bonds2;
  vector<string> angles1, angles2, angles3;

  // Create atom type lists from fragment
  set<string> soluteList;
  int firstAtom = frag->getFirstSolventAtomIndex(); 
  TAtm *atm;
  string type;
  for (i=0; i<firstAtom; i++) {
    atm = frag->atomRef(i);
    type = atm->atomType();
    if (soluteList.find(type) == soluteList.end()) {
      soluteAtoms.push_back(type);
      soluteList.insert(type);
    }
  }
  set<string> solventList;
  int numAtoms = frag->numAtoms();
  for (i=firstAtom; i<numAtoms; i++) {
    atm = frag->atomRef(i);
    type = atm->atomType();
    if (solventList.find(type) == solventList.end()) {
      solventAtoms.push_back(type);
      solventList.insert(type);
    }
  }
  set<string> pseudPotList;
  for (i=firstAtom; i<numAtoms; i++) {
    atm = frag->atomRef(i);
    type = atm->atomType();
    if (pseudPotList.find(type) == pseudPotList.end()) {
      pseudPotAtoms.push_back(type);
      pseudPotList.insert(type);
    }
  }
  // Create list of unique bond types from fragment
  set<string> bondList;
  vector<TBond*> *bonds = frag->bonds();
  size = bonds->size();
  TAtm *atm1;
  TAtm *atm2;
  TBond *bond;
  string type1, type2, tmp1, tmp2, btype;
  if (!useShake) {
    for (i=0; i<size; i++) {
      bond = (*bonds)[i];
      atm1 = bond->atom1();
      atm2 = bond->atom2();
      if (atm1->index() >= firstAtom && atm2->index() >= firstAtom) {
        type1 = atm1->atomType();
        type2 = atm2->atomType();
        if (type1<type2) {
          tmp1 = type1;
          tmp2 = type2;
        } else {
          tmp2 = type1;
          tmp1 = type2;
        }
        STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
        STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
        btype = tmp1;
        btype.append("-");
        btype.append(tmp2);
        if (bondList.find(btype) == bondList.end()) {
          if (type1<type2) {
            bonds1.push_back(type1);
            bonds2.push_back(type2);
          } else {
            bonds1.push_back(type2);
            bonds2.push_back(type1);
          }
          bondList.insert(btype);
        }
      }
    }
    delete bonds;
  }

  // Create list of unique angle types from fragment
  set<string> angleList;
  TAtm *atm3;
  string type3, tmp3, atype;
  int j, k, asize;
  if (!useShake) {
    for (i=firstAtom; i<numAtoms; i++) {
      atm1 = frag->atomRef(i);
      const vector<TBond*>& abonds = atm1->bondList();
      asize = abonds.size();
      for (j=0; j < asize; j++) {
        if (abonds[j]->atom1() == atm1) {
          atm2 = abonds[j]->atom2();
        } else {
          atm2 = abonds[j]->atom1();
        }
        for (k=j+1; k < asize; k++) {
          if (abonds[k]->atom1() == atm1) {
            atm3 = abonds[k]->atom2();
          } else {
            atm3 = abonds[k]->atom1();
          }
          if (atm1->index() >= firstAtom && atm2->index() >= firstAtom &&
              atm3->index() >= firstAtom) {
            type1 = atm1->atomType();
            type2 = atm2->atomType();
            type3 = atm3->atomType();
            tmp1 = type1;
            if (type2<type3) {
              tmp2 = type2;
              tmp3 = type3;
            } else {
              tmp2 = type3;
              tmp3 = type2;
            }
            STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
            STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
            STLUtil::stripLeadingAndTrailingWhiteSpace(tmp3);
            btype = tmp2;
            btype.append("-");
            btype.append(tmp1);
            btype.append("-");
            btype.append(tmp3);
            if (angleList.find(btype) == angleList.end()) {
              if (type2<type3) {
                angles1.push_back(type1);
                angles2.push_back(type2);
                angles3.push_back(type3);
              } else {
                angles1.push_back(type1);
                angles2.push_back(type3);
                angles3.push_back(type2);
              }
              angleList.insert(btype);
            }
          }
        }
      }
    }
  }

  // Compare lists derived from fragment with current model. If term is on
  // fragment but does not exist in model, then add it to the model. If term is
  // in model but not in fragment, then delete if from model.
  set<string> termList;
  QMMMParams *term;
  vector<string> *types;
  vector<string> atypes;
  vector<double> rpar;
  // Check solute atoms
  QMType = "su_vdw";
  vector<QMMMParams*> *soluteModel = frag->getQMMMModel()->getQMMMTerms(QMType);
  size = soluteModel->size();
  // Check that terms in the model exist on the fragment
  for (i=0; i<size; i++) {
    term = (*soluteModel)[i];
    types = term->getQMMMPotentials();
    if (soluteList.find((*types)[0]) == soluteList.end()) {
      frag->getQMMMModel()->remove(term);
    } else {
      termList.insert((*types)[0]);
    }
    delete types;
  }
  // Check that all atoms in fragment are represented in model
  size = soluteAtoms.size();
  for (i=0; i<size; i++) {
    if (termList.find(soluteAtoms[i]) == termList.end()) {
      atypes.clear();
      atypes.push_back(soluteAtoms[i]);
      rpar.clear();
      rpar.push_back(0.0);
      rpar.push_back(0.0);
      term = new QMMMParams(QMType,1,&atypes,2,&rpar);
      frag->getQMMMModel()->addQMMMTerm(term);
    }
  }
  delete soluteModel;
  termList.clear();

  // Check solvent atoms
  QMType = "sv_vdw";
  vector<QMMMParams*> *solventModel = frag->getQMMMModel()->getQMMMTerms(QMType);
  size = solventModel->size();
  // Check that terms in the model exist on the fragment
  for (i=0; i<size; i++) {
    term = (*solventModel)[i];
    types = term->getQMMMPotentials();
    if (solventList.find((*types)[0]) == solventList.end()) {
      frag->getQMMMModel()->remove(term);
    } else {
      termList.insert((*types)[0]);
    }
    delete types;
  }
  // Check that all atoms in fragment are represented in model
  size = solventAtoms.size();
  for (i=0; i<size; i++) {
    if (termList.find(solventAtoms[i]) == termList.end()) {
      atypes.clear();
      atypes.push_back(solventAtoms[i]);
      rpar.clear();
      rpar.push_back(0.0);
      rpar.push_back(0.0);
      term = new QMMMParams(QMType,1,&atypes,2,&rpar);
      frag->getQMMMModel()->addQMMMTerm(term);
    }
  }
  delete solventModel;
  termList.clear();

  // Check pseudo-potential atoms
  QMType = "pseud_pot";
  vector<QMMMParams*> *pseudPotModel = frag->getQMMMModel()->getQMMMTerms(QMType);
  size = pseudPotModel->size();
  // Check that terms in the model exist on the fragment
  for (i=0; i<size; i++) {
    term = (*pseudPotModel)[i];
    types = term->getQMMMPotentials();
    if (pseudPotList.find((*types)[0]) == pseudPotList.end()) {
      frag->getQMMMModel()->remove(term);
    } else {
      termList.insert((*types)[0]);
    }
    delete types;
  }
  // Check that all atoms in fragment are represented in model
  size = pseudPotAtoms.size();
  for (i=0; i<size; i++) {
    if (termList.find(pseudPotAtoms[i]) == termList.end()) {
      atypes.clear();
      atypes.push_back(pseudPotAtoms[i]);
      rpar.clear();
      rpar.push_back(0.0);
      rpar.push_back(0.0);
      rpar.push_back(0.0);
      term = new QMMMParams(QMType,1,&atypes,3,&rpar);
      frag->getQMMMModel()->addQMMMTerm(term);
    }
  }
  delete pseudPotModel;
  termList.clear();

  // Check bonds
  QMType = "bond";
  vector<QMMMParams*> *bondModel = frag->getQMMMModel()->getQMMMTerms(QMType);
  size = bondModel->size();
  // Check that terms in the model exist on the fragment
  if (!useShake) {
    for (i=0; i<size; i++) {
      term = (*bondModel)[i];
      types = term->getQMMMPotentials();
      type1 = (*types)[0];
      type2 = (*types)[1];
      if (type1<type2) {
        tmp1 = type1;
        tmp2 = type2;
      } else {
        tmp2 = type1;
        tmp1 = type2;
      }
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
      btype = tmp1;
      btype.append("-");
      btype.append(tmp2);
      if (bondList.find(btype) == bondList.end()) {
        frag->getQMMMModel()->remove(term);
      } else {
        termList.insert(btype);
      }
      delete types;
    }
    // Check that all bonds in fragment are represented in model
    size = bonds1.size();
    for (i=0; i<size; i++) {
      type1 = bonds1[i];
      type2 = bonds2[i];
      if (type1<type2) {
        tmp1 = type1;
        tmp2 = type2;
      } else {
        tmp2 = type1;
        tmp1 = type2;
      }
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
      btype = tmp1;
      btype.append("-");
      btype.append(tmp2);
      if (termList.find(btype) == termList.end()) {
        atypes.clear();
        atypes.push_back(bonds1[i]);
        atypes.push_back(bonds2[i]);
        rpar.clear();
        rpar.push_back(0.0);
        rpar.push_back(0.0);
        term = new QMMMParams(QMType,2,&atypes,2,&rpar);
        frag->getQMMMModel()->addQMMMTerm(term);
      }
    }
    delete bondModel;
    termList.clear();
  }

  // Check angles
  QMType = "angle";
  vector<QMMMParams*> *angleModel = frag->getQMMMModel()->getQMMMTerms(QMType);
  size = angleModel->size();
  // Check that terms in the model exist on the fragment
  if (!useShake) {
    for (i=0; i<size; i++) {
      term = (*angleModel)[i];
      types = term->getQMMMPotentials();
      type1 = (*types)[1];
      type2 = (*types)[0];
      type3 = (*types)[2];
      tmp1 = type1;
      if (type2<type3) {
        tmp2 = type2;
        tmp3 = type3;
      } else {
        tmp2 = type3;
        tmp3 = type2;
      }
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp3);
      btype = tmp2;
      btype.append("-");
      btype.append(tmp1);
      btype.append("-");
      btype.append(tmp3);
      if (angleList.find(btype) == angleList.end()) {
        frag->getQMMMModel()->remove(term);
      } else {
        termList.insert(btype);
      }
      delete types;
    }
    // Check that all angles in fragment are represented in model
    size = angles1.size();
    for (i=0; i<size; i++) {
      type1 = angles1[i];
      type2 = angles2[i];
      type3 = angles3[i];
      if (type2<type3) {
        tmp2 = type2;
        tmp3 = type3;
      } else {
        tmp2 = type3;
        tmp3 = type2;
      }
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp3);
      btype = tmp2;
      btype.append("-");
      btype.append(tmp1);
      btype.append("-");
      btype.append(tmp3);
      if (termList.find(btype) == termList.end()) {
        atypes.clear();
        atypes.push_back(angles2[i]);
        atypes.push_back(angles1[i]);
        atypes.push_back(angles3[i]);
        rpar.clear();
        rpar.push_back(0.0);
        rpar.push_back(0.0);
        term = new QMMMParams(QMType,3,&atypes,2,&rpar);
        frag->getQMMMModel()->addQMMMTerm(term);
      }
    }
    delete bondModel;
    termList.clear();
  }

  // Construct a hash table of unique bonds and angles and their locations in
  // the QMMMModel model
  int numTerms = frag->getQMMMModel()->numQMMMTerms();
  QMMMParams *params;
  map<string, int> hashtags;
  map<string, int>::iterator mit;
  if (!useShake) {
    for (i=0; i<numTerms; i++) {
      params = frag->getQMMMModel()->getQMMMTerm(i);
      if (params->getQMMMType() == "bond") {
        types = params->getQMMMPotentials();
        type1 = (*types)[0];
        STLUtil::stripLeadingAndTrailingWhiteSpace(type1);
        type2 = (*types)[1];
        STLUtil::stripLeadingAndTrailingWhiteSpace(type2);
        btype = type1;
        btype.append("-");
        btype.append(type2);
        delete types;
        hashtags.insert(pair<string, int>(btype,i));
      }
      if (params->getQMMMType() == "angle") {
        types = params->getQMMMPotentials();
        type1 = (*types)[1];
        STLUtil::stripLeadingAndTrailingWhiteSpace(type1);
        type2 = (*types)[0];
        STLUtil::stripLeadingAndTrailingWhiteSpace(type2);
        type3 = (*types)[2];
        STLUtil::stripLeadingAndTrailingWhiteSpace(type3);
        btype = type2;
        btype.append("-");
        btype.append(type1);
        btype.append("-");
        btype.append(type3);
        delete types;
        hashtags.insert(pair<string, int>(btype,i));
      }
    }
  }

  // Now that we have identified all atoms, bonds, and angles appearing in the
  // system, we need to construct a complete list of individual bonds and angles
  // appearing on a single solvent molecule. Start by constructing a fragment
  // consisting of a single solvent molecule.

  Fragment tmpfrag;
  j = 0;
  for (i=firstAtom; i<firstAtom+p_solventNumAtoms; i++) {
    atm1 = frag->atomRef(i);
    const double *tcoord = atm1->coordinates();
    string symbol = atm1->atomicSymbol();
    type1 = atm1->atomType();
    tmpfrag.addAtom(symbol, tcoord);
    atm2 = tmpfrag.atomRef(j);
    atm2->atomType(type1.c_str());
    j++;
  }
  tmpfrag.deleteAllBonds();
  tmpfrag.addCovalentBonds();

  // If using SHAKE constraints, then add or modify existing SHAKE parameters to
  // QMMM model
  if (useShake) {
    QMType = "shake";
    frag->getQMMMModel()->purgeParams(QMType);
    if (tmpfrag.numAtoms() == 3) {
      atm1 = tmpfrag.atomRef(0);
      atm2 = tmpfrag.atomRef(1);
      atm3 = tmpfrag.atomRef(2);
      const double* crd1 = atm1->coordinates();
      const double* crd2 = atm2->coordinates();
      const double* crd3 = atm3->coordinates();
      double r12 = sqrt((crd1[0]-crd2[0])*(crd1[0]-crd2[0])
                 +      (crd1[1]-crd2[1])*(crd1[1]-crd2[1])
                 +      (crd1[2]-crd2[2])*(crd1[2]-crd2[2]));
      double r23 = sqrt((crd2[0]-crd3[0])*(crd2[0]-crd3[0])
                 +      (crd2[1]-crd3[1])*(crd2[1]-crd3[1])
                 +      (crd2[2]-crd3[2])*(crd2[2]-crd3[2]));
      double r31 = sqrt((crd3[0]-crd1[0])*(crd3[0]-crd1[0])
                 +      (crd3[1]-crd1[1])*(crd3[1]-crd1[1])
                 +      (crd3[2]-crd1[2])*(crd3[2]-crd1[2]));
      vector<int> shkidx;
      vector<double> shkpar;
      shkidx.push_back(0);
      shkidx.push_back(1);
      shkidx.push_back(2);
      shkpar.push_back(r12);
      shkpar.push_back(r23);
      shkpar.push_back(r31);
      term = new QMMMParams(QMType,3,&shkidx,3,&shkpar);
      frag->getQMMMModel()->addQMMMTerm(term);
    } else {
      //TODO: Some kind of error
    }
  }

  // Now construct list of all bonds and angles in tmpfrag and get their associated
  // parameters
  if (!useShake) {
    bonds = tmpfrag.bonds();
    size = bonds->size();
    int idx, idx1, idx2, idx3;
    for (i=0; i<size; i++) {
      bond = (*bonds)[i];
      atm1 = bond->atom1();
      atm2 = bond->atom2();
      type1 = atm1->atomType();
      type2 = atm2->atomType();
      if (type1<type2) {
        tmp1 = type1;
        tmp2 = type2;
        idx1 = atm1->index();
        idx2 = atm2->index();
      } else {
        tmp2 = type1;
        tmp1 = type2;
        idx2 = atm1->index();
        idx1 = atm2->index();
      }
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
      STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
      btype = tmp1;
      btype.append("-");
      btype.append(tmp2);
      // find location of parameters from hashtags
      mit = hashtags.find(btype);
      if (mit != hashtags.end()) {
        idx = mit->second;
        vector<int> *indices = new vector<int>;
        indices->push_back(idx1);
        indices->push_back(idx2);
        params = frag->getQMMMModel()->getQMMMTerm(idx);
        term = new QMMMParams("bond_set",2,indices,params);
        frag->getQMMMModel()->addQMMMTerm(term);
        delete indices;
      } else {
        // TODO: some kind of error
      }
    }
    delete bonds;

    // Find all angles in tmpfrag
    numAtoms = tmpfrag.numAtoms();
    for (i=0; i<numAtoms; i++) {
      atm1 = tmpfrag.atomRef(i);
      const vector<TBond*>& abonds = atm1->bondList();
      asize = abonds.size();
      for (j=0; j < asize; j++) {
        if (abonds[j]->atom1() == atm1) {
          atm2 = abonds[j]->atom2();
        } else {
          atm2 = abonds[j]->atom1();
        }
        for (k=j+1; k < asize; k++) {
          if (abonds[k]->atom1() == atm1) {
            atm3 = abonds[k]->atom2();
          } else {
            atm3 = abonds[k]->atom1();
          }
          type1 = atm1->atomType();
          type2 = atm2->atomType();
          type3 = atm3->atomType();
          tmp1 = type1;
          idx1 = atm1->index();
          if (type2<type3) {
            tmp2 = type2;
            tmp3 = type3;
            idx2 = atm2->index();
            idx3 = atm3->index();
          } else {
            tmp2 = type3;
            tmp3 = type2;
            idx2 = atm3->index();
            idx3 = atm2->index();
          }
          STLUtil::stripLeadingAndTrailingWhiteSpace(tmp1);
          STLUtil::stripLeadingAndTrailingWhiteSpace(tmp2);
          STLUtil::stripLeadingAndTrailingWhiteSpace(tmp3);
          btype = tmp2;
          btype.append("-");
          btype.append(tmp1);
          btype.append("-");
          btype.append(tmp3);
          // find location of parameters from hashtags
          mit = hashtags.find(btype);
          if (mit != hashtags.end()) {
            idx = mit->second;
            vector<int> *indices = new vector<int>;
            indices->push_back(idx2);
            indices->push_back(idx1);
            indices->push_back(idx3);
            params = frag->getQMMMModel()->getQMMMTerm(idx);
            term = new QMMMParams("angle_set",3,indices,params);
            frag->getQMMMModel()->addQMMMTerm(term);
            delete indices;
          } else {
            // TODO: some kind of error
          }
        }
      }
    }
  }
}

void QMMMPotentials::clearTables()
{
  // Clear model and redisplay tables
  QMMMModel *model = getModel();
  if (model && model->size() > 0) {
    model->clear();
    notify();
    p_shakeEvent = false;
    initQMMMLists();
  }
  initDisplay();
}


void QMMMPotentials::hideTable(WxGridView *table, wxSizer *container)
{
  if (table->IsShown()) {

    GridModel *gridModel = table->getModel();  

    // Clear Table
    gridModel->DeleteRows(0, gridModel->GetNumberRows());

    // Clear container
    table->Show(false);
    container->Detach(table);
    container->Clear();

    ewxStaticText *label = new ewxStaticText(this, wxID_STATIC, " ");
    container->Add(label, 0, wxALIGN_CENTER_VERTICAL);

  }

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void QMMMPotentials::initQMMMTypesTable(WxGridView *table)
{
  GridModel *model = table->getModel();
  ewxGrid *grid = dynamic_cast<ewxGrid*>(table->getGrid());
  int cols = model->GetNumberCols(); 
  int rows = model->GetNumberRows();

  if ((cols > 0) && (rows > 0)) {
    for (int i = 0; i < rows; i++) {
      if ((i % 2) == 0) {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        rowAttr->SetBackgroundColour(grid->getHightLightRowColor());
        grid->SetRowAttr(i, rowAttr);
      }
      else {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        grid->SetRowAttr(i, rowAttr);
      }
      if (i == 0) {
        for (int j = 0; j < cols; j++) {
          GridElement *colElement = model->GetValueAsGridElement(i, j);
          if (colElement != 0) {
            wxGridCellAttr *colAttr = new wxGridCellAttr();
            switch (colElement->getPropertyType()) {
              case GridProperty::PT_CDATA:
                colAttr->SetReadOnly();
                colAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
                break;
              case GridProperty::PT_FLOAT:
                colAttr->SetRenderer(new wxGridCellStringRenderer);
                colAttr->SetEditor(new wxGridCellFloatEditor);
                colAttr->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
                break;              
              default:
                colAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
                colAttr->SetReadOnly();
                break;
            }
            grid->SetColAttr(j, colAttr);
          }
        }
      }
    }
  }

  table->enableGridLines(true);
  table->getGrid()->ClearSelection();
  table->enableLabelClickProcessing(false);

  //setTableSize(table);
}


void QMMMPotentials::setTableSize(WxGridView *table) 
{
  if (table->getRowCount() > 0) {
    int rows = (p_visibleRows <= table->getRowCount()) ? 
                p_visibleRows :
                table->getRowCount();
    int height = 0;
    int labelHeight = table->getColLabelHeight();
    int rowHeight = table->getGrid()->GetGridWindow()->GetCharHeight();

    height = labelHeight + (rowHeight * rows);

    // Set the window's minimum size
    // so sizer will be correct
    wxSize winSize = table->GetSize();
    winSize.SetHeight(height + 50);
    table->SetMinSize(winSize);
    table->SetSize(winSize);
    table->SetMaxSize(winSize);
  }

}


void QMMMPotentials::createSoluteAtomTable()
{
  // Solute atom terms
  p_soluteAtomsTable = new WxGridView(this, wxID_ANY, ATOMTYPES);
  p_soluteAtomsSizer->Add(p_soluteAtomsTable, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 3);
  p_soluteAtomsTable->setModel(gridModel);
  p_soluteAtomsTable->setColLabel(0, "Atom Type");
  p_soluteAtomsTable->setColLabel(1, "Sigma");
  p_soluteAtomsTable->setColLabel(2, "Epsilon");
  initQMMMTypesTable(p_soluteAtomsTable);
}


void QMMMPotentials::createSolventAtomTable()
{
  // Solvent atom terms
  p_solventAtomsTable = new WxGridView(this, wxID_ANY, ATOMTYPES);
  p_solventAtomsSizer->Add(p_solventAtomsTable, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 3);
  p_solventAtomsTable->setModel(gridModel);
  p_solventAtomsTable->setColLabel(0, "Atom Type");
  p_solventAtomsTable->setColLabel(1, "Sigma");
  p_solventAtomsTable->setColLabel(2, "Epsilon");
  initQMMMTypesTable(p_solventAtomsTable);
}


void QMMMPotentials::createBondStretchTable()
{
  // bond stretch terms
  p_bondStretchTable = new WxGridView(this, wxID_ANY, ATOMTYPES);
  p_bondStretchSizer->Add(p_bondStretchTable, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 4);
  p_bondStretchTable->setModel(gridModel);
  p_bondStretchTable->setColLabel(0, "Atom1 Type");
  p_bondStretchTable->setColLabel(1, "Atom2 Type");
  p_bondStretchTable->setColLabel(2, "Kappa");
  p_bondStretchTable->setColLabel(3, "R_eq");
  initQMMMTypesTable(p_bondStretchTable);
}


void QMMMPotentials::createAngleBendTable()
{
  // angle bend terms
  p_angleBendTable = new WxGridView(this, wxID_ANY, ATOMTYPES);
  p_angleBendSizer->Add(p_angleBendTable, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 5);
  p_angleBendTable->setModel(gridModel);
  p_angleBendTable->setColLabel(0, "Atom1 Type");
  p_angleBendTable->setColLabel(1, "Atom2 Type");
  p_angleBendTable->setColLabel(2, "Atom3 Type");
  p_angleBendTable->setColLabel(3, "Kappa");
  p_angleBendTable->setColLabel(4, "Theta_eq");
  initQMMMTypesTable(p_angleBendTable);
}


void QMMMPotentials::createPseudPotTable()
{
  // angle bend terms
  p_solventPseudPotTable = new WxGridView(this, wxID_ANY, ATOMTYPES);
  p_solventPseudPotSizer->Add(p_solventPseudPotTable, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 4);
  p_solventPseudPotTable->setModel(gridModel);
  p_solventPseudPotTable->setColLabel(0, "Atom Type");
  p_solventPseudPotTable->setColLabel(1, "Z(ion)");
  p_solventPseudPotTable->setColLabel(2, "n_sigma");
  p_solventPseudPotTable->setColLabel(3, "R_c");
  initQMMMTypesTable(p_solventPseudPotTable);
}


/*
 * Redisplay all tables
 */
void QMMMPotentials::initDisplay(bool isEnabled)
{
  // Get model
  QMMMModel *model = getModel();
  bool checked = ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->IsChecked();

  // Display tables
  if (model != 0) {
    displaySoluteAtoms(model);
    displaySolventAtoms(model);
    displayPseudPotAtoms(model);
    if (!checked) {
      displayBonds(model);
      displayAngles(model);
    }
  }
  else {
    hideTable(p_soluteAtomsTable, p_soluteAtomsSizer);
    hideTable(p_solventAtomsTable, p_solventAtomsSizer);
    hideTable(p_solventPseudPotTable, p_solventPseudPotSizer);
    hideTable(p_bondStretchTable, p_bondStretchSizer);
    hideTable(p_angleBendTable, p_angleBendSizer);
  }
    

  // Enable/Disable buttons
  if (isEnabled) {
    FindWindow(ID_ATMT_CLEAR_ALL_BUTTON)->Enable();

    p_soluteAtomsTable->Enable();
    p_solventAtomsTable->Enable();
    p_solventPseudPotTable->Enable();
    p_bondStretchTable->Enable();
    p_angleBendTable->Enable();
  }
  else {
    FindWindow(ID_ATMT_CLEAR_ALL_BUTTON)->Disable();

    p_soluteAtomsTable->Disable();
    p_solventAtomsTable->Disable();
    p_solventPseudPotTable->Disable();
    p_bondStretchTable->Disable();
    p_angleBendTable->Disable();
  }

  // Fix layout
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void QMMMPotentials::notify() 
{
  p_metaEd->enableSave();
}


QMMMModel *QMMMPotentials::getModel()
{
  QMMMModel *ret = 0;
  Fragment *frag = p_metaEd->getFrag();

  if (frag) {
     ret = frag->getQMMMModel();
  }

  return ret;
}


void QMMMPotentials::displaySoluteAtoms(QMMMModel *model)
{
  string type = "su_vdw";
  vector<QMMMParams*> *terms = model->getQMMMTerms(type);
  displayQMMMTerms(p_soluteAtomsSizer, 
                   p_soluteAtomsTable,
                   *terms);
  delete terms;
}

void QMMMPotentials::displaySolventAtoms(QMMMModel *model)
{
  string type = "sv_vdw";
  vector<QMMMParams*> *terms = model->getQMMMTerms(type);
  displayQMMMTerms(p_solventAtomsSizer, 
                   p_solventAtomsTable,
                   *terms);
  delete terms;
}


void QMMMPotentials::displayBonds(QMMMModel *model)
{
  string type = "bond";
  vector<QMMMParams*> *terms = model->getQMMMTerms(type);
  displayQMMMTerms(p_bondStretchSizer, 
                   p_bondStretchTable,
                   *terms);
  delete terms;
}


void QMMMPotentials::displayAngles(QMMMModel *model)
{
  string type = "angle";
  vector<QMMMParams*> *terms = model->getQMMMTerms(type);
  displayQMMMTerms(p_angleBendSizer, 
                   p_angleBendTable,
                   *terms);
  delete terms;
}


void QMMMPotentials::displayPseudPotAtoms(QMMMModel *model)
{
  string type = "pseud_pot";
  vector<QMMMParams*> *terms = model->getQMMMTerms(type);
  displayQMMMTerms(p_solventPseudPotSizer, 
                   p_solventPseudPotTable,
                   *terms);
  delete terms;
}


void QMMMPotentials::displayQMMMTerms(wxSizer *container,
                                 WxGridView *table,
                                 vector<QMMMParams*> &terms)
{
  char buf[32];
  GridModel *gridModel = table->getModel();  

  // Clear table
  gridModel->DeleteRows(0, gridModel->GetNumberRows());

  // Clear container
  table->Show(false);
  container->Detach(table);
  container->Clear(true);


  vector<string> *types;
  vector<double> *params;
  int idx, jdx;
  if (terms.size() > 0) {

    for (idx=0; idx<terms.size(); idx++) {

      gridModel->InsertRows(idx);

      // Fill in row with properties of atom type
      types = terms[idx]->getQMMMPotentials();
      for (jdx=0; jdx < types->size(); jdx++) {
        sprintf(buf, "%s", (*types)[jdx].c_str());
        GridElement *atomCell = new GridElement(buf, new GridProperty(GridProperty::PT_CDATA));
        gridModel->SetValueAsGridElement(idx, jdx, atomCell);
      }

      params = terms[idx]->getParams();
      for (jdx=0; jdx < params->size(); jdx++) {
        sprintf(buf, "%lf", (*params)[jdx]);
        GridElement *valCell = new GridElement(buf, new GridProperty(GridProperty::PT_FLOAT));
        gridModel->SetValueAsGridElement(idx, types->size()+jdx, valCell);
      }
      delete types;
      delete params;
    }
    initQMMMTypesTable(table);
    container->Add(table);
    //container->Add(table, 0, wxEXPAND, 0);
    table->Show(true);
  }
  else {
    ewxStaticText *label = new ewxStaticText(this, wxID_STATIC, " ");
    container->Add(label, 0, wxALIGN_CENTER_VERTICAL);
  }

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void QMMMPotentials::requestBuilder(const string& actionTopic) 
{
  string url = p_metaEd->getContext();

  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("action","start");
  publisher.publish("ecce_activity",*msg);
  delete msg;

  // Send the start app request to gateway:
  Target gateway(GATEWAY, "");
  msg = publisher.newMessage(gateway);
  // Ensure that this message will carry original sender info for
  // sending point to point responses after startup
  msg->markAsPointToPoint();


  // which builder do we invoke?  find out now!
  Resource *resource = EDSIFactory::getResource(url);
  if (resource) {
    ResourceType *resourceType = resource->getDescriptor();
    if (resourceType) {
      vector<ResourceTool*> tools = resourceType->getTools();
      for (int i = 0; i < tools.size(); i++) {
        string name = tools[i]->getName();
        if (name == BUILDER) {
          publisher.invoke(*msg, name, 0, url, actionTopic);
          break;
        }
      }
    }
  }

  delete msg;
}



/**
 * wxEVT_CLOSE_WINDOW event handler
 */
void QMMMPotentials::OnCloseWindow( wxCloseEvent& event )
{
  this->Show(false);
}

vector<QMMMParams*> *QMMMPotentials::getSelectedTableConstraints()
{
  vector<QMMMParams*> *ret = 0;
  QMMMModel *model = getModel();

  if (p_soluteAtomsTable->GetId() == p_selectedTable->GetId()) {
    ret = model->getQMMMTerms("su_vdw");
  }
  if (p_solventAtomsTable->GetId() == p_selectedTable->GetId()) {
    ret = model->getQMMMTerms("sv_vdw");
  }
  if (p_solventPseudPotTable->GetId() == p_selectedTable->GetId()) {
    ret = model->getQMMMTerms("pseud_pot");
  }
  if (p_bondStretchTable->GetId() == p_selectedTable->GetId()) {
    ret = model->getQMMMTerms("bond");
  }
  if (p_angleBendTable->GetId() == p_selectedTable->GetId()) {
    ret = model->getQMMMTerms("angle");
  }

  return ret;
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATMT_CLEAR_ALL_BUTTON
 */
void QMMMPotentials::OnClearAllButtonClick( wxCommandEvent& event )
{
  clearTables();
  p_selectedTable = 0;
  event.Skip();
}

void QMMMPotentials::setShakeConfig( bool useShake )
{
  if (!useShake) {
    // Get model
    QMMMModel *model = getModel();
    // Display tables
    if (model != 0) {
      displayBonds(model);
      displayAngles(model);
    }
  } else {
    hideTable(p_bondStretchTable, p_bondStretchSizer);
    hideTable(p_angleBendTable, p_angleBendSizer);
  }
}

void QMMMPotentials::OnAtmtUseShakeCheckboxClick( wxCommandEvent& event )
{
  bool checked = ((ewxCheckBox*)FindWindow(ID_ATMT_USE_SHAKE_CHECKBOX))->IsChecked();
  p_shakeEvent = true;
  initQMMMLists();
  setShakeConfig(checked);
  notify();
  event.Skip();
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATMT_CLOSE_BUTTON
 */
void QMMMPotentials::OnCloseButtonClick( wxCommandEvent& event )
{
  wxCloseEvent evt;
  OnCloseWindow(evt);
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATMT_CLOSE_BUTTON
 */
void QMMMPotentials::OnHelpButtonClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showPage(help.URL("HomeFallback"));
}


void QMMMPotentials::connectEventHandlers()
{
    // Left click on a table cell.
    this->Connect(wxEVT_GRID_CELL_LEFT_CLICK,
                  wxGridEventHandler(QMMMPotentials::gridCellLeftClick));

    // Value changed
    this->Connect(wxEVT_GRID_CELL_CHANGE,
                  wxGridEventHandler(QMMMPotentials::gridCellChange));
}


void QMMMPotentials::gridCellLeftClick(wxGridEvent& event)
{
  ewxGrid *table = (ewxGrid *)(event.GetEventObject());
  p_selectedTable = (WxGridView*)table->GetParent();

  if (p_soluteAtomsTable->GetId() != p_selectedTable->GetId()) {
    p_soluteAtomsTable->getGrid()->ClearSelection();
  }
  if (p_solventAtomsTable->GetId() != p_selectedTable->GetId()) {
    p_solventAtomsTable->getGrid()->ClearSelection();
  }
  if (p_solventPseudPotTable->GetId() != p_selectedTable->GetId()) {
    p_solventPseudPotTable->getGrid()->ClearSelection();
  }
  if (p_bondStretchTable->GetId() != p_selectedTable->GetId()) {
    p_bondStretchTable->getGrid()->ClearSelection();
  }
  if (p_angleBendTable->GetId() != p_selectedTable->GetId()) {
    p_angleBendTable->getGrid()->ClearSelection();
  }
  event.Skip();
}


void QMMMPotentials::gridCellChange(wxGridEvent& event)
{
  WxGridView *table = (WxGridView*)(((wxWindow*)(event.GetEventObject()))->GetParent());

  if (p_selectedTable != 0) {
    if (table->GetId() == p_selectedTable->GetId()) {
      vector<QMMMParams*> *params = getSelectedTableConstraints();
      if (params != 0) {
        GridElement *element =
          dynamic_cast<GridElement*>(table->getElement(event.GetRow(), event.GetCol()));
        if (element != 0) {
          double val;
          StringConverter::toDouble(element->getValue().c_str(),val);
          QMMMParams *param = (*params)[event.GetRow()];
          int natoms = param->getNumAtoms();
          param->setParamValue(val, event.GetCol()-natoms);
          notify();
        }
        delete params;
      }
    }
  }
}



