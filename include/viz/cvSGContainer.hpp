#ifndef  _CVSGCONTAINER_
#define  _CVSGCONTAINER_

#include <vector>
#include <deque>

using std::vector;
using std::deque;

#include "viz/SGContainer.H"

class ChemColor;
class ChemRadii;
class SoSeparator;
class SoSwitch;
class ChemDisplay;
class ChemDisplayParam;
class SoMaterial;
class SoShapeHints;
class SGFragment;
class Fragment;
class IPropCalculation;
class SingleGrid;
class PropGrids;
class PropVecGrid;
class TProperty;

class  cvSGContainer : public SGContainer
{

   SO_NODE_HEADER(cvSGContainer);


   public:  

   enum SGObject { 
      VRCS,
      VRMO,
      VRVECTOR,
      VRNModeVec,
      VREGradVec,
      VRQuadrupole,
      VRDipole,
      VRPolarizability,
      VRShieldTensor,
      VREFIELD,
      VREFIELDGRAD,
      VRDIPOLETENSOR,
      VRSPINSPINTENSOR
   };

   cvSGContainer(); 
   virtual  ~cvSGContainer();
   SGContainer *clone();

   void changeExperiment(IPropCalculation* propCalc);
   IPropCalculation *getCalculation();

   void flipSwitch(SGObject obj, int state);
   void switchOffAll();

   SoGroup* getTopLabel();
   void removeMeasures();
   void updateMeasures();
   void adjustMeasures();

   SoSwitch* getMORoot();
   SoSwitch* getNMRoot();
   SoSwitch* getNMVecRoot();
   SoSwitch* getEGradVecRoot();
   SoSwitch* getQuadrupoleRoot();
   SoSwitch* getDipoleRoot();
   SoSwitch* getPolarizabilityRoot();
   SoSwitch* getShieldTensorRoot();
   SoSwitch* getGTRoot();
   SoSwitch* getTopVectors();
   SoSwitch* getEfieldRoot();
   SoSwitch* getEfieldGradRoot();
   SoSwitch* getdipoleTensorRoot();
   SoSwitch* getSpinSpinTensorRoot();


   // Grid stuff.  The MO gui has a grid and we have a generic property
   // grid structure too.
   // There is the notion of a single current grid so commands must set
   // this when its their turn to visualize.
   void        clearGrids();   // data structures
   void        clearGridScene();
   SingleGrid *getMOGrid(const string& key) const;
   void        addMOGrid(SingleGrid *grid);
   void        removeMOGrid(const string& key);

   SingleGrid *getCurrentGrid() const;
   void        setCurrentGrid(SingleGrid *grid);
   PropGrids  *getPropGrids(int index) const;
   void        addPropGrids(PropGrids *grid);
   void        removePropGrids();
   void        removePropGrids(int index);
   int         getNumPropGrids() const;

   protected:

   void initMORoot();

   private:

   IPropCalculation* p_propCalc;

   SoSwitch*                 p_GTSwitch;
   //SoSwitch*               p_TrajectorySwitch;
   SoSwitch*                 p_NMSwitch;
   SoSwitch*                 p_NMVecSwitch;
   SoSwitch*                 p_EGradVecSwitch;
   SoSwitch*                 p_QuadrupoleSwitch;
   SoSwitch*                 p_DipoleSwitch;
   SoSwitch*                 p_PolarizabilitySwitch; 
   SoSwitch*                 p_ShieldTensorSwitch;
   SoSwitch*                 p_topSepVects;
   SoSwitch*                 p_MOSwitch;
   SoSwitch*                 p_efieldSwitch;
   SoSwitch*                 p_efieldGradSwitch;
   SoSwitch*                 p_dipoleTensorSwitch;
   SoSwitch*                 p_spinSpinTensorSwitch;

   // The current visible grid.  Could be an MO or a generic cube grid
   SingleGrid               *p_curGrid;

   // Cache MO grids because they can be expensive to compute
   deque<SingleGrid*>       p_moGridCache;

   // Cube file grids
   PropVecGrid              *p_vecGrid;   

   SoINTERNAL public:
          static void       initClass();


};

#endif // _SGCONTAINER_ 

