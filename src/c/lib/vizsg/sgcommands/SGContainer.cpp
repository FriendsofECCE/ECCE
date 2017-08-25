#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;

#ifdef __GNUC__
#include <ext/hash_map>
   using __gnu_cxx::hash_map;
   using __gnu_cxx::hash;
#endif

#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoShapeHints.H"
#include "inv/nodes/SoClipPlane.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoSearchAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/fields/SoMFNode.H"

#include "util/ETimer.H"   
#include "util/Color.H"   
#include "util/Spectrum.H"   
#include "util/STLUtil.H"   
#include "util/CompareOps.H"

#include "tdat/SingleGrid.H"
#include "tdat/PropGrids.H"
#include "tdat/PropVecGrid.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/TResTab.H"
#include "tdat/TResItem.H"
#include "tdat/LinkCellList.H"

#include "inv/ChemKit/ChemKit.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemColor.H"
#include "inv/ChemKit/ChemRadii.H"
#include "inv/ChemKit/ChemDisplayParam.H"
#include "inv/ChemKit/SbResidue.H"

#include "viz/CustomBBoxNode.H"
#include "viz/MoveAction.H"
#include "viz/AtomDeleteAction.H"
#include "viz/AtomMeasureAngle.H"
#include "viz/AtomMeasureTorsion.H"
#include "viz/AtomMeasureDist.H"
#include "viz/SGLattice.H"
#include "viz/SGContainer.H"   
#include "viz/SGFragment.H"   
#include "viz/SGPlane.H"   
#include "viz/MillerPlane.H"   
#include "viz/StyledPlane.H"   

// comment this out to turn of double bonds and rings which cause crashes
// define it if you are trying to do ring bug debugging.
#define debuggingrings

// comment this out to turn off our timer tests...
//#define timertests

//#define DEBUG

//static  SoClipPlane *test = 0;

SO_NODE_SOURCE(SGContainer);
const float SGContainer::selectRGB[] = {1.0, 0.0, 1.0};

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void SGContainer::initClass()
{
   SO_NODE_INIT_CLASS(SGContainer, SoSwitch,"Switch");
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
SGContainer::SGContainer()
{
  SO_NODE_CONSTRUCTOR(SGContainer);
  constructor();
}

SGContainer *SGContainer::clone()
{
  SGContainer *ret = new SGContainer;
  return ret;
}

string SGContainer::commandObjectType() const
{
  return "SGContainer";
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void SGContainer::constructor()
{
   // For gridded data in the viewer
   setCurrentGrid(0);  // No need to have one to start is there

   whichChild.setValue(SO_SWITCH_ALL);

   p_showRings = true;

   // Show one thing at a time
   p_mainSep = new SoSeparator;
   addChild(p_mainSep);


   // Show one thing at a time
   SoSeparator *sep2 = new SoSeparator;
   p_mainSep->addChild(sep2);

   p_viewBbox = 0;
   SoSeparator *bsep = new SoSeparator;
   p_viewBbox = new CustomBBoxNode();
   bsep->addChild(p_viewBbox);
   sep2->addChild(bsep);

   // The Top for Lines
   p_topSepLine = new SoSeparator;
   sep2->addChild(p_topSepLine);

   // lattice switch
   SoSwitch *latticeswitch = new SoSwitch;
   latticeswitch->setName("latticeswitch");
   latticeswitch->whichChild.setValue(SO_SWITCH_NONE);

   sep2->addChild(latticeswitch);
   SGLattice *lattice = new SGLattice;
   lattice->setName("lattice");
   latticeswitch->addChild(lattice);

   // Plane switch - goes with lattice code
   SoSwitch *planeswitch = new SoSwitch;
   planeswitch->setName("planeswitch");
   planeswitch->whichChild.setValue(SO_SWITCH_NONE);
   sep2->addChild(planeswitch);

   // The Top for rot/trans Manipulators
   p_topManip = new SoGroup;
   sep2->addChild(p_topManip);

   // The Top for rotate Manipulators
   p_topRotManip = new SoGroup;
   sep2->addChild(p_topRotManip);

   // two sided shading
   p_shapeHints = new SoShapeHints;
   p_shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
   p_shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE ;
   sep2->addChild(p_shapeHints) ;

   // The Top for Labels
   //   p_topSepLabel = new SoSeparator;  
   //this was changed to SoSwitch in VRExpt, 
   //although I can't remember why...
   p_topSepLabel = new SoSwitch;
   sep2->addChild(p_topSepLabel);
   p_topSepLabel->whichChild.setValue(SO_SWITCH_ALL);

   SoDrawStyle  * myStl = new SoDrawStyle;
   p_topSepLabel->addChild(myStl);
   p_topSepLine->addChild(myStl);
   myStl->style.setValue(SoDrawStyle::FILLED);
   myStl->linePattern.setValue(0x1C47);
   //myStl->linePattern.setValue(0xffffff);
   myStl->lineWidth.setValue(5);

   /* test showing background image
   SoTexture2 *test = new SoTexture2;
   test->filename.setValue("/tmp/test.rgb");
   p_topSepLabel->addChild(test);
   */

   p_topLabel = new SoGroup;
   p_topSepLabel->addChild(p_topLabel);

   // Since we figure this must have been a work around for not having
   // H-Bonds, make them look the same as our H bonds.
   // Maybe we should just eliminate this altogether?
   p_topLine = new SoGroup;
   p_topSepLine->addChild(p_topLine);
   myStl = new SoDrawStyle;
   p_topLine->addChild(myStl);
   myStl->style.setValue(SoDrawStyle::FILLED);
   myStl->linePattern.setValue(0xcccc);
   myStl->lineWidth.setValue(3);
   p_topLine->addChild(new SoGroup);



   // The Hydrogen bonds need to have a nicer line style etc
   // so create a separator and a special style.
   // Note that the gettopHBond method needs to return the
   // correct node, not the top one
   // Sotiris wanted a thin line (but not too thin:))
   p_topHBonds = new SoSeparator;
   p_topSepLine->addChild(p_topHBonds);
   myStl = new SoDrawStyle;
   p_topHBonds->addChild(myStl);
   myStl->style.setValue(SoDrawStyle::FILLED);
   myStl->linePattern.setValue(0xcccc);
   myStl->lineWidth.setValue(3);
   p_topHBonds->addChild(new SoGroup);


   // A switch for trajectory data.  Not sure if switch is useful but...
   p_topAtomVectors = new SoSwitch;
   p_topAtomVectors->whichChild.setValue(SO_SWITCH_ALL);
   p_mainSep->addChild(p_topAtomVectors);


   // The main CS switch
   p_csSwitch = new SoSwitch;
   p_csSwitch->whichChild.setValue(SO_SWITCH_ALL);
   p_mainSep->addChild(p_csSwitch);

   p_chemSysTop = new SoSeparator;
   p_csSwitch->addChild(p_chemSysTop);

   // Top for clip plane manips
   //SbBox3f bbox( -12, -12, -12, 12, 12, 12 ) ;
   //SoClipPlaneManip *scp = new SoClipPlaneManip ;
   //scp->setValue( bbox, SbVec3f( 0, 1, 0), 0.1) ;
   //  p_topClipManip = new SoGroup ;
   //p_topClipManip->addChild(scp) ;

   //  p_chemSysTop->insertChild(p_topClipManip,0) ;

   // *** DISPLAY ***
   p_display = new SoGroup;
   p_display->ref();
   //test = new SoClipPlane();
   //p_display->addChild(test);

   // add Material for the ChemBaseData node.  
   // Don't set any default values here
   p_cdMat = new SoMaterial;
   p_display->addChild(p_cdMat);
   p_cdMat->shininess.setValue(0.8);

   // For now, both atom and bond labels will be set to the same
   // color.  See command AtomPropLabelColor.
   p_colors = new ChemColor;
   p_colors->atomLabelColorBinding.setValue(ChemColor::ATOM_LABEL_OVERALL);
   p_colors->bondLabelColorBinding.setValue(ChemColor::BOND_LABEL_OVERALL);
   p_display->addChild(p_colors);

   p_radii = new ChemRadii;
   p_display->addChild(p_radii);
   //p_radii->atomRadiiBinding.setValue(ChemRadii::RADII_OVERALL);
   //p_radii->atomRadiiBinding.setValue(1);

   p_CPK_radii = new ChemRadii;
   //p_CPK_radii->atomRadiiBinding.setValue
   //   (ChemRadii::RADII_OVERALL);
   p_display->addChild(p_CPK_radii);

   // Each style will be a group under this node.  This makes it easy to
   // do things like delete them all and reset to a new style
   p_dynamicStyles = new SoGroup();
   p_dynamicStyles->ref();
   p_display->addChild(p_dynamicStyles);

   createStyle(DisplayDescriptor("default", 
      DisplayStyle::fromStyle(DisplayStyle::BALLWIRE), "Element"));

   // ---------------------Property Nodes --------------------------
   // The MO switch
   p_MOSwitch = new SoSwitch;
   p_mainSep->addChild(p_MOSwitch);
   initMORoot();

   p_NMVecSwitch = new SoSwitch;
   p_mainSep->addChild(p_NMVecSwitch);

   // The NormalMode switch
   p_NMSwitch = new SoSwitch;
   p_mainSep->addChild(p_NMSwitch);

   // -------------------------------------------------------------

   // Since Fragment is allowed to have no atoms now-a-days,  just add it
   // right away.
   SGFragment *frag = new SGFragment();
   frag->setMainDisplayStyle( DisplayStyle::BALLWIRE ) ;
   insertFragment(frag);
   p_chemSysTop->addChild(p_display);

   frag->touchNumbers();
   touchChemDisplay(frag);

}


/**
 * Determine if the name style exists or not.
 */
bool SGContainer::styleExists(const string& name) const
{
   bool ret = false;
   for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
      if (name == p_displayDescs[idx].getName()) {
         ret = true;
         break;
      }
   }
   return ret;

}


/**
 * Determine if any styles in use require the protein attributes to
 * be computed.
 */
bool SGContainer::proteinStyleExists() const
{
   bool ret = false;
   int numDisplays = p_displayParams.size();
   for (int idx=0; idx<numDisplays; idx++) {
      if (p_displayParams[idx]->residueDisplayStyle.getValue() != 
            ChemDisplayParam::DISPLAY_RESIDUES_NONE) {
         ret = true;
         break;
      }
   }
   return ret;
}

/**
 * Get the set of all display style names in use.
 * The top level nodes for each display group contain the generated or
 * user assigned name.
 */
vector<string> SGContainer::getDisplayStyleNames() const
{
   vector<string> names;
   for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
      names.push_back(p_displayDescs[idx].getName());
   }
   return names;
}


DisplayDescriptor SGContainer::getDisplayDescriptor(const string& name) const
{
   DisplayDescriptor ret;
   for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
      if (name == p_displayDescs[idx].getName()) {
         ret = p_displayDescs[idx];
         break;
      }
   }
   return ret;
}


/**
 * Get number of display style groups currently in use.
 */
int SGContainer::getNumDisplayStyles() const
{
   return p_dynamicStyles->getNumChildren();
}


void SGContainer::resetToStyle( const DisplayDescriptor& dd)
{
   // Remove existing styles
   p_dynamicStyles->removeAllChildren();

   // Remove our stashes displays
   p_displayDescs.clear();
   p_displays.clear();
   p_displayParams.clear();

   // Create new style
   createStyle(dd);

}



void SGContainer::changeStyle( const DisplayDescriptor& dd)
{
   // Find index of style
   int whichStyle = -1;
   string name = dd.getName();
   for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
      if (name == p_displayDescs[idx].getName()) {
         whichStyle = idx;
         p_displayDescs[idx] = dd;
         removeStyleNodes(whichStyle);
         createStyle(dd,whichStyle);
         break;
      }
   }

}




