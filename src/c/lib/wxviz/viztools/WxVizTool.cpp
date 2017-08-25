#include <wx/fileconf.h>

#include "util/NullPointerException.H"
#include "util/Event.H"
#include "util/EventDispatcher.H"
using namespace ecce;

#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/Residue.H"

#include "inv/ChemKit/ChemDisplayPath.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxviz/SGSelection.H"
#include "wxviz/WxVizToolFW.H"
#include "wxviz/WxVizTool.H"


WxVizTool::WxVizTool()
{
   p_vizfw = 0;
}

WxVizTool::~WxVizTool()
{
   /* Don't delete the connector! */
   p_vizfw = 0;
}

WxVizToolFW& WxVizTool::getFW() const
{
   NULLPOINTEREXCEPTION(p_vizfw,"Viz framework not set.");
   return *p_vizfw;
}


void WxVizTool::connectToolKitFW(WxVizToolFW * fw)
{
   p_vizfw = fw;
}

/**
 * Nothing to do in the generic case.
 */
void WxVizTool::restoreSettings(wxConfig * config)
{
  
}

/**
 * Nothing to do in the generic case.
 */
void WxVizTool::saveSettings(wxConfig * config)
{
  
}



/**
 * Sets the fragment atom and highlight bonds, invokes the select command
 * and notifies that the selection has changed.
 */
void 
WxVizTool::selectCommand(const vector<int>& atoms, const vector<int>& bonds)
{
   SGContainer& sg = getFW().getSceneGraph();
   SGFragment *frag = sg.getFragment();
   if (frag) {
      frag->m_atomHighLight = atoms;
      frag->m_bondHighLight = bonds;

      Event event("SelectionChanged","");
      EventDispatcher::getDispatcher().publish(event);

   }
}

/**
 * Sets the fragment atom and highlight bonds, invokes the select command
 * and notifies that the selection has changed.
 */
void 
WxVizTool::selectCommand(const vector<bool>& atoms, const vector<bool>& bonds)
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   if (frag) {

      int idx;

      frag->m_atomHighLight.clear();
      int numAtoms = atoms.size();
      for (idx=0; idx<numAtoms; idx++) {
         if (atoms[idx]) frag->m_atomHighLight.push_back(idx);
      }

      frag->m_bondHighLight.clear();
      int numBonds = bonds.size();
      for (idx=0; idx<numBonds; idx++) {
         if (bonds[idx]) frag->m_bondHighLight.push_back(idx);
      }
      Event event("SelectionChanged","");
      EventDispatcher::getDispatcher().publish(event);

   }
}



/**
 *   This method takes a set of atoms (and bonds) and sets the inventor
 *   selection node to those atoms.  This version works off atom indices.
 *   See also an overloaded function that works off object pointers.
 *
 *   supports multiple ChemDisplay objects
 *
 *   This is one of those methods that needs to be efficient so if you
 *   make mods be sure to test it with a large system (20k+ atoms).
 *
 *   Last revised for efficiency 1/31/2001 KLS
 *
 * NOTE if its not working try clearing selection first.
 */
void 
WxVizTool::setSelection(const vector<int>& atoms, const vector<int>& bonds)
{
   // Important performance note: don't call the fragment bonds() method
   // since for large systems this can be slow.  Instead assume that
   // SGFragment::touchNumbers has been called to generate a cached
   // bondIndices instance that can be used for fast access

   int j;
   int numSelAtoms ;
   int numSelBonds ;
   SGSelection *esel = getFW().getViewer().getSel();
   esel->deselectAll();
   MFVec2i *lAtoms = new MFVec2i;
   MFVec2i *lBonds = new MFVec2i;
   SGContainer& sg = getFW().getSceneGraph();
   SGFragment *frag = sg.getFragment();

   vector<string> names = sg.getDisplayStyleNames();
   int numDisplayStyles = names.size();
   for (int ds = 0; ds < numDisplayStyles; ds++)
   {

     SoPath * pathCD =  sg.getSelectionPath(esel, ds);
      if (pathCD == NULL) { continue; }
      string name = names[ds];

      numSelAtoms = 0 ;
      numSelBonds = 0 ;


      int nAtoms = atoms.size();
      for (j=0 ; j < nAtoms ; j++) {
         if (frag->atomRef(atoms[j])->displayStyle().getName() == name) {
            lAtoms->set1Value(numSelAtoms,SbVec2i(atoms[j],1));
            numSelAtoms++;
         }
      }
      lAtoms->setNum(numSelAtoms);

      int nBonds = bonds.size();
      for (j=0; j< nBonds; j++) {
         if (frag->getBondStyle(bonds[j]).getName() == name)
         {
            lBonds->set1Value(numSelBonds,SbVec2i(bonds[j],1));
            numSelBonds++;
         }
      }
      lBonds->setNum(numSelBonds);

      if (lAtoms->getNum() > 0 || lBonds->getNum() > 0 ) {
         ChemDisplayPath * cpt = new ChemDisplayPath;
         cpt->ref();
         (void)cpt->setPath(pathCD, lAtoms , lBonds ,NULL ,NULL);
         esel->merge(cpt);
      }

      lAtoms->deleteValues(0,-1);
      lAtoms->setNum(0);
      lBonds->deleteValues(0,-1);
      lBonds->setNum(0);
   }


   // Not sure if these should be deleted or not.  With simple testing it
   // seems ok to do so.  Added 3/22/2000
   if (lAtoms) delete lAtoms;
   if (lBonds) delete lBonds;
}