void SGContainer::createStyle( const DisplayDescriptor& dd, int pos)
{
   p_displayDescs.push_back(dd);

   // Create a node for this style.
   SoGroup *styleNode = new SoGroup();


   // Add custom color node
   ChemColor *local = 0;
   if (dd.getColorScheme() == "Block Color") {
      local = new ChemColor;
      Color c(dd.getBlockColor());
      local->atomColorBinding.setValue(ChemColor::ATOM_OVERALL);
      //local->bondColorBinding.setValue(ChemColor::BOND_OVERALL);
      local->atomColor.set1Value(0, c.red(), c.green(), c.blue());
      local->bondColor.set1Value(0, c.red(), c.green(), c.blue());
      local->residueColorBinding.setValue(ChemColor::RESIDUE_OVERALL);
      local->residueColor.set1Value(0, c.red(), c.green(), c.blue());
   } else if (dd.getColorScheme() == "Element") {
   /*
   SoMaterial *xx = new SoMaterial;
   styleNode->addChild(xx);
   xx->transparency.setValue(0.8);
  xx->transparency.setIgnored(false);
  */
      // Link in the existing color node
      local  = getColorNode();
   } else if (dd.getColorScheme() == "Atom Index") {
      local = createAtomIndexColors(dd.getSpectrumType(), new ChemColor);
   } else if (dd.getColorScheme() == "Atom Charge") {
      local = createAtomChargeColors(dd.getSpectrumType(), new ChemColor);
   } else if (dd.getColorScheme() == "Position from CM") {
      DisplayStyle::Style style = DisplayStyle::toStyle(dd.getStyle());
      if (style >= DisplayStyle::CAWIRE) {
        local = createResidueCenterMassColors(dd.getSpectrumType(), new ChemColor);
      } else {
        local = createCenterMassColors(dd.getSpectrumType(), new ChemColor);
      }
   } else if (dd.getColorScheme() == "Residue Index") {
      local = createResidueIndexColors(dd.getSpectrumType(), new ChemColor,
                                       dd.getStyle());
   } else if (dd.getColorScheme() == "Residue Name") {
      local = createResidueNameColors(dd.getSpectrumType(), new ChemColor);
   } else if (dd.getColorScheme() == "Hydrophobicity") {
      local = createHydrophobicityColors(dd.get3ColorStyle(), new ChemColor);
   } else if (dd.getColorScheme() == "Secondary Structure") {
      local = createSecondaryStructureColors(dd.get3ColorStyle(), new ChemColor);
   } else if (dd.getColorScheme() == "Chains") {
      local = createChainColors(dd.getSpectrumType(), new ChemColor);
   } else if (dd.getColorScheme() == "Residue Charge") {
      local = createResidueChargeColors(dd.get3ColorStyle(), new ChemColor);
   }
   if (local != 0) styleNode->addChild(local);

   // Add a ref to the correct radii for this style
   if (dd.getStyle() == DisplayStyle::fromStyle(DisplayStyle::CPK)) {
      styleNode->addChild(p_CPK_radii);
   } else {
      styleNode->addChild(p_radii);
   }
   ChemDisplayParam *displayParam = createDisplayParam(dd);

   // Create ChemDisplay for holding atoms/bonds
   ChemDisplay *chemDisplay = new ChemDisplay;
   p_displays.push_back(chemDisplay);

  //
  // Currently this is the only style that we support for Cartoons so
  // in here we set the schematic color table.
  // More work needs to be done to decide what we should do.
  // Default is SCHEMATIC_PER_SECONDARY_TYPE so we don't bother to set it.
  // Aaahhhh doesn't compile - private variables though in the documentation
  // Had to change moiv to make this work.
  //
  if (dd.getStyle() == "Cartoon") {
     chemDisplay->schematicArrowIndex.set1Value(0,0);
     chemDisplay->schematicTubeIndex.set1Value(0,1);
     chemDisplay->schematicCylinderIndex.set1Value(0,2);
  }


   styleNode->addChild(displayParam);
   styleNode->addChild(chemDisplay);
   p_dynamicStyles->addChild(styleNode);

}


/**
 * Create a color node with a spectrum of colors that represents
 * the distance from the center of mass.
 * All atoms are currently considered in computing the min/max for
 * the spectrum as we decided this is how people will/should use this.
 * @todo maybe compute spectrum based on selection only
 */
ChemColor *SGContainer::createCenterMassColors(const string& spectrumtype, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);

   SGFragment *frag = getFragment();

   // compute min/max
   int nAtoms = frag->numAtoms();
   int nSlices = nAtoms;
   double cx, cy, cz;
   frag->centerOfMass(cx, cy, cz);
   vector<double> distances;

   float min = 10000.;
   float max = 0.;
   int idx;
   float dist;
   TAtm *atm;
   MPoint secondPoint(cx, cy, cz);
   for (idx = 0; idx < nAtoms; idx++ ) {
      atm = frag->atomRef(idx);
      const double *at1 = atm->coordinates();
      MPoint firstPoint(at1[0],at1[1],at1[2]);
      dist = firstPoint.subtract1(secondPoint).length();
      distances.push_back(dist);
      if (dist < min) min = dist;
      if (dist > max) max = dist;
   }

   Spectrum spectrum(min, max, nSlices);

   float r, g, b;
   for (idx=0; idx<nAtoms; idx++) {
      if (spectrumtype == "Red-White-Blue") {
        spectrum.getPosNegMidpointValue(distances[idx], r, g, b);
      } else if (spectrumtype == "Rainbow") {
        spectrum.getRainbowValue(distances[idx], r, g, b);
      } else if (spectrumtype == "Red") {
        spectrum.getNegValue(distances[idx], r, g, b);
      } else if (spectrumtype == "Blue") {
        spectrum.getBlueValue(distances[idx], r, g, b);
      } else if (spectrumtype == "Green") {
        spectrum.getGreenValue(distances[idx], r, g, b);
      } else {
        //cout << "Unrecognized spectrum type" << spectrumtype << endl;
        spectrum.getRainbowValue(distances[idx], r, g, b);
      }
      local->atomColor.set1Value(idx, r, g, b);
   }

   return local;
}


ChemColor *SGContainer::createResidueCenterMassColors(const string& spectrumtype, ChemColor *local)
{
   local->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

   SGFragment *frag = getFragment();
   ChemData *moiv = (ChemData*)frag;
   TResTab restab;

   // compute min/max
   int nResidues = frag->numResidues();
   int nSlices = nResidues;
   double cx, cy, cz;
   frag->centerOfMass(cx, cy, cz);
   vector<float> distances;

   float min = 10000.;
   float max = 0.;
   int idx;
   float dist;
   MPoint secondPoint(cx, cy, cz);
   double x, y, z;
   for (idx = 0; idx < nResidues; idx++ ) {
      frag->residueCenterOfMass(idx,x,y,z);
      if (restab.verify(frag->residue(idx)->name().c_str()) != TResItem::Other) {
        MPoint firstPoint(x,y,z);
        dist = firstPoint.subtract1(secondPoint).length();
        distances.push_back(dist);
        if (dist < min) min = dist;
        if (dist > max) max = dist;
      }
   }

   Spectrum spectrum(min, max, nSlices);

   moiv->residueColorIndex.setNum(0);
   float r, g, b;
   for (int rdx=0; rdx<nResidues; rdx++) {
      if (restab.verify(frag->residue(rdx)->name().c_str()) != TResItem::Other) {
        dist = distances[rdx];
      } else {
        dist = min;
      }
      if (spectrumtype == "Red-White-Blue") {
        spectrum.getPosNegMidpointValue(dist, r, g, b);
      } else if (spectrumtype == "Rainbow") {
        spectrum.getRainbowValue(dist, r, g, b);
      } else if (spectrumtype == "Red") {
        spectrum.getNegValue(dist, r, g, b);
      } else if (spectrumtype == "Blue") {
        spectrum.getBlueValue(dist, r, g, b);
      } else if (spectrumtype == "Green") {
        spectrum.getGreenValue(dist, r, g, b);
      } else {
        //cout << "Unrecognized spectrum type" << spectrumtype << endl;
        spectrum.getRainbowValue(dist, r, g, b);
      }
      local->residueColor.set1Value(rdx, r, g, b);
      moiv->residueColorIndex.set1Value(rdx,rdx);
   }

   return local;
}


ChemColor *SGContainer::createAtomIndexColors(const string& spectrumtype, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);

   SGFragment *frag = getFragment();

   // compute min/max
   // Hacking for now - should compute
   int nAtoms = frag->numAtoms();
   int nSlices = nAtoms;
   float min = 0;
   float max = nAtoms;
   Spectrum spectrum(min, max, nSlices);

   float r, g, b;
   for (int idx=0; idx<nAtoms; idx++) {
      if (spectrumtype == "Red-White-Blue") {
        spectrum.getPosNegMidpointValue(idx, r, g, b);
      } else if (spectrumtype == "Rainbow") {
        spectrum.getRainbowValue(idx, r, g, b);
      } else if (spectrumtype == "Red") {
        spectrum.getNegValue(idx, r, g, b);
      } else if (spectrumtype == "Blue") {
        spectrum.getBlueValue(idx, r, g, b);
      } else if (spectrumtype == "Green") {
        spectrum.getGreenValue(idx, r, g, b);
      } else {
        //cout << "Unrecognized spectrum type" << spectrumtype << endl;
        spectrum.getRainbowValue(idx, r, g, b);
      }
      local->atomColor.set1Value(idx, r, g, b);
   }

   return local;
}


ChemColor *SGContainer::createAtomChargeColors(const string& spectrumtype, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);

   SGFragment *frag = getFragment();

   // compute min/max
   // Hacking for now - should compute
   int nAtoms = frag->numAtoms();
   int nSlices = nAtoms;
   float min = 0.;
   float max = 0.;
   int idx;
   float charge;
   TAtm *atm;
   for (idx = 0; idx < nAtoms; idx++ ) {
      atm = frag->atomRef(idx);
      charge = atm->partialCharge();
      if (charge < min) min = charge;
      if (charge > max) max = charge;
   }

   Spectrum spectrum(min, max, nSlices);

   float r, g, b;
   for (idx=0; idx<nAtoms; idx++) {
      atm = frag->atomRef(idx);
      if (spectrumtype == "Red-White-Blue") {
        spectrum.getPosNegMidpointValue(atm->partialCharge(), r, g, b);
      } else if (spectrumtype == "Rainbow") {
        spectrum.getRainbowValue(atm->partialCharge(), r, g, b);
      } else if (spectrumtype == "Red") {
        spectrum.getNegValue(atm->partialCharge(), r, g, b);
      } else if (spectrumtype == "Blue") {
        spectrum.getBlueValue(atm->partialCharge(), r, g, b);
      } else if (spectrumtype == "Green") {
        spectrum.getGreenValue(atm->partialCharge(), r, g, b);
      } else {
        //cout << "Unrecognized spectrum type" << spectrumtype << endl;
        spectrum.getRainbowValue(atm->partialCharge(), r, g, b);
      }
      local->atomColor.set1Value(idx, r, g, b);
   }

   return local;
}



/**
 * Create a color table using a spectrum based on residue indices.
 * The color table is for atoms displayed in one of the traditional styles.
 */
ChemColor *SGContainer::createResidueIndexColors(const string& spectrumtype,
                                                 ChemColor *local, string style)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
   local->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

   bool isRibbon;
   if (style == "Line Ribbon" || style == "Flat Ribbon" ||
       style == "Solid Ribbon") {
     isRibbon = true;
   } else {
     isRibbon = false;
   }

   SGFragment *frag = getFragment();
   ChemData *moiv = (ChemData*)frag;

   // compute min/max
   //int nAtoms = frag->numAtoms();
   int nResidues = frag->numResidues();
   int nSlices = nResidues;
   int rdx;
   TResTab restab;
   if (isRibbon) {
     nSlices = 0;
     for (rdx=0; rdx<nResidues; rdx++) {
       if (restab.verify(frag->residue(rdx)->name().c_str()) != TResItem::Other) {
         nSlices++;
       }
     }
   }
   float min = 0;
   float max = nSlices;
   Spectrum spectrum(min, max, nSlices);

   moiv->residueColorIndex.setNum(0);
   float r, g, b;
   int idx = 0;
   int ldx = 0;
   for (rdx=0; rdx<nResidues; rdx++) {
      if (isRibbon && restab.verify(frag->residue(rdx)->name().c_str()) !=
          TResItem::Other) {
        ldx++;
      } else if (!isRibbon) {
        ldx++;
      }
      if (spectrumtype == "Red-White-Blue") {
        spectrum.getPosNegMidpointValue(ldx, r, g, b);
      } else if (spectrumtype == "Rainbow") {
        spectrum.getRainbowValue(ldx, r, g, b);
      } else if (spectrumtype == "Red") {
        spectrum.getNegValue(ldx, r, g, b);
      } else if (spectrumtype == "Blue") {
        spectrum.getBlueValue(ldx, r, g, b);
      } else if (spectrumtype == "Green") {
        spectrum.getGreenValue(ldx, r, g, b);
      } else {
        //cout << "Unrecognized spectrum type" << spectrumtype << endl;
        spectrum.getRainbowValue(ldx, r, g, b);
      }
      Residue *residue = frag->residue(rdx);
      vector<TAtm*> atoms = frag->findResidueAtoms(residue);

      // Set the residue color entry
      local->residueColor.set1Value(rdx, r, g, b);
      moiv->residueColorIndex.set1Value(rdx,ldx);

      // Set the atom color entries
      int nResAtoms = atoms.size();
      for (int adx=0; adx<nResAtoms; adx++) {
         local->atomColor.set1Value(idx++, r, g, b);
      }
   }

   return local;
}


/**
 * Create a color table using a spectrum based on residue names.
 * The color table is for atoms displayed in one of the traditional styles.
 */
ChemColor *SGContainer::createResidueNameColors(const string& spectrumtype, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
   local->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

   SGFragment *frag = getFragment();
   ChemData *moiv = (ChemData*)frag;

   //int nAtoms = frag->numAtoms();
   int nResidues = frag->numResidues();

   // Create a hash table to uniquely assign residue names to indices
   // This is really a btree and might be too slow....
   map<string, int, less<string> > nameHash;

   int rdx;
   int index = 0;
   string name;
   for (rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);
      name = residue->name();
      if ( nameHash.find(name) == nameHash.end()) {
         nameHash[name] = index;
         index++;
      }
   }

   // compute min/max
   int nSlices = index;  // the number of unique residues
   float min = 0;
   float max = index;    // the number of unique residuesResidues;
   Spectrum spectrum(min, max, nSlices);

   moiv->residueColorIndex.setNum(0);
   float r, g, b;
   int idx = 0;
   for (rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);
      index = nameHash[residue->name()];
      spectrum.getRainbowValue(index, r, g, b);
      if (spectrumtype == "Red-White-Blue") {
        spectrum.getPosNegMidpointValue(index, r, g, b);
      } else if (spectrumtype == "Rainbow") {
        spectrum.getRainbowValue(index, r, g, b);
      } else if (spectrumtype == "Red") {
        spectrum.getNegValue(index, r, g, b);
      } else if (spectrumtype == "Blue") {
        spectrum.getBlueValue(index, r, g, b);
      } else if (spectrumtype == "Green") {
        spectrum.getGreenValue(index, r, g, b);
      } else {
        //cout << "Unrecognized spectrum type" << spectrumtype << endl;
        spectrum.getRainbowValue(index, r, g, b);
      }

      // Set the residue color entry
      local->residueColor.set1Value(rdx, r, g, b);
      moiv->residueColorIndex.set1Value(rdx,rdx);

      // Set the atom color entries
      vector<TAtm*> atoms = frag->findResidueAtoms(residue);
      int nResAtoms = atoms.size();
      for (int adx=0; adx<nResAtoms; adx++) {
         local->atomColor.set1Value(idx++, r, g, b);
      }
   }

   return local;
}


ChemColor *SGContainer::createHydrophobicityColors(const vector<string>& colors, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
   local->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

   SGFragment *frag = getFragment();
   ChemData *moiv = (ChemData*)frag;

   //int nAtoms = frag->numAtoms();
   int nResidues = frag->numResidues();

   TResTab restab;
   int hydrophobicity, charge;
   int cindex;
   vector<int> hydrophobicities;
   for (int rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);
      hydrophobicity = restab.hydrophobicity(residue->name().c_str());
      // Evaluate charge. If it is non-zero, then residue is hydrophylic
      charge = restab.charge(residue->name().c_str());
      if (residue->couplingInfo() == Residue::HEAD) {
         charge++;
      } else if (residue->couplingInfo() == Residue::TAIL) {
         charge--;
      }
      if (charge != 0) {
        hydrophobicity = -1;
      }
      cindex=1;
      if (hydrophobicity < 0) {
         cindex = 0;
      } else if (hydrophobicity > 0) {
         cindex = 2;
      }
      hydrophobicities.push_back(cindex);
   }

   SbColor sbcolors[3];
   for (size_t idx=0; idx<colors.size(); idx++) {
      Color color(colors[idx]);
      sbcolors[idx] = SbColor(color.red(), color.green(), color.blue());
   }
   moiv->residueColorIndex.setNum(0);
   //float r, g, b;
   int idx = 0;
   for (int rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);

      // Set the residue color entry
      local->residueColor.set1Value(rdx, sbcolors[hydrophobicities[rdx]]);
      moiv->residueColorIndex.set1Value(rdx,rdx);

      // Set the atom color entries
      vector<TAtm*> atoms = frag->findResidueAtoms(residue);
      int nResAtoms = atoms.size();
      for (int adx=0; adx<nResAtoms; adx++) {
         local->atomColor.set1Value(idx++, sbcolors[hydrophobicities[rdx]]);
      }
   }

   return local;
}


/**
 * Create colors for secondary structure color scheme.
 */
ChemColor *SGContainer::createSecondaryStructureColors(const vector<string>& colors, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
   local->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

   SGFragment *frag = getFragment();
   ChemData *moiv = (ChemData*)frag;

   //int nAtoms = frag->numAtoms();
   int nResidues = frag->numResidues();

   vector<int> structure;
   for (int rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);
      if (residue->getProteinAttribute() == Residue::SHEET) {
        structure.push_back(0);
      } else if (residue->getProteinAttribute() == Residue::HELIX) {
        structure.push_back(2);
      } else {
        structure.push_back(1);
      }
   }

   SbColor sbcolors[3];
   for (size_t idx=0; idx<colors.size(); idx++) {
      Color color(colors[idx]);
      sbcolors[idx] = SbColor(color.red(), color.green(), color.blue());
   }
   moiv->residueColorIndex.setNum(0);
   //float r, g, b;
   int idx = 0;
   for (int rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);

      // Set the residue color entry
      local->residueColor.set1Value(rdx, sbcolors[structure[rdx]]);
      moiv->residueColorIndex.set1Value(rdx,rdx);

      // Set the atom color entries
      vector<TAtm*> atoms = frag->findResidueAtoms(residue);
      int nResAtoms = atoms.size();
      for (int adx=0; adx<nResAtoms; adx++) {
         local->atomColor.set1Value(idx++, sbcolors[structure[rdx]]);
      }
   }


   // Probably not necessary but not harmful
   for (int sdx=0; sdx<3; sdx++) {
     local->schematicColor.set1Value(sdx,sbcolors[sdx]);
   }


   return local;
}

/**
 * Assign atom colors on a color spectrum based on the residue charge
 * for the residue each atom belongs to.
 */
ChemColor *SGContainer::createResidueChargeColors(const vector<string>& colors, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
   local->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

   SGFragment *frag = getFragment();
   ChemData *moiv = (ChemData*)frag;

   //int nAtoms = frag->numAtoms();
   int nResidues = frag->numResidues();

   TResTab restab;
   int charge;
   int cindex;
   vector<int> charges;
   for (int rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);
      charge = restab.charge(residue->name().c_str());
      if (residue->couplingInfo() == Residue::HEAD) {
         charge++;
      } else if (residue->couplingInfo() == Residue::TAIL) {
         charge--;
      }
      cindex=1;
      if (charge < 0) {
         cindex = 0;
      } else if (charge > 0) {
         cindex = 2;
      }
      charges.push_back(cindex);
   }

   SbColor sbcolors[3];
   for (size_t idx=0; idx<colors.size(); idx++) {
      Color color(colors[idx]);
      sbcolors[idx] = SbColor(color.red(), color.green(), color.blue());
   }
   moiv->residueColorIndex.setNum(0);
   //float r, g, b;
   int idx = 0;
   for (int rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);

      // Set the residue color entry
      local->residueColor.set1Value(rdx, sbcolors[charges[rdx]]);
      moiv->residueColorIndex.set1Value(rdx,rdx);

      // Set the atom color entries
      vector<TAtm*> atoms = frag->findResidueAtoms(residue);
      int nResAtoms = atoms.size();
      for (int adx=0; adx<nResAtoms; adx++) {
         local->atomColor.set1Value(idx++, sbcolors[charges[rdx]]);
      }
   }

   return local;
}

/**
 * Create a color table using a spectrum based on residue chains.
 * The color table is for atoms displayed in one of the traditional styles.
 */
ChemColor *SGContainer::createChainColors(const string& spectrumtype, ChemColor *local)
{
   local->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
   local->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

   SGFragment *frag = getFragment();
   ChemData *moiv = (ChemData*)frag;

   //int nAtoms = frag->numAtoms();
   int nResidues = frag->numResidues();

   // Create a hash table to uniquely assign residue chains to indices
   map<string, int, less<string> > chainHash;

   int rdx;
   int index = 0;
   string name;
   char chain;
   for (rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);
      chain = residue->chain();
      name = chain;
      if ( chainHash.find(name) == chainHash.end()) {
         chainHash[name] = index;
         index++;
      }
   }

   // compute min/max
   int nSlices = index;  // the number of unique residues
   float min = 0;
   float max = index;    // the number of unique residuesResidues;
   Spectrum spectrum(min, max, nSlices);

   moiv->residueColorIndex.setNum(0);
   float r, g, b;
   int idx = 0;
   for (rdx=0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx);
      chain = residue->chain();
      name = chain;
      index = chainHash[name];
      if (spectrumtype == "Red-White-Blue") {
        spectrum.getPosNegMidpointValue(index, r, g, b);
      } else if (spectrumtype == "Rainbow") {
        spectrum.getRainbowValue(index, r, g, b);
      } else if (spectrumtype == "Red") {
        spectrum.getNegValue(index, r, g, b);
      } else if (spectrumtype == "Blue") {
        spectrum.getBlueValue(index, r, g, b);
      } else if (spectrumtype == "Green") {
        spectrum.getGreenValue(index, r, g, b);
      } else {
        //cout << "Unrecognized spectrum type" << spectrumtype << endl;
        spectrum.getRainbowValue(index, r, g, b);
      }

      // Set the residue color entry
      local->residueColor.set1Value(rdx, r, g, b);
      moiv->residueColorIndex.set1Value(rdx,rdx);

      // Set the atom color entries

      vector<TAtm*> atoms = frag->findResidueAtoms(residue);
      int nResAtoms = atoms.size();
      for (int adx=0; adx<nResAtoms; adx++) {
         local->atomColor.set1Value(idx++, r, g, b);
      }
   }

   return local;
}


/**
 * Set the fragment selection arrays.  Does not finialize selection.
 */
bool SGContainer::selectStyle(const string& name)
{
   bool changed = false;

   int numStyles = p_dynamicStyles->getNumChildren();

   for (int idx=0; idx<numStyles; idx++) {

      //SoGroup *node = (SoGroup*)p_dynamicStyles->getChild(idx);

      if (name == p_displayDescs[idx].getName()) {
         changed = true;
         SGFragment *frag = getFragment();
         frag->m_atomHighLight.clear();
         frag->m_bondHighLight.clear();

         ChemDisplay *cd = p_displays[idx];

         int numAtoms = cd->atomIndex.getNum();
         for (int adx=0; adx<numAtoms; adx++) {
            // This works because we make an entry in atomIndex for each atom
            frag->m_atomHighLight.push_back(cd->atomIndex[adx][0]);
         }

         vector<TBond*> *bonds = frag->bonds();
         int numBonds = cd->bondIndex.getNum();
         for (int bdx=0; bdx<numBonds; bdx++) {
            // This works because we make an entry in bondIndex for each bond
            frag->m_bondHighLight.push_back(cd->bondIndex[bdx][0]);
         }
         delete bonds;


         break;
      }
   }

   return changed;
}


/**
 * All atoms and bonds with the named style are changed to 
 * another style (the first currently) and the named style is removed.
 */