/**
 *   This method takes a set of atoms (and bonds) and sets the inventor
 *   selection node to those atoms.
 *
 *  supports multiple ChemDisplay objects
 *  (same as other setSelection, but this is faster with the BitVec)
 *
 * NOTE if its not working try clearing selection first.
 */
void 
WxVizTool::setSelection(const vector<bool>& atoms, const vector<bool>& bonds)
{
  // Important performance note: don't call the fragment bonds() method
  // since for large systems this can be slow.  Instead assume that
  // SGFragment::touchNumbers has been called to generate a cached
  // bondIndices instance that can be used for fast access
   
  int j;
  int numSelAtoms ;
  int numSelBonds ;
  SGSelection *esel = getFW().getViewer().getSel();
  esel->deselectAll();
  MFVec2i *lAtoms = new MFVec2i;
  MFVec2i *lBonds = new MFVec2i;

  SGContainer& sg = getFW().getSceneGraph();
  SGFragment *frag = sg.getFragment();

  // for each displayStyle

  vector<string> names = sg.getDisplayStyleNames();
  int numDisplayStyles = names.size();
  for (int ds = 0; ds < numDisplayStyles; ds++)
  {
     numSelAtoms = 0 ;
     numSelBonds = 0 ;
     lAtoms->deleteValues(0,-1);
     lBonds->deleteValues(0,-1);
     SoPath * pathCD =  sg.getSelectionPath(esel, ds);

     if (pathCD == NULL) { continue; }
     string name = names[ds];

     int numAtoms = atoms.size();
     for (j=0 ; j < numAtoms ; j++) {
       if (atoms[j] == 1 && 
           frag->atomRef(j)->displayStyle().getName() == name ) {
           lAtoms->set1Value(numSelAtoms,SbVec2i(j,1));
           numSelAtoms++;
        }
     }
     lAtoms->setNum(numSelAtoms);

     int numBonds = bonds.size();
     for (j=0; j< numBonds; j++) {
       if (bonds[j] == 1 && frag->getBondStyle(bonds[j]).getName() == name) { 
           lBonds->set1Value(numSelBonds,SbVec2i(j,1));
           numSelBonds++;
        }
     }
     lBonds->setNum(numSelBonds);
     /*
     if (lAtoms->getNum() > 0 || lBonds->getNum() > 0 ) {
        ChemDisplayPath * cpt = new ChemDisplayPath;
        cpt->ref();
        (void)cpt->setPath(pathCD, lAtoms , lBonds ,NULL ,NULL);
        //esel->select(cpt);
        esel->merge(cpt);
     }
     */
  }


  // Not sure if these should be deleted or not.  With simple testing it
  // seems ok to do so.  Added 3/22/2000
  if (lAtoms) delete lAtoms;
  if (lBonds) delete lBonds;
}

void WxVizTool::invertSelection()
{
   SGContainer& sg = getFW().getSceneGraph();
   SGFragment *frag = sg.getFragment();

   // Initialize bitmask vectors
   vector<bool> vatoms(frag->numAtoms(), false);
   vector<bool> vbonds(frag->numBonds(), false);

   // Get current selection into bit mask
   int idx;
   for (idx=0; idx<frag->m_atomHighLight.size(); idx++) {
      vatoms[frag->m_atomHighLight[idx]] = true;
   }
   for (idx=0; idx<frag->m_bondHighLight.size(); idx++) {
      vbonds[frag->m_bondHighLight[idx]] = true;
   }

   frag->m_atomHighLight.clear();
   for (idx=0; idx<vatoms.size(); idx++) {
      if (!vatoms[idx]) frag->m_atomHighLight.push_back(idx);
   }
   frag->m_bondHighLight.clear();
   for (idx=0; idx<vbonds.size(); idx++) {
      if (!vbonds[idx]) frag->m_bondHighLight.push_back(idx);
   }

   setSelection(vatoms, vbonds);

}