void SGContainer::removeStyle(const string& name)
{
   int numStyles = p_dynamicStyles->getNumChildren();

   //int pos = -1;
   for (int idx=0; idx<numStyles; idx++) {

      //SoGroup *node = (SoGroup*)p_dynamicStyles->getChild(idx);

      if (name == p_displayDescs[idx].getName()) {
         Fragment *frag = getFragment();

         DisplayStyle newstyle = getMainDisplayStyle();

         // KLS 4/7/6 Is this the right place for this code???
         // Should always be at least one style
         vector<string> names = getDisplayStyleNames();
         newstyle.setName(names[0]);
         // If we are removing the 0th, use the 1st as the new style
         if (idx == 0) newstyle.setName(names[1]);

         ChemDisplay *cd = p_displays[idx];

         // Atoms
         int numAtoms = cd->atomIndex.getNum();
         for (int adx=0; adx<numAtoms; adx++) {
            // This works because we make an entry in atomIndex for each atom
            TAtm *atm = frag->atomRef(cd->atomIndex[adx][0]);
            atm->displayStyle(newstyle);
         }

         // Bonds
         vector<TBond*> *bonds = frag->bonds();
         int numBonds = cd->bondIndex.getNum();
         for (int bdx=0; bdx<numBonds; bdx++) {
            // This works because we make an entry in bondIndex for each bond
            TBond *bnd = (*bonds)[cd->bondIndex[bdx][0]];
            bnd->displayStyle(newstyle);
         }
         delete bonds;

         // Residues
         int numResidues = cd->residueIndex.getNum();
         for (int rdx=0; rdx<numResidues; rdx++) {
            Residue *res = frag->residue(cd->residueIndex[rdx][0]);
            res->displayStyle(newstyle);
            // Because we do not put atoms and bonds that are part of residues
            // into the residue ChemDisplay, we need to process them here
            // from the residue.  Might be better to change the way we do this
            // when the style is created.
            // A problem with this method is that we don't really know which
            // bonds to change - for now, we change them if they are connected
            // to a residue atom
            vector<TAtm*> atoms = frag->findResidueAtoms(res);
            for (size_t idx=0; idx<atoms.size(); idx++) {
               atoms[idx]->displayStyle(newstyle);
               const vector<TBond*>& bonds = atoms[idx]->bondList();
               for (size_t cidx=0; cidx<bonds.size(); cidx++) {
                  TBond *bond = bonds[cidx];
                  bond->displayStyle(newstyle);
               }
            }
         }

         removeStyleNodes(idx);

         break;
      }
   }
}


void SGContainer::removeStyleNodes(int which)
{

   //SoGroup *node = (SoGroup*)p_dynamicStyles->getChild(which);


   p_displayDescs.erase(p_displayDescs.begin()+which);
   p_displays.erase(p_displays.begin()+which);
   p_displayParams.erase(p_displayParams.begin()+which);
   p_dynamicStyles->removeChild(which);

}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
SGContainer::~SGContainer()
{
}


ChemDisplayParam * SGContainer::createDisplayParam(const DisplayDescriptor& dd)
{

   ChemDisplayParam *displayParam = new ChemDisplayParam ;
   p_displayParams.push_back(displayParam);

   // Have to do this for each new display
   displayParam->setShowRings(p_showRings);

   applyGlobalStyles(displayParam);

   applyStyle(dd, displayParam);

   return displayParam;
}



/**
 * Applies non-global styles specified in dd to the specified ChemDisplayParam
 */
void SGContainer::applyStyle(const DisplayDescriptor& dd, ChemDisplayParam *cdp)
{

   DisplayStyle::Style style = DisplayStyle::toStyle(dd.getStyle());
   // KLS should this be a different field to avoid mis-use of enum?
   if (style < DisplayStyle::CAWIRE) {
      cdp->displayStyle.setValue (toChemkitStyle(style)) ;
      cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_NONE);
      cdp->bondWireframeLineWidth.setValue(dd.getLineWidth());
      cdp->bondCylinderRadius.setValue(dd.getCylinderRadius() /100.0);

      cdp->bondCylinderComplexity.setValue(dd.getCylinderRQ() /100.0);
      cdp->bondCylinderDisplayStyle.setValue(dd.getBondCylinderOpt());

      // Spheres
      cdp->atomSphereDisplayStyle.setValue(dd.getAtomSphereOpt());
      DisplayStyle::Style style = DisplayStyle::toStyle(dd.getStyle());
      if (style != DisplayStyle::CPK) {
         cdp->atomRadiiScaleFactor.setValue(dd.getSphereSize()/100.0);
      } else {
         // cpk radii should always be 1.0
         cdp->atomRadiiScaleFactor.setValue(1.0);
      }
      cdp->atomSphereComplexity.setValue(dd.getSphereRQ()/100.0);
   } else {
      // Turn atoms off
      cdp->displayStyle.setValue (ChemDisplayParam::DISPLAY_NONE) ;

      // Residue style
      cdp->residueInterpolateColor.setValue(dd.getRibbonBlended());
      cdp->solidRibbonSmoothNormals.setValue(dd.getRibbonRounded());
      cdp->residueWireframeLineWidth.setValue(dd.getLineWidth());
      cdp->residueCoilWidth.setValue(dd.getCoilWidth());
      cdp->residueSheetWidth.setValue(dd.getSheetWidth());
      cdp->residueHelixWidth.setValue(dd.getHelixWidth());
      cdp->residueHelixOffsetWidth.setValue(dd.getHelixRadius());
      cdp->residueHeigthFactor.setValue(dd.getRibbonThickness());
      cdp->residueCylinderRadius.setValue(dd.getResidueCylinderRadius());
      cdp->schematicCylinderRadius.setValue(dd.getCartoonCylinderRadius());
      cdp->schematicTubeRadius.setValue(dd.getCartoonTubeRadius());

      // Cartoons
      cdp->schematicCylinderComplexity.setValue(dd.getCartoonCylinderRQ()/100.0);
      cdp->schematicCylinderRadius.setValue(dd.getCartoonCylinderRadius());
      //cdp->schematicArrowTesselationSegments.setValue(dd.getCartoonArrowTSegments());
      //cdp->schematicTubeTesselationSegments.setValue(dd.getCartoonTubeTSegments());
      cdp->schematicTubeRadius.setValue(dd.getCartoonTubeRadius());

      if (style == DisplayStyle::CAWIRE) {
         cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE);
      } else if (style == DisplayStyle::CYLINDER) {
         cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_CASTICK);
      } else if (style == DisplayStyle::LINE) {
         cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON);
      } else if (style == DisplayStyle::FLAT) {
         cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON);
      } else if (style == DisplayStyle::SOLID) {
         cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON);
      } else if (style == DisplayStyle::CARTOON) {
         cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC);
      } else {
         //cout << "unknown style " << endl;
      }
   }
}


/**
 * Applies the global styles defined in dd to the specified ChemDisplayParam.
 * This method also applied hardwired default that can't be controlled by
 * the user.
 */
void SGContainer::applyGlobalStyles(ChemDisplayParam *cdp)
{
   // These baseline defaults from Erich
   cdp->ballStickSphereScaleFactor.setValue(1.0);
   cdp->atomRadiiScaleFactor.setValue(0.5);

   // turn culling on - OpenMoiv feature
   cdp->clipAtoms.setValue(true);
   cdp->clipBonds.setValue(true);

   // basic stuff like fonts and colors that user can't control
   cdp->highlightStyle.setValue (ChemDisplayParam::HIGHLIGHT_DIFFUSE);
   cdp->highlightColor.setValue (SbColor(selectRGB));
   cdp->fontName.setValue("Times-Bold");
   cdp->fontSize.setValue(20);

   // This seems to look better
   cdp->residueInterpolateColor.setValue(true);

   cdp->schematicTubeShapeSegments.setValue(8);

   // TODO SHOULD THIS BE SETTABLE
   cdp->residueNumberOfThreads.setValue(5);

   cdp->residueWireframeAntiAlias.setValue(ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE);

   // TODO - we don't support turns yet
   //cdp->residueTurnWidth.setValue():

}



/**
 * Adjust location of nubs which depends on the ball/stick scale and
 * CPK radii scale factor.
 */
void SGContainer::fixAllNubs()
{
   double bs = getScaleFactor(DisplayStyle(DisplayStyle::BALLSTICK));
   double cpk = getScaleFactor(DisplayStyle(DisplayStyle::CPK));
   getFragment()->fixAllNubs(bs, cpk);
   //getFragment()->fixAllNubs(1.0, 0.5);
      
}



void SGContainer::setScaleFactor(float value)
{
   for (size_t idx=0; idx<p_displays.size(); idx++) {
      ChemDisplayParam *param = p_displayParams[idx];
      param->atomRadiiScaleFactor.setValue(value);
   }
}


/**
 * Get scale factor for specific DisplayStyle type.
 * Not sure if we should be getting by name or by basic non-residue type
 * info.  First one found wins as we assume the scales will not vary
 * across displays.
 */
double SGContainer::getScaleFactor(DisplayStyle ds) // const
{
   double value = 1.0 ;  // For CPK - from erich

   string styleName = ds.getName();
   DisplayStyle::Style style = ds.getStyle();

   if (style != DisplayStyle::CPK) {
      value = 0.5;  // default for non-cpk from Erich
   }
   for (size_t idx=0; idx<p_displayParams.size(); idx++) {
      ChemDisplayParam *cdp = p_displayParams[idx];
      if (style == toOurStyle( (ChemDisplayParam::DisplayBinding)cdp->displayStyle.getValue())) {
         value = cdp->atomRadiiScaleFactor.getValue();
         break;
      }
   }
  return value ; 
}


/**
 *  Uses scene graph traversal to find the first traverable fragment.
 */
SGFragment *SGContainer::getActiveFragment()
{
   SGFragment *frag = (SGFragment *)0;

   SoSearchAction *sa = new SoSearchAction;

   sa->setType(ChemBaseData::getClassTypeId(),true);
   sa->setInterest(SoSearchAction::FIRST);
   sa->apply(this);
   SoPath *path = sa->getPath();
   if (path != 0) {
      frag = (SGFragment*)path->getTail();
   }
   delete sa;
   return frag;

}

vector<SGFragment*> SGContainer::getAllFragments()
{
   vector<SGFragment*> ret;

   SoSearchAction *sa = new SoSearchAction;

   sa->setType(ChemBaseData::getClassTypeId(),true);
   sa->setInterest(SoSearchAction::ALL);
   sa->setSearchingAll(true);

   sa->apply(this);

   SoPathList& paths = sa->getPaths();
   for (int idx=0; idx<paths.getLength(); idx++) {
      SoPath *path = paths[idx];
      SGFragment *frag = (SGFragment*)path->getTail();
      ret.push_back(frag);
   }
   delete sa;
   return ret;

}


/**
 * Gets "the" base fragment.
 */
SGFragment *SGContainer::getFragment()
{
  SGFragment *frag = (SGFragment *)0;
  SoSeparator *node = (SoSeparator*)getCSTop();
  if (node->getNumChildren() > 0) {
    frag = (SGFragment*)node->getChild(0);
  }
  return frag;
}

const SGFragment *SGContainer::getFragment() const
{
  SGFragment *frag = (SGFragment *)0;
  if (p_chemSysTop->getNumChildren() > 0) {
    frag = (SGFragment*)p_chemSysTop->getChild(0);
  }
  return (const SGFragment*)frag;
}

void SGContainer::deleteFragment()
{
   SoSeparator *node = (SoSeparator*)getCSTop();
   node->removeChild(0);
}

void SGContainer::insertFragment(SGFragment* sgfrag)
{
   SoSeparator *node = (SoSeparator*)getCSTop();

   if (node->getNumChildren() > 1)
   {
      node->removeChild(0);
   }
   
   node->insertChild(sgfrag,0);
}

DisplayStyle SGContainer::getMainDisplayStyle() const
{
   SGFragment *frag = ((SGFragment*)getFragment()) ;
   DisplayStyle ds( frag->getMainDisplayStyle() ) ;

   return ds ;
}


ChemDisplayParam::DisplayBinding SGContainer::toChemkitStyle(DisplayStyle::Style style)
{
   ChemDisplayParam::DisplayBinding ret = ChemDisplayParam::DISPLAY_DEFAULT;
   switch (style) {
      case DisplayStyle::STICK:
        ret = ChemDisplayParam::DISPLAY_STICK;
        break;
      case DisplayStyle::BALLSTICK:
        ret = ChemDisplayParam::DISPLAY_BALLSTICK;
        break;
      case DisplayStyle::BALLWIRE:
        ret = ChemDisplayParam::DISPLAY_BALLWIRE;
        break;
      case DisplayStyle::WIRE:
        ret = ChemDisplayParam::DISPLAY_WIREFRAME;
        break;
      case DisplayStyle::DNONE:
        ret = ChemDisplayParam::DISPLAY_NONE;
        break;
      case DisplayStyle::CPK:
        ret = ChemDisplayParam::DISPLAY_CPK;
        break;
      default:
        break;
   }
   return ret;
}