/**
 * Selects all atoms and bonds in residues at the specified residue indices.
 */
void 
WxVizTool::selectResidues(const vector<int>& resIndicies, bool extendSelect)
{
   int idx;

   SGContainer& sg = getFW().getSceneGraph();
   SGFragment *sgfrag = sg.getFragment();

   vector<int>* atomHighLight = &(sgfrag->m_atomHighLight);
   vector<int>* bondHighLight = &(sgfrag->m_bondHighLight);

   vector<bool> bondbits(sgfrag->numBonds(), false);
   vector<bool> atombits(sgfrag->numAtoms(), false);

   int numAtoms = sgfrag->numAtoms();
   int numBonds = sgfrag->numBonds();
   if ( extendSelect) {
      for (idx=0; idx<atomHighLight->size(); idx++) {
         atombits[(*atomHighLight)[idx]] = true;
      }
      for (idx=0; idx<bondHighLight->size(); idx++) {
         bondbits[(*bondHighLight)[idx]] = true;
      }
   }
   atomHighLight->clear() ;
   bondHighLight->clear() ;

   const vector<Residue*>& residues = ((Fragment*)sgfrag)->residues();
   int nRes = resIndicies.size();
   for (idx=0; idx<nRes; idx++) {
      Residue *res = residues[resIndicies[idx]];
      vector <TAtm*> atomptrs = sgfrag->findResidueAtoms(res) ;

      // add atom indices
      int numAtoms = atomptrs.size();
      int numBonds=0;

      for( int i = 0; i < numAtoms; i++) {
         TAtm *anAtom =  atomptrs[i] ;
         atombits[anAtom->index()] = true;

         // add bond indices
         const vector<TBond*>& atomBonds = anAtom->bondList();
         numBonds = atomBonds.size();
         for (int b =0; b <numBonds; b++) {
            TBond *aBond = atomBonds[b] ;
            bondbits[aBond->index()] = true;
         }
      }

   }

   // Now restore from bitmasks
   for (idx=0; idx<numAtoms; idx++) {
      if (atombits[idx]) atomHighLight->push_back(idx);
   }
   for (idx=0; idx<numBonds; idx++) {
      if (bondbits[idx]) bondHighLight->push_back(idx);
   }

}

/**
 * Selects all atoms and bonds in residues at the specified residue indices.
 */
void WxVizTool::selectResidue(const Residue& res, bool extendSelect)
{
   int idx;

   SGContainer& sg = getFW().getSceneGraph();
   SGFragment *sgfrag = sg.getFragment();

   vector<int>* atomHighLight = &(sgfrag->m_atomHighLight);
   vector<int>* bondHighLight = &(sgfrag->m_bondHighLight);

   vector<bool> bondbits(sgfrag->numBonds(), false);
   vector<bool> atombits(sgfrag->numAtoms(), false);

   int numAtoms = sgfrag->numAtoms();
   int numBonds = sgfrag->numBonds();
   if ( extendSelect) {
      for (idx=0; idx<atomHighLight->size(); idx++) {
         atombits[(*atomHighLight)[idx]] = true;
      }
      for (idx=0; idx<bondHighLight->size(); idx++) {
         bondbits[(*bondHighLight)[idx]] = true;
      }
   }
   atomHighLight->clear() ;
   bondHighLight->clear() ;

   vector <TAtm*> atomptrs = sgfrag->findResidueAtoms(&res) ;

   // add atom indices
   int numResAtoms = atomptrs.size();
   int numAtomBonds=0;

   for( int i = 0; i < numResAtoms; i++) {
      TAtm *anAtom =  atomptrs[i] ;
      atombits[anAtom->index()] = true;

      // add bond indices
      const vector<TBond*>& atomBonds = anAtom->bondList();
      numAtomBonds = atomBonds.size();
      for (int b =0; b <numAtomBonds; b++) {
         TBond *aBond = atomBonds[b] ;
         bondbits[aBond->index()] = true;
      }
   }

   // Now restore from bitmasks
   for (idx=0; idx<numAtoms; idx++) {
      if (atombits[idx]) atomHighLight->push_back(idx);
   }
   for (idx=0; idx<numBonds; idx++) {
      if (bondbits[idx]) bondHighLight->push_back(idx);
   }

}


bool WxVizTool::areLabelsOn() const
{
  return p_vizfw->areLabelsOn();
}