DisplayStyle::Style SGContainer::toOurStyle(ChemDisplayParam::DisplayBinding style)
{
   DisplayStyle::Style ret = DisplayStyle::BALLWIRE;
   switch (style) {
      case ChemDisplayParam::DISPLAY_STICK:
        ret = DisplayStyle::STICK;
        break;
      case ChemDisplayParam::DISPLAY_BALLSTICK:
        ret = DisplayStyle::BALLSTICK;
        break;
      case ChemDisplayParam::DISPLAY_BALLWIRE:
        ret = DisplayStyle::BALLWIRE;
        break;
      case ChemDisplayParam::DISPLAY_WIREFRAME:
        ret = DisplayStyle::WIRE;
        break;
      case ChemDisplayParam::DISPLAY_NONE:
        ret = DisplayStyle::DNONE;
        break;
      case ChemDisplayParam::DISPLAY_CPK:
        ret = DisplayStyle::CPK;
        break;
   }
   return ret;
}


/**
 * Generate a path to the node with the specified display style index.
 */
SoPath *SGContainer::getSelectionPath(ChemSelection * esel, int index)
{
   SoSearchAction *getCD = new SoSearchAction;
   getCD->setInterest(SoSearchAction::FIRST);
   getCD->setNode(p_displays[index]);

   getCD->setFind(SoSearchAction::NODE);
   getCD->apply(esel);
   SoPath * pathCD =   getCD->getPath();
   return pathCD; 
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
ChemColor *SGContainer::getColorNode(void)
{
   return p_colors;
}

/**
 * Gets main radii node.
 * This node is used/referenced for all by CPK.
 */
ChemRadii *SGContainer::getRadiiNode()
{
   return p_radii;
}

ChemRadii *SGContainer::getCPKRadiiNode()
{
   return p_CPK_radii;
}

/**
 * Get ChemDisplayParam at specified index.
 * See getNumDisplayStyles().
 */
ChemDisplayParam *SGContainer::getChemDisplayParam(int index)
{
   return p_displayParams[index];
}


/**
 * Turn atom labels on or off for all display styles.
 * @todo control labels by style or selection or ??
 */
void SGContainer::setAtomLabels(int on_off)
{
   for (size_t idx=0; idx<p_displays.size(); idx++) {
      ChemDisplayParam *param = p_displayParams[idx];
      param->showAtomLabels.setValue(on_off);
   }
}

/**
 * Turn bond labels on or off for all display styles.
 * @todo control labels by style or selection or ??
 */
void SGContainer::setBondLabels(int on_off)
{
   for (size_t idx=0; idx<p_displays.size(); idx++) {
      ChemDisplayParam *param = p_displayParams[idx];
      param->showBondLabels.setValue(on_off);
   }
}


/**
 * Turn residues labels on or off for all display styles.
 * @todo control labels by style or selection or ??
 */
void SGContainer::setResidueLabels(int on_off)
{
   for (size_t idx=0; idx<p_displays.size(); idx++) {
      ChemDisplayParam *param = p_displayParams[idx];
      param->showResidueLabels.setValue(on_off);
   }
}



/**
 * Set depth cueing for all styles.
 * @todo control per style?
 */
void SGContainer::setDepthCue(int flag, float r, float g, float b)
{
   for (size_t idx=0; idx<p_displays.size(); idx++) {
      ChemDisplayParam *param = p_displayParams[idx];
      if (flag) {
         param->fogType.setValue(ChemDisplayParam::FOG_HAZE);
         param->fogColor.setValue((float)r,(float)g,(float)b);
      } else {
         param->fogType.setValue(ChemDisplayParam::FOG_NONE);
      }
   }

}


void SGContainer::setShowHydrogens(int on_off)
{
   for (size_t idx=0; idx<p_displays.size(); idx++) {
      ChemDisplayParam *param = p_displayParams[idx];
      param->showHydrogens.setValue(on_off);
   }
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
SoMaterial *SGContainer::getMatNode()
{
   return p_cdMat;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
CustomBBoxNode *SGContainer::getViewBBox()
{
   return p_viewBbox;
}

SoGroup *SGContainer::getTopManip()
{
   return p_topManip;
}

SoSwitch * SGContainer::getcsSwitch()
{
   return p_csSwitch;
}

SoSeparator * SGContainer::getCSTop()
{
   return p_chemSysTop;
}

SoSeparator * SGContainer::getMainSep()
{
   return p_mainSep ;
}

SoSwitch * SGContainer::getTopSepLabel()
{
   return p_topSepLabel;
}

/**
 * Return SoGroup that holds the atom to atom lines a user can draw
 */
SoGroup * SGContainer::getTopLine()
{
   return p_topLine;
}


/**
 * Return SoGroup that holds the lines that represent H bonds
 */
SoGroup * SGContainer::getTopHBonds()
{
   // Child 0 holds the style settings
   return (SoGroup *)p_topHBonds->getChild(1);
}

SoSwitch *SGContainer::getTopAtomVectors()
{
   // Child 0 holds the style settings
   return p_topAtomVectors;
}

SoGroup * SGContainer::getTopRotManip()
{
   return p_topRotManip;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
SoGroup * SGContainer::getTopLabel()
{
   return p_topLabel;
}


/**
 * Iterate through existing display styles and recompute the colors
 * for color schemes that vary based on chemical system makeup.
 */
void SGContainer::updateColorNodes()
{
   for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
      DisplayDescriptor dd = p_displayDescs[idx];

      // Assumption:
      // The ChemColor object is the first node under the group for this
      // particular display style.  Each style does not necessarily have
      // a ChemColor but for the types we care about, it should exist.
      SoGroup *stylegroup = (SoGroup*)p_dynamicStyles->getChild(idx);

      ChemColor *cc = (ChemColor*)stylegroup->getChild(0);
      if (dd.getColorScheme() == "Atom Index") {
         createAtomIndexColors(dd.getSpectrumType(), cc);
      } else if (dd.getColorScheme() == "Atom Charge") {
         createAtomChargeColors(dd.getSpectrumType(), cc);
      } else if (dd.getColorScheme() == "Position from CM") {
         DisplayStyle::Style style = DisplayStyle::toStyle(dd.getStyle());
         if (style >= DisplayStyle::CAWIRE) {
            createResidueCenterMassColors(dd.getSpectrumType(), cc);
         } else {
            createCenterMassColors(dd.getSpectrumType(), cc);
         }
      } else if (dd.getColorScheme() == "Residue Index") {
         createResidueIndexColors(dd.getSpectrumType(), cc, dd.getStyle());
      } else if (dd.getColorScheme() == "Residue Name") {
         createResidueNameColors(dd.getSpectrumType(), cc);
      } else if (dd.getColorScheme() == "Hydrophobicity") {
         createHydrophobicityColors(dd.get3ColorStyle(),cc);
      } else if (dd.getColorScheme() == "Secondary Structure") {
         createSecondaryStructureColors(dd.get3ColorStyle(),cc);
      } else if (dd.getColorScheme() == "Chains") {
         createChainColors(dd.getSpectrumType(), cc);
      } else if (dd.getColorScheme() == "Residue Charge") {
         createResidueChargeColors(dd.get3ColorStyle(), cc);
      }
   }
}


/**
 * recomputes the fragment center for use by the lattice and
 * sets the lattice to force a redraw.
 */
void SGContainer::touchLattice()
{
   SGFragment* frag = getFragment() ;
   LatticeDef *lattice = frag->getLattice();
   SGLattice *sgl = (SGLattice*)findNode("lattice");
   if (frag && lattice && sgl) {

      int na1, na2, na3;
      lattice->getReplicationFactors(na1, na2, na3);
      if (na1 == 1 && na2 == 1 && na3 == 1) {
         int natoms = frag->numAtoms();
         double cmx = 0.0;
         double cmy = 0.0;
         double cmz = 0.0;
         double mtot = 0.0;
         const double *coords;
         double weight = 1.0;
         TAtm* atom;
         for (int i=0; i<natoms; i++) {
            atom = frag->atomRef(i);
            if (atom->atomicNumber()>0) {
               mtot += weight; // all equal weight
               coords = atom->coordinates();
               cmx += weight*coords[0];
               cmy += weight*coords[1];
               cmz += weight*coords[2];
            }
         }
         cmx = cmx/mtot;
         cmy = cmy/mtot;
         cmz = cmz/mtot;
         lattice->setFragCenter(cmx,cmy,cmz);
      }
      // Could just do sgl->touch() if we overload to redo setLattice??
      sgl->setLattice(lattice);

   }
}

/**
 * Update mapping of atoms to ChemDisplay nodes.
 */
void  SGContainer::touchChemDisplay()
{
   SGFragment* frag = getFragment() ;
   touchChemDisplay(frag);
}


/**
 * Update mapping of atoms to ChemDisplay nodes.
 * Each atom and bond has a DisplayStyle with a name that should match 
 * one of the display group names.
 */
void  SGContainer::touchChemDisplay(SGFragment* frag)
{
#ifdef timertests
  ETimer t,T;
  t.start();
#endif

#ifdef timertests
  T.start();
#endif
  // Re-initialize all displays to no atoms/bonds
  for (size_t ddx=0; ddx<p_displays.size(); ddx++) {
      ChemDisplay *cd = p_displays[ddx];
      cd->atomIndex.setNum(0);
      cd->bondIndex.setNum(0);
      cd->atomLabelIndex.setNum(0);
      cd->bondLabelIndex.setNum(0);
      // Initializing residues here breaks cartoon style.
      // See residue section below.
  }
#ifdef timertests
  T.stop();
  cout << T.elapsedTime() << " " << t.CPUTime() << " Re-initialize all displays to no atoms/bonds" << endl;
#endif

   int k;

   // Need this to get the actual SbResidues which are not necessarily
   // the same as fragment residues for performance reasons.
   //ChemData *moiv = (ChemData*)frag;

   int numbatoms = frag->getNumberOfAtoms();
   int numbbonds = frag->numBonds();
   int nResidues = frag->numResidues();

   DisplayStyle style;
   string name;
   int numDisplays = p_displays.size();

   // Note that we tried to have code that just did:
   //   chemDisplay->atomIndex.set1Value
   //     (0,SbVec2i(0,CHEM_DISPLAY_USE_REST_OF_ATOMS));
   //   chemDisplay->bondIndex.set1Value
   //     (0,SbVec2i(0,CHEM_DISPLAY_USE_REST_OF_BONDS));
   // if there were no atoms or bonds but this caused problems when we
   // tried to go from 2 display styles down to one by deleting all atoms
   // of a given display style.


   //////////////////////////////////////////////////////////////////////
   // Atoms
   //////////////////////////////////////////////////////////////////////
#ifdef timertests
  T.start();
#endif
   if (numbatoms > 0) {
      MFVec2i *cdAtomIndices = new MFVec2i[numDisplays]; 
      for (int idx=0; idx<numDisplays; idx++) {
         cdAtomIndices[idx].setNum(0);
      }
      for (k = 0; k < numbatoms; k++ ) {
         TAtm *atm = frag->atomRef(k);
         style = atm->displayStyle();
         name = style.getName();
         //cout << "atom style name = " << name << endl;

         for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
            if (name == p_displayDescs[idx].getName() && style.isDisplayed()) {
               //cout << "found a matched style name " << cdAtomIndices[idx].getNum() << endl;;
               cdAtomIndices[idx].set1Value(cdAtomIndices[idx].getNum(),k,1);
               //cout << " " << cdAtomIndices[idx].getNum() << endl;
               break;
            }
         }
      }

      for (size_t idx=0; idx<p_displayParams.size(); idx++) {
         p_displays[idx]->atomIndex.setNum(cdAtomIndices[idx].getNum());
         p_displays[idx]->atomIndex = cdAtomIndices[idx];
         p_displays[idx]->atomLabelIndex = cdAtomIndices[idx];
//         cout << "Atom Display " << p_displayDescs[idx].getName() << " " << cdAtomIndices[idx].getNum() << endl;
      }
      delete [] cdAtomIndices;
   }
#ifdef timertests
  T.stop();
  cout << T.elapsedTime() << " " << t.CPUTime() << " Atoms" << endl;
#endif


   //////////////////////////////////////////////////////////////////////
   // Bonds
   //////////////////////////////////////////////////////////////////////
#ifdef timertests
  T.start();
#endif
   if (numbbonds > 0) {
      MFVec2i *cdBondIndices = new MFVec2i[numDisplays]; 
      for (int idx=0; idx<numDisplays; idx++) {
         cdBondIndices[idx].setNum(0);
      }
      for (k = 0; k < numbbonds; k++ )
      {
         style = frag->getBondStyle(k);
         name = style.getName();
         //cout << "bond style name = " << name << endl;
         for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
            if (name == p_displayDescs[idx].getName() && style.isDisplayed()) {
               //cout << "found a matched style name " << cdBondIndices[idx].getNum();
               cdBondIndices[idx].set1Value(cdBondIndices[idx].getNum(),k,1);
               //cout << " " << cdBondIndices[idx].getNum() << endl;
               break;
            }
         }
      }

      for (size_t idx=0; idx<p_displayParams.size(); idx++) {
//         cout << "Bond Display " << p_displayDescs[idx].getName() << " " << cdBondIndices[idx].getNum() << endl;
         p_displays[idx]->bondIndex.setNum(cdBondIndices[idx].getNum());
         p_displays[idx]->bondIndex = cdBondIndices[idx];
         p_displays[idx]->bondLabelIndex = cdBondIndices[idx];
      }
      delete [] cdBondIndices;
   }
#ifdef timertests
  T.stop();
  cout << T.elapsedTime() << " " << t.CPUTime() << " Bonds" << endl;
#endif


   //////////////////////////////////////////////////////////////////////
   // Residues
   // NOTE: Executing this code when Cartoon is desired causes nothing
   // to be displayed.  Instead of fixing, for now, just make sure it
   // doesn't happen.  That is, if they want cartoon, its all they can
   // have for now.
   // MUST execute event if nResidues == 0
   //////////////////////////////////////////////////////////////////////
#ifdef timertests
  T.start();
#endif
   if (!(numDisplays == 1 && p_displayDescs[0].getStyle() == "Cartoon") &&
       nResidues > 0) {
      MFVec2i *cdResidueIndices = new MFVec2i[numDisplays]; 
      for (int idx=0; idx<numDisplays; idx++) {
         cdResidueIndices[idx].setNum(0);
         p_displays[idx]->residueIndex.setNum(0);
//         p_displays[idx]->residueLabelIndex.setNum(0);
      }
      for (k = 0; k < nResidues; k++ )
      {
         Residue *residue = frag->residue(k);
         style = residue->displayStyle();
         name = style.getName();
         for (size_t idx=0; idx<p_displayDescs.size(); idx++) {
            //cout << name << "==" << p_displayDescs[idx].getName() << endl;
            if (name == p_displayDescs[idx].getName()) {
               //cout << "found a matched residue style name: " << name << " " << cdResidueIndices[idx].getNum() << endl;
               cdResidueIndices[idx].set1Value(cdResidueIndices[idx].getNum(),k,1);
               break;
            }
         }
      }

      for (size_t idx=0; idx<p_displayParams.size(); idx++) {
//         cout << "Residue Display " << p_displayDescs[idx].getName() << " " << cdResidueIndices[idx].getNum() << endl;
         p_displays[idx]->residueIndex.setNum(cdResidueIndices[idx].getNum());
         p_displays[idx]->residueIndex = cdResidueIndices[idx];
//         p_displays[idx]->residueLabelIndex = cdResidueIndices[idx];
      }
      delete [] cdResidueIndices;
   }
#ifdef timertests
  T.stop();
  cout << T.elapsedTime() << " " << t.CPUTime() << " Residues" << endl;
#endif

   removeUnusedStyles();

#ifdef timertests
   t.stop();
   cout << t.elapsedTime() << " " << t.CPUTime() << " touchChemDisplay" << endl;
#endif
}


/**
 * Remove unused display styles from scene graph.
 * If there are no atoms, and thus all styles are unused, the first
 * style will not be removed.
 */
void SGContainer::removeUnusedStyles()
{
#ifdef timertests
  ETimer t;
  t.start();
#endif
   // Remove empty styles.
   // doesn't currently account for residues
   if (p_displays.size() > 1) {
      vector<ChemDisplay*>::iterator cdit = p_displays.begin();
      vector<ChemDisplayParam*>::iterator cdpit = p_displayParams.begin();
      vector<DisplayDescriptor>::iterator cdn = p_displayDescs.begin();
      for (int idx=p_displayParams.size()-1; idx>=0; idx--) {
         if (p_displays[idx]->atomIndex.getNum() == 0 &&
               p_displays[idx]->bondIndex.getNum() == 0 && 
               p_displays[idx]->residueIndex.getNum() == 0) {
            // Remove Node from SceneGraph
            p_dynamicStyles->removeChild(idx);

            // Remove from internal arrays
            p_displays.erase(cdit + idx);
            p_displayDescs.erase(cdn + idx);
            p_displayParams.erase(cdpit + idx);
         }
      }
   }
#ifdef timertests
   t.stop();
   cout << t.elapsedTime() << " " << t.CPUTime() << " removeUnusedStyles" << endl;
#endif
}



/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
SoGroup *SGContainer::getDisplayGroup()
{
   return p_display;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
SoShapeHints * SGContainer::getShapeHints()
{
  return p_shapeHints;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
SoGroup* SGContainer::getTopClipManip()
{
   return p_topClipManip ;
}


/**
 * Set sphere options (user preferences)
 * Currently applies to all ChemDisplayParam - except in one special case
 * where parameters have different meaning for cpk.
 */
void SGContainer::setSphereOptions(const string& sphere, 
      double complexity, double scale)
{
   int numDisplays = getNumDisplayStyles();
   for (int idx=0; idx<numDisplays; idx++) {
      ChemDisplayParam *cdp = getChemDisplayParam(idx);

      if (sphere == "hemisphere") {
         cdp->atomSphereDisplayStyle.setValue
            (ChemDisplayParam::ATOMSPHERE_HEMISPHERES);
      } else {
         cdp->atomSphereDisplayStyle.setValue
            (ChemDisplayParam::ATOMSPHERE_FULLSPHERES);
      }

      cdp->atomSphereComplexity.setValue(complexity);

      if (cdp->displayStyle.getValue() != ChemDisplayParam::DISPLAY_CPK) {
         cdp->atomRadiiScaleFactor.setValue(scale);
      }

      getFragment()->fixAllNubs
         (scale,getScaleFactor(DisplayStyle(DisplayStyle::CPK)) ) ;
      // cpk radii doesn't change
   }
}


/**
 * Set stick display options (user preferences)
 * Currently applies to all ChemDisplayParam 
 */
void SGContainer::setStickOptions(const string& cap, 
      double complexity, double radius)
{
   int numDisplays = getNumDisplayStyles();
   for (int idx=0; idx<numDisplays; idx++) {
      ChemDisplayParam *cdp = getChemDisplayParam(idx);
      if (cap == "none") {
         cdp->bondCylinderDisplayStyle.setValue
            (ChemDisplayParam::BONDCYLINDER_NOCAP);
      } else if (cap == "round") {
         cdp->bondCylinderDisplayStyle.setValue
            (ChemDisplayParam::BONDCYLINDER_ROUNDCAP);
      } else {
         cdp->bondCylinderDisplayStyle.setValue
            (ChemDisplayParam::BONDCYLINDER_FLATCAP);
      }
      cdp->bondCylinderComplexity.setValue(complexity);
      cdp->bondCylinderRadius.setValue(radius);
   }
}

/**
 * Set wireframe display options (user preferences)
 * Currently applies to all ChemDisplayParam 
 */
void SGContainer::setWireframeOptions( double width)
{
   int numDisplays = getNumDisplayStyles();
   for (int idx=0; idx<numDisplays; idx++) {
      ChemDisplayParam *cdp = getChemDisplayParam(idx);
      cdp->bondWireframeLineWidth.setValue(width);
   }
}


void SGContainer::setShowRings(bool showRings) 
{
   p_showRings = showRings;
   int numDisplays = getNumDisplayStyles();
   for (int idx=0; idx<numDisplays; idx++) {
      ChemDisplayParam *cdp = getChemDisplayParam(idx);
      cdp->setShowRings(showRings);
   }
}

bool SGContainer::getShowRings() const
{
   return p_showRings;
}

/** 
 * Marks all atoms/bonds as selected.
 * This information must be passed back to the scenegraph to be seen.
 */
void SGContainer::selectAll()
{
   SGFragment *frag = getFragment();

   vector<int> *atomHighLight = &(frag->m_atomHighLight);
   vector<int> *bondHighLight = &(frag->m_bondHighLight);


   // Clear any existing values so we don't get duplicates
   atomHighLight->clear();
   bondHighLight->clear();

   int idx;
   int numAtoms = frag->numAtoms();
   vector<TAtm*> *atoms = frag->atoms();
   TAtm *atom;
   for (idx=0; idx<numAtoms; idx++) {
      atom = (*atoms)[idx];
      if (atom->displayStyle().isDisplayed()) {
         atomHighLight->push_back(idx);
      }
   }
   delete atoms;


   int numBonds = frag->numBonds();
   // Used cached bond list for speed
   // Note that this is a ChemBaseData virtual so we have to stick with an int
   for (idx=0; idx<numBonds; idx++) {
      if (frag->getBondStyle(idx).getStyle() != (int)DisplayStyle::DNONE) {
         bondHighLight->push_back(idx);
      }
   }

}


/** 
 * Clears selection bit vectors.
 * This information must be passed back to the scenegraph to be seen.
 */
void SGContainer::unselectAll()
{
   SGFragment *frag = getFragment();
   frag->m_atomHighLight.clear();
   frag->m_bondHighLight.clear();

}


bool SGContainer::setStyle(const DisplayDescriptor& dd, 
      bool all, 
      SGFragment *frag)
{
   //bool ret = true;

   DisplayStyle newStyle ;    // default is ball and wire
   newStyle.setDisplayed(true);

   // This is a temporary hack because we can't currently support mixing
   // cartoon on just some of the atoms.
   // Part two of hack at end of method.
   if (dd.getStyle() == "Cartoon") {
      all = true;
   }


   // what's our display style?
   newStyle.setStyle(DisplayStyle::toStyle(dd.getStyle()));
   if (dd.getStyle() == "Invisible")
   {
      newStyle.setDisplayed(false);
   }

   bool changed = false ;

   // Set up the new style
   newStyle.setName(dd.getName());
   if (newStyle.getStyle() >= DisplayStyle::CAWIRE) {
      // make atoms hide if doing residues
      newStyle.setDisplayed(false); 
   }

   bool exists = styleExists(dd.getName());

   if (all) {
      changed = changeStyleAll( newStyle, frag );
      //   if (newStyle.getStyle() >= DisplayStyle::CAWIRE) {
      changed |= changeStyleAllResidues(newStyle, frag);
      //   }
      if (exists) {
         changeStyle(dd);
      } else {
         resetToStyle(dd);
      }
   } else {
      changed = changeStyleSelected( newStyle, frag );
      //   if (newStyle.getStyle() >= DisplayStyle::CAWIRE) {
      changed |= changeStyleSelectedResidues( newStyle, frag );
      //   }
      if (exists) {
         changeStyle(dd);
      } else {
         createStyle(dd);
      }
   }

   // Part 2 of hack.  Do after creating new style otherwise
   if (dd.getStyle() == "Cartoon") {
      // Remove existing so hack in SGContainer::touchChemDisplay works
      vector<string> styles = getDisplayStyleNames();
      int numStyles = styles.size();
      for (int idx=numStyles-1; idx>=0; idx--) {
         if (styles[idx] != dd.getName()) {
            removeStyle(styles[idx]);
         }
      }
   }
   return changed;
}


bool SGContainer::changeStyleSelectedResidues(const DisplayStyle& newstyle, SGFragment* frag)
{
   bool changed = false; 

   // Set the return value
   if ( frag->m_atomHighLight.size() > 0) changed = true;

   TAtm* atmptr ;
   //int ai;
   DisplayStyle curStyle;
   DisplayStyle bondStyle;

   const vector<int>& atomList = frag->m_atomHighLight;

   int numResidues = frag->numResidues();
   vector<bool> selResidues(numResidues, false);

#if 111
   // Initialize a map to 10% larger than numResidues  - just a guess
   hash_map<unsigned long, Residue*, hash<unsigned long>, equint> mymap((int)(numResidues * 1.1));

   int selAtoms = atomList.size();
   for (int ai= 0; ai< selAtoms; ai++)
   {
      atmptr = frag->atomRef(atomList[ai]) ;
      Residue *res = atmptr->getResidue();
      if (res && mymap.find((unsigned long)res) == mymap.end() ) {
         // Never got the code for indexed residues commited - use slow way now
         int num = ((Fragment*)frag)->getResidueIndex(res);
         //int num = res->index();
         selResidues[num] = true;
         mymap[(unsigned long)res] = res;
      }
   }
#else
   // Calling getResidueIndex is very slow.....
   // But this old code is here for posterity

   // Figure out which residues are selected based on which atoms
   // are selected.  If any atom in a residue is selected, the
   // residue is considered selected.
   for ( ai= 0; ai< atomList.size(); ai++)
   {
      atmptr = frag->atomRef(atomList[ai]) ;
      Residue *res = atmptr->getResidue();
      if (res) {
         int num = ((Fragment*)frag)->getResidueIndex(res);
         selResidues[num] = true;
      }
   }
#endif

   for (size_t idx=0; idx<selResidues.size(); idx++) {
      if (selResidues[idx]) {
         Residue *residue = frag->residue(idx);
         // Don't change the style if the thing is currently not displayed.
         // That way it will come back in its current style.
         curStyle = residue->displayStyle();
         curStyle.setDisplayed(newstyle.isDisplayed());
         curStyle.setStyle(newstyle.getStyle());
         curStyle.setName(newstyle.getName());
         residue->displayStyle(curStyle) ;
      }
   }
   return changed;
}



/**
 * Change the display style of all selected atoms and bonds.
 */
bool SGContainer::changeStyleSelected(const DisplayStyle& newstyle, SGFragment* frag)
{
   bool changed = false ;

   vector<int> atomIndexList;
   vector<int> bondIndexList;


   // get list of atoms and bonds that are highlighted
   if ( frag->m_atomHighLight.size() > 0 ||
         frag->m_bondHighLight.size() > 0 )
   {
      atomIndexList = frag->m_atomHighLight ;
      bondIndexList = frag->m_bondHighLight ;
      changed = true ;
   }

   TAtm* atmptr ;
   TBond* bondptr ;
   DisplayStyle curStyle;

   // set the display style for each TAtm.
   for (size_t ai= 0; ai< atomIndexList.size(); ai++)
   {
      atmptr = frag->atomRef(atomIndexList[ai]) ;

      // ok... this is a clear hack.  When a CPK atom is selected using
      // the lasso, the bonds are not selected and then never have their
      // display style changed.  This loop changes the display style on
      // any bonds associated with a CPK atom.
      curStyle = atmptr->displayStyle();
      if (curStyle.getStyle() == DisplayStyle::CPK) 
      {
         for (size_t cb = 0; cb < atmptr->bondList().size(); cb++)
         {
            bondptr = atmptr->bondList()[cb] ;
            bondptr->displayStyle(newstyle);
         }
      }

      curStyle.setDisplayed(newstyle.isDisplayed());
      if (newstyle.isDisplayed()) 
      {
         curStyle.setStyle(newstyle.getStyle());
         curStyle.setName(newstyle.getName());
      }
      atmptr->displayStyle(curStyle) ;

   }

   vector<TBond*>* allbonds = frag->bonds() ;
   for (size_t ai= 0; ai< bondIndexList.size(); ai++)
   {
      bondptr = (*allbonds)[bondIndexList[ai]] ;
      curStyle = bondptr->displayStyle();
      curStyle.setDisplayed(newstyle.isDisplayed());
      if (newstyle.isDisplayed())
      {
         curStyle.setStyle(newstyle.getStyle());
         curStyle.setName(newstyle.getName());
      }
      bondptr->displayStyle(curStyle) ;
   }
   delete allbonds;

   return changed ;
}



/**
 * Change all atoms, whether highlighted or not, to the new display
 * style.  And change the main display style.
 */
bool SGContainer::changeStyleAll(const DisplayStyle& newstyle, SGFragment* frag)
{
  bool changed = false ;

  TAtm* atmptr ;
  TBond* bondptr ;
  int ai;
  DisplayStyle curStyle;

  // set the display style for each TAtm.
  int num_atoms = frag->numAtoms();
  for ( ai= 0; ai< num_atoms; ai++)
  {
     atmptr = frag->atomRef(ai) ;

     // Don't change the style if the thing is currently not displayed.
     // That way it will come back in its current style.
     curStyle = atmptr->displayStyle();
     curStyle.setDisplayed(newstyle.isDisplayed());
     if (newstyle.isDisplayed())
     {
       curStyle.setStyle(newstyle.getStyle());
       curStyle.setName(newstyle.getName());
     }
     atmptr->displayStyle(curStyle) ;

  }

  // set the display style for each TBond.
  vector<TBond*>* allbonds = frag->bonds() ;
  int num_bonds = allbonds->size();
  for ( ai= 0; ai< num_bonds; ai++)
  {
     bondptr = (*allbonds)[ai] ;
     curStyle = bondptr->displayStyle();
     curStyle.setDisplayed(newstyle.isDisplayed());
     if (newstyle.isDisplayed())
     {
       curStyle.setStyle(newstyle.getStyle());
       curStyle.setName(newstyle.getName());
     }
     bondptr->displayStyle(curStyle) ;
  }
  delete allbonds;

  if (num_atoms > 0 || num_bonds > 0) changed = true;

  // set main display style
  frag->setMainDisplayStyle(newstyle) ;


  return changed ;
}


/**
 * Change all residues to the new style.
 */
bool SGContainer::changeStyleAllResidues( const DisplayStyle& newstyle, SGFragment* frag)
{
   int nResidues = frag->numResidues();
   DisplayStyle curStyle;
   for ( int rdx= 0; rdx<nResidues; rdx++) {
      Residue *residue = frag->residue(rdx) ;

      // Don't change the style if the thing is currently not displayed.
      // That way it will come back in its current style.
      curStyle = residue->displayStyle();
      curStyle.setDisplayed(newstyle.isDisplayed());
      curStyle.setStyle(newstyle.getStyle());
      curStyle.setName(newstyle.getName());
      residue->displayStyle(curStyle) ;

   }
   return nResidues > 0;

}

/**
 * Description
 *   Returns true if any nubs were changed to Hydrogens
 *
 *   Currently we just use 1.0 angstrom for all the bond lengths.  I
 *   asked Erich about this:  
 *
 *     "Regarding the H bond lengths of 1.0 angstroms, I don't think it 
 *      matters.  They are usually about that for organic molecules
 *      anyway.  It might make a difference with metal hydrogen bonds,
 *      but I don't think it's worth spending any time on it.
 *      If its easy, do it!"
 */
bool SGContainer::addHydrogens(SGFragment *frag, bool all,
                                     bool checkStatus) 
{
  TPerTab tpt;
  bool ret = false;
  vector <TAtm*> *list;
  TAtm *parent, *atm, *catm;

  float crH = tpt.covalentRadius(1); // Hydrogen
  float radii;
  int nSel = frag->m_atomHighLight.size();
  int isize;
//
//  Some atoms selected, only add hydrogens to selected atoms
//
  if (nSel > 0 && all == false) {
    for (int idx=0; idx<nSel; idx++) {
      atm = frag->atomRef(frag->m_atomHighLight[idx]);
//
// Nub is selected, change it to hydrogen
//
      if (atm->atomicSymbol() == "Nub") {
        parent = frag->nubParent(atm);
        radii = crH +
           tpt.covalentRadius(tpt.atomicNumber(parent->atomicSymbol()));
        changeTo(atm,frag->nubParent(atm),"H",radii);
        atm->atomName(atm->getHydrogenName().c_str());
        ret = true;
//
// atom is selected, change all its nubs to hydrogens
//
      } else {
        list = atm->connectedAtoms();
        isize = list->size();
        for (int ic=0; ic < isize; ic++) {
          catm = (*list)[ic];
          if (catm->atomicSymbol() == "Nub") {
            parent = frag->nubParent(catm);
            radii = crH +
               tpt.covalentRadius(tpt.atomicNumber(parent->atomicSymbol()));
            changeTo(catm,frag->nubParent(catm),"H",radii);
            catm->atomName(catm->getHydrogenName().c_str());
            ret = true;
          }
        }
        delete list;
      }
    }
//
//  No atoms selected, default is to change all nubs to hydrogens
//
  } else {
    vector<TAtm*> *atoms = frag->atoms();
    int numAtoms = atoms->size();
    MPoint parentXYZ,xyz;
    bool addH;
    for (int idx=0; idx<numAtoms; idx++) {
      atm = (*atoms)[idx];
      if (checkStatus) {
        if (atm->getResidue()->residueStatus() == Residue::GENERATABLE) {
          addH = true;
        } else {
          addH = false;
        }
      } else {
        addH = true;
      }
      if (atm->atomicSymbol() == "Nub" && addH) {
        parent = frag->nubParent(atm);
        radii = crH +
           tpt.covalentRadius(tpt.atomicNumber(parent->atomicSymbol()));
        changeTo(atm,frag->nubParent(atm),"H",radii);
        atm->atomName(atm->getHydrogenName().c_str());
        ret = true;
      }
    }
    delete atoms;
  }


  return ret;
}

/**
 * Description
 *   Remove H atoms that are bonded to only one thing and replace them
 *   with Nub atoms.
 *
 *   We hardwire 0.5 angstrom but we need to determine this such
 *   that the nub is 1/2 out of its parent.  We need the scale
 *   factor.
 */
bool SGContainer::removeHydrogens(SGFragment *frag, bool all) 
{
  bool ret = false;

  TAtm *curAtm, *batm;
  vector<TAtm*> *blist;

  int nSel = frag->m_atomHighLight.size();
  if (nSel > 0 && all == false) {
    for (int idx=0; idx<nSel; idx++) {
      curAtm = frag->atomRef(frag->m_atomHighLight[idx]);

      if (curAtm->atomicSymbol() == "H") {
        blist = curAtm->connectedAtoms();
        if (blist->size() > 0) {
          // Just assume there is one parent at index 0
          batm = (*blist)[0];
          if (blist->size() == 1 && batm->atomicSymbol() == "Nub") {
            // skip it
          } else {
            changeTo(curAtm,frag->nubParent(curAtm),"Nub",0.5);
            ret = true;
            frag->fixNub(curAtm,getScaleFactor(curAtm->displayStyle()));
          }
        }
        delete blist;
      } else {
        // Remove H connected to selected atom.  This code was missing
        // but it makes sense to me.
        blist = curAtm->connectedAtoms();
        for (size_t bdx=0; bdx<blist->size(); bdx++) {
          batm = (*blist)[bdx];
          if (batm->atomicSymbol() == "H") {
            changeTo(batm,curAtm,"Nub",
                  getScaleFactor(batm->displayStyle()));
            ret = true;

            frag->fixNub(batm,getScaleFactor(batm->displayStyle()));
          }
        }
        delete blist;
      }
    }
  } else {

    vector<TAtm*> *atoms = frag->atoms();
    int numAtoms = atoms->size();

    for (int idx=0; idx<numAtoms; idx++) {

      curAtm = (*atoms)[idx];
      if (curAtm->atomicSymbol() == "H") {
        blist = curAtm->connectedAtoms();
        if (blist->size() > 0) {
          // Just assume there is one parent at index 0
          batm = (*blist)[0];
          if (blist->size() == 1 && batm->atomicSymbol() == "Nub") {
            // skip it
          } else {
            changeTo(curAtm,frag->nubParent(curAtm),"Nub",
                  getScaleFactor(curAtm->displayStyle()));
            ret = true;

            frag->fixNub(curAtm,getScaleFactor(curAtm->displayStyle()));

          }
        }
        delete blist;
      }
    }
    delete atoms;
  }
  return ret;
}

void SGContainer::changeTo(TAtm *atm, TAtm *parent, const char *tag, float scale)
{
  TPerTab tpt;
  MPoint parentXYZ,xyz;
  double coords[3];

  parentXYZ.xyz(parent->coordinates());
  xyz.xyz(atm->coordinates());
  xyz.subtract(parentXYZ);
  xyz.normalize();
  xyz.scale(scale);
  xyz.add(parentXYZ);
  coords[0] = xyz.x(); 
  coords[1] = xyz.y();
  coords[2] = xyz.z();
  atm->coordinates(coords);
  atm->atomicSymbol(tag);
  atm->atomicNumber(tpt.atomicNumber(tag));

}




/**
 * Updates locations of non-atom object that are associated with the
 * location of atoms that may have moved.
 * (examples: measures, manipulators...)
 */
void SGContainer::adjustAtomContainers()
{
   MoveAction act;
   act.apply(getTopRotManip());
   act.apply(getTopManip());
   act.apply(getTopLabel());
   act.apply(getTopLine());
}
void SGContainer::removeMeasures()
{
   // remove the measures the chemsys uses
   SGContainer::getTopLabel()->removeAllChildren();

}

//
// when the property changes, the measures have to be updated.  This
// replaces the fragment under topLabel so the measure can adjust to the
// new coordinates
//
void SGContainer::updateMeasures()
{
   SoGroup* chemSysTopLabel = getTopLabel();
   SGFragment* frag = getFragment() ;

   //if (frag->numAtoms() != 0)
   if (frag)
   {
      int numChildren = chemSysTopLabel->getNumChildren();

      for ( int m = 0; m < numChildren; m++)
      {
         // get measure node
         SoNode* measureNode = ((SoSeparator*)chemSysTopLabel->
               getChild(m))->getChild(0);

         // cast each node and replace the fragment with the current
         // property fragment
         if (measureNode->isOfType(AtomMeasureAngle::getClassTypeId()) )
         {
            AtomMeasureAngle* smt = (AtomMeasureAngle*)measureNode ;
            smt->replaceSGFragment(frag) ;
         }
         if (measureNode->isOfType(AtomMeasureTorsion::getClassTypeId()) )
         {
            AtomMeasureTorsion* smp = (AtomMeasureTorsion*)measureNode ;
            smp->replaceSGFragment(frag) ;
         }
         if (measureNode->isOfType(AtomMeasureDist::getClassTypeId()) )
         {
            AtomMeasureDist* smd = (AtomMeasureDist*)measureNode ;
            smd->replaceSGFragment(frag) ;
         }

      }
   }
}

//
// Explicitly replace the measure when the data changes.  Used in the
// CmdStep commands
//
void SGContainer::adjustMeasures()
{
   SoGroup* chemSysTopLabel = getTopLabel();
   SGFragment* frag = getFragment() ;

   if (frag->numAtoms() != 0)
   {
      int numChildren = chemSysTopLabel->getNumChildren();

      for ( int m = 0; m < numChildren; m++)
      {
         // get measure node
         SoNode* measureNode = ((SoSeparator*)chemSysTopLabel->
               getChild(m))->getChild(0);

         // cast each node and replace the fragment with the current
         // property fragment
         if (measureNode->isOfType(AtomMeasureAngle::getClassTypeId()) )
         {
            AtomMeasureAngle* smt = (AtomMeasureAngle*)measureNode ;
            smt->jump();
         }
         if (measureNode->isOfType(AtomMeasureTorsion::getClassTypeId()) )
         {
            AtomMeasureTorsion* smp = (AtomMeasureTorsion*)measureNode ;
            smp->jump();
         }
         if (measureNode->isOfType(AtomMeasureDist::getClassTypeId()) )
         {
            AtomMeasureDist* smd = (AtomMeasureDist*)measureNode ;
            smd->jump();
         }

      }
   }
}


/**
 * Updates atom nodes that may be invalide because atoms were deleted.
 */
void SGContainer::cleanAtomDeletes()
{
   AtomDeleteAction act;
   act.apply(getTopManip());
   act.apply(getTopRotManip());
   act.apply(getTopLine());
   act.apply(getTopLabel());
}


/**
 * This method clears the MO grid data
 * <p/>
 * For historical purposes, the MO grid itself is not deleted, but is
 * cleared out.
 *
 * The current grid is set to the zero pointer.
 */
void SGContainer::clearGrids()
{

  // There is no current Grid
  setCurrentGrid( (SingleGrid*)0 );

  // Clear the MO Grid
  /*tosme
  SingleGrid *grid = getMOGrid();
  if (grid && grid->fieldData() != (float*)0) {
    grid->clearField();
  }
  */

  int idx;

  // Clear MO cache
  for (idx=p_moGridCache.size()-1; idx>=0; idx--) {
    delete p_moGridCache[idx];
  }
  p_moGridCache.clear();

  // No longer needed - the PropVecGrid is cleared in the 
  // GridTool::initialize method
  //p_vecGrid->removeGrids();

}

/**
 * Clear out the grid scene graph.
 */
void SGContainer::clearGridScene()
{
  SoSwitch *moRoot = getMORoot();

  // This is hardwired for now; however, should add accessors,etc into VRExpt.
  SoSwitch *isoSwitch = (SoSwitch*)(moRoot->getChild(0));
  int numChildren = isoSwitch->getNumChildren();
  if (numChildren > 0) {
    isoSwitch->removeAllChildren();
  }
}

PropGrids *SGContainer::getPropGrids(int index) const
{
  return p_vecGrid->getPropGrids(index);
}


void  SGContainer::addPropGrids(PropGrids *grid)
{
  p_vecGrid->addGrid(grid);
}


void  SGContainer::removePropGrids()
{
  p_vecGrid->removeGrids();
}

void  SGContainer::removePropGrids(int index)
{
  p_vecGrid->removeGrids(index);
}


int  SGContainer::getNumPropGrids() const
{
  return p_vecGrid->getNumPropGrids();
}



/**
 * Find the MO grid with the specified unique key.
 * @return 0 pointer if not found
 */
SingleGrid* SGContainer::getMOGrid(const string& key) const
{
   SingleGrid *ret = 0;
   for (size_t idx=0; idx<p_moGridCache.size(); idx++) {
      if (p_moGridCache[idx]->name() == key) {
         ret = p_moGridCache[idx];
         break;
      }
   }
   return ret ;
}



/**
 * Add a grid to the grid cache and prune the cache if necessary.
 * The cache size is controlled by the environment variable ECCE_MAX_CACHED_MO.
 * The cache does NOT try to determine if a similar object already exists.
 * @param grid - the grid to add to the cache
 */
void SGContainer::addMOGrid(SingleGrid *grid)
{
   static char *maxCacheVar = getenv("ECCE_MAX_CACHED_MO");
   size_t maxCache = 10;
   if (maxCacheVar) {
     sscanf(maxCacheVar, "%zu", &maxCache);
   }

   p_moGridCache.push_back(grid);
   if (p_moGridCache.size() > maxCache) {
      p_moGridCache.pop_front();
   }
}



/** 
 * Remove the specified grid from the cache.
 * If the grid does not exist, quietly do nothing.
 */
void SGContainer::removeMOGrid(const string& key)
{
   deque<SingleGrid*>::iterator it = p_moGridCache.begin();
   while (it != p_moGridCache.end()) {
      if ( (*it)->name() == key) {
         p_moGridCache.erase(it);
         break;
      }
      it++;
   }
}



/**
 * Gets the pointer to the current grid.
 *
 * @returns the current grid which could be a 0 pointer.
 */
SingleGrid* SGContainer::getCurrentGrid() const
{
   return p_curGrid ;
}

/**
 * Sets the grid that will contain data for the one and only grid that
 * can be viewed at this time.
 * <p>
 * After setting the current grid, the relevent commands should be rerun
 * to update the display.
 * 
 * @param grid - The grid - could be a zero pointer.
 */
void SGContainer::setCurrentGrid(SingleGrid *grid)
{
   p_curGrid = grid ;
}

void SGContainer::initMORoot()
{
   SoSwitch *isoSwitch = new SoSwitch;
   p_MOSwitch->addChild(isoSwitch);
   SoSwitch *xySliceSwitch = new SoSwitch;
   p_MOSwitch->addChild(xySliceSwitch);
   SoSwitch *xzSliceSwitch = new SoSwitch;
   p_MOSwitch->addChild(xzSliceSwitch);
   SoSwitch *yzSliceSwitch = new SoSwitch;
   p_MOSwitch->addChild(yzSliceSwitch);


}

/**
 * returns the first node of specified name.
 */
SoNode * SGContainer::findNode(const string& name)
{
   SoNode *ret = 0;
   SoSearchAction *sa = new SoSearchAction;

//   sa->setType(SoNode::getClassTypeId(),true);
   sa->setInterest(SoSearchAction::FIRST);
   sa->setName(name.c_str());
   sa->setSearchingAll(true); // will search switched off nodes
   sa->apply(p_mainSep); 
   SoPath *path = sa->getPath();
   if (path != 0) {
     ret = (SoNode*)path->getTail();
   } else {
      //cout << "find node failed for " << name << endl;
   }
   delete sa;
   return ret;
}

/**
 * Returns switch for named property.
 * If switch doesn't exist, its created
 */
SoSwitch* SGContainer::getPropertySwitch(const string& name)
{
   SoSwitch *ret = 0;
   SoSearchAction *sa = new SoSearchAction;

   sa->setType(SoSwitch::getClassTypeId(),false);
   sa->setInterest(SoSearchAction::FIRST);
   sa->setName(name.c_str());
   sa->apply(p_mainSep); // This is where the property switches live
   SoPath *path = sa->getPath();
   if (path != 0) {
     ret = (SoSwitch*)path->getTail();
   }
   delete sa;
   if (ret == 0) {
      ret = new SoSwitch;
      ret->setName(name.c_str());
      ret->whichChild.setValue(SO_SWITCH_NONE);   // off by default
      p_mainSep->addChild(ret);
   }

   return ret;
}

/**
 * Get Plane Switch.
 */
SoSwitch* SGContainer::getPlaneSwitch()
{
   SoSwitch *ret = 0;
   SoSearchAction *sa = new SoSearchAction;

   sa->setType(SoSwitch::getClassTypeId(),false);
   sa->setInterest(SoSearchAction::FIRST);
   sa->setName("planeswitch");
   sa->apply(p_mainSep); 
   SoPath *path = sa->getPath();
   if (path != 0) {
     ret = (SoSwitch*)path->getTail();
   } else {
      cerr << "Oh oh, can't find the plane switch" << endl;
   }
   delete sa;

   return ret;
}

/**
 * Names in the scenegraph with special chars will fail on searches.
 * Replace such chars with _
 */
string SGContainer::hackSGName(const string& name)
{
   string tmpname;
   for (int idx=0; idx<name.size(); idx++) {
      if (name[idx] == '(' || name[idx] == ')') {
         tmpname.push_back('_');
      } else {
         tmpname.push_back(name[idx]);
      }
   }
   return tmpname;
}

void SGContainer::deletePlane(const string& name)
{
   SoSwitch *planes = getPlaneSwitch();
   int index = getPlaneSwitchIndex(hackSGName(name));
   planes->removeChild(index);
}

int SGContainer::getPlaneSwitchIndex(const string& name)
{
   int ret = -1;
   SoSwitch *planes = getPlaneSwitch();

   // This code assumes all children are separators with the name of 
   // the plane.
   for (int idx=0; idx<planes->getNumChildren(); idx++) {
      SoNode *node = planes->getChild(idx);
      if (node->getName() == name.c_str()) {
         ret = idx;
         break;
      }
   }
   return ret;
}

/**
 * Get Plane.
 */
StyledPlane* SGContainer::getPlane(const string& name)
{
   SoSwitch *planes = getPlaneSwitch();

   StyledPlane *ret = 0;
   SoSearchAction *sa = new SoSearchAction;

   sa->setType(StyledPlane::getClassTypeId(),false);
   sa->setInterest(SoSearchAction::FIRST);
   //cout << "searching for!" << name << "!" << endl;
   // Search fails if names have special chars in them so replace...

   string tmpname = hackSGName(name);
   sa->setName(tmpname.c_str());
   sa->apply(planes); 
   SoPath *path = sa->getPath();
   if (path != 0) {
     ret = (StyledPlane*)path->getTail();
   }
   delete sa;

   return ret;
}

/**
 * Add a plane to the scenegraph but with a StyledPlane wrapper.
 * The caller should use getPlane and only add if a plane of that
 * name doesn't already exist.  Searches over duplicate names won't work.
 * @param name should be unique
 */
StyledPlane *SGContainer::addPlane(SGPlane *plane, const string& name)
{
   StyledPlane *sp = 0;
   if (plane) {
      SoSwitch *planes = getPlaneSwitch();
      sp = new StyledPlane(plane);
      string tmpname = hackSGName(name);
      sp->setName(tmpname.c_str());
      plane->setName(tmpname.c_str());
      planes->addChild(sp);
      planes->whichChild.setValue(SO_SWITCH_ALL);
   }
   return sp;
}

SoSwitch* SGContainer::getMORoot()
{
   return p_MOSwitch ;
}

SoSwitch* SGContainer::getNMVecRoot()
{
   return p_NMVecSwitch;
}

SoSwitch* SGContainer::getNMRoot()
{
   return p_NMSwitch;
}
