#ifndef  _SGCONTAINER_
#define  _SGCONTAINER_

class ChemColor;
class ChemRadii;
class SoSeparator;
class SoSwitch;
class ChemDisplay;
class ChemDisplayParam;
class SoMaterial;
class SoShapeHints;
class SGFragment;
class CustomBBoxNode;
class SingleGrid;
class PropGrids;
class SGPlane;
class StyledPlane;
class PropVecGrid;
class PropTSVecTable;

#include <string>
  using std::string;
#include <vector>
  using std::vector;
#include <deque>
  using std::deque;

#include "inv/nodes/SoSwitch.H"

#include "util/ICommandObject.H"
#include "util/Receiver.H"

#include "tdat/TAtm.H"
#include "tdat/TPerTab.H"
#include "tdat/DisplayStyle.H"
#include "tdat/DisplayDescriptor.H"

#include "inv/ChemKit/ChemDisplayParam.H"
#include "inv/ChemKit/ChemSelection.H"

//////////////////////////////////////////////////////////////////////////////
//
// Basic Scene Graph:
//   The scene graph is configured so that we can view any set of
//   properties in combination if we ever want to do that.
//
//                               SoSeparator
//                                    |
//      +---------+-------------+-----+---+---------+---------+
//      |         |             |         |         |         |
//    Display  SoSwitch    
//       / \   (chemsys)  
//         \      |       
//          \   +-+----+  
//           \  |      |  
//            Display sep 
//                        
//                       
//                      
//                     
//////////////////////////////////////////////////////////////////////////////

  class  SGContainer : public SoSwitch, 
                       public ICommandObject,   // old commands
                       public Receiver          // new commands
{

   SO_NODE_HEADER(SGContainer);

   

public:  

    SGContainer(); 

    virtual SGContainer *clone();

    // Modifiers
    virtual void touchLattice();
    virtual void touchChemDisplay();
    virtual void touchChemDisplay(SGFragment* frag);
    virtual void updateColorNodes();

    DisplayStyle::Style toOurStyle(ChemDisplayParam::DisplayBinding style);
    ChemDisplayParam::DisplayBinding toChemkitStyle(DisplayStyle::Style style);

    // ICommandObject virtuals
    virtual string commandObjectType() const;

    // Accessors
    ChemColor        *getColorNode();
    ChemRadii        *getRadiiNode();
    ChemRadii        *getCPKRadiiNode();
    SoSwitch         *getcsSwitch();
    SoSeparator      *getCSTop();
    SoSeparator      *getMainSep();
    //SoSeparator         *getTopSepLabel();
    SoSwitch         *getTopSepLabel();
    SoGroup          *getTopManip();
    SoMaterial       *getMatNode();
    SoGroup          *getTopRotManip();
    SoGroup          *getTopLabel();
    SoGroup          *getTopLine();
    SoGroup          *getTopHBonds();
    SoSwitch         *getTopAtomVectors();
    SoGroup          *getDisplayGroup();
    SoSwitch         *getTopVects();
    SoPath           *getSelectionPath(ChemSelection *, int index);
    SoShapeHints     *getShapeHints();
    SoGroup	     *getTopClipManip() ;
    CustomBBoxNode *getViewBBox();

    string hackSGName(const string& name);

    // Uses scene graph traversal to get the 1st currently traversable fragment
    virtual SGFragment *getActiveFragment() ;

    virtual void adjustAtomContainers();
    virtual void cleanAtomDeletes();
    virtual void removeMeasures();
    virtual void updateMeasures();
    virtual void adjustMeasures();

    // Dynamic Style methods
    bool setStyle(const DisplayDescriptor& dd, bool all, SGFragment *frag);
    bool changeStyleSelected(const DisplayStyle& newstyle, SGFragment* frag);
    bool changeStyleSelectedResidues(const DisplayStyle& newstyle, SGFragment* frag);
    bool changeStyleAll(const DisplayStyle& newstyle, SGFragment* frag);
    bool changeStyleAllResidues(const DisplayStyle& newstyle, SGFragment* frag);

    bool selectStyle(const string& name);
    void createStyle(const DisplayDescriptor& dd, int pos=-1);
    void changeStyle(const DisplayDescriptor& dd);
    ChemColor *createCenterMassColors(const string& spectrum, ChemColor *cc);
    ChemColor *createResidueCenterMassColors(const string& spectrum, ChemColor *cc);
    ChemColor *createAtomIndexColors(const string& spectrum, ChemColor *cc);
    ChemColor *createAtomChargeColors(const string& spectrum, ChemColor *cc);
    ChemColor *createResidueIndexColors(const string& spectrum, ChemColor *cc,
                                        string style);
    ChemColor *createResidueNameColors(const string& spectrum, ChemColor *cc);
    ChemColor *createResidueChargeColors(const vector<string>& colors, ChemColor *cc);
    ChemColor *createHydrophobicityColors(const vector<string>& colors, ChemColor *cc);
    ChemColor *createSecondaryStructureColors(const vector<string>& colors, ChemColor *cc);
    ChemColor *createChainColors(const string& spectrum, ChemColor *cc);
    void resetToStyle(const DisplayDescriptor& dd);
    void removeStyle(const string& name);
    bool styleExists(const string& name) const;
    DisplayStyle getMainDisplayStyle() const ;
    vector<string> getDisplayStyleNames() const;
    DisplayDescriptor getDisplayDescriptor(const string& name) const;
    int getNumDisplayStyles() const;
    bool proteinStyleExists() const;
    ChemDisplayParam *getChemDisplayParam(int styleIndex) ;
    void setSphereOptions(const string& sphere, double complexity, double scale);
    void setStickOptions(const string& cap, double complexity, double radius);
    void setWireframeOptions(double width);
    void setShowRings(bool showRings);
    bool getShowRings() const;

    void selectAll();
    void unselectAll();

    // Uses scene graph traversal to get the all currently traversable fragments
    virtual vector<SGFragment*> getAllFragments() ;

    // Get "the" base fragment.
    SGFragment *getFragment() ;
    const SGFragment *getFragment() const ;

    // Other methods on "the" fragment
    void deleteFragment();
    void insertFragment(SGFragment* sgfrag);
    /**
     * Functions to implement the add hydrogen calls
     */
    virtual bool addHydrogens(SGFragment *frag, bool all, bool checkStatus);
    virtual bool removeHydrogens(SGFragment *frag, bool all);
    virtual void changeTo(TAtm *atm, TAtm *parent,
                          const char *tag, float scale);

    // These are often needed to know where to position the nubs.
    //bool isCPK() const;
    void      fixAllNubs() ;
    double    getScaleFactor(DisplayStyle) ; //const;
    void      setScaleFactor(float value);
    void      setAtomLabels(int on_off);
    void      setBondLabels(int on_off);
    void      setSelectModeDrag(int on_off);
    void      setResidueLabels(int on_off);
    void      setDepthCue(int on_off, float r, float g, float b);
    void      setShowHydrogens(int on_off);

    // public state - what is this used for??
    static const float selectRGB[];      // select color

    SoSwitch* getMORoot();
    void initMORoot();

    SoSwitch* getNMVecRoot();
    SoSwitch* getNMRoot();

    SoSwitch *getPropertySwitch(const string& name);
    SoNode *findNode(const string& name);

    SoSwitch *getPlaneSwitch();
    StyledPlane  *getPlane(const string& name);
    StyledPlane  *addPlane(SGPlane *plane, const string& name);
    void     deletePlane(const string& name);
    int      getPlaneSwitchIndex(const string& name);


SoINTERNAL public:
    static void		initClass();

  protected:
   virtual  ~SGContainer();
   void  constructor();

    SoSeparator *p_chemSysTop;
    SoSeparator *p_mainSep;

  private:
    SoSwitch    *p_csSwitch;
    SoGroup     *p_display;
    SoGroup     *p_topManip;
    SoSeparator     *p_topSepLine;

    /** A group to hold Atom to atom lines drawn by user */
    SoGroup     *p_topLine;  

    /** A group to hold the lines we use to draw Hydrogen bonds. */
    SoGroup     *p_topHBonds; 

    /** A group to hold vectors associated with trajectories.
        This should probably be moved to cvSGContainer if we ever remove
        the trajectory toolkit from the builder.
    */
    SoSwitch     *p_topAtomVectors; 


    SoGroup     *p_topRotManip;
    SoMaterial  *p_cdMat;
    SoSwitch    *p_topSepLabel;
    //SoSeparator    *p_topSepLabel;
    SoGroup     *p_topLabel;
    ChemColor   *p_colors;
    ChemRadii   *p_radii;
    ChemRadii   *p_CPK_radii;
    SoShapeHints *p_shapeHints;
    SoGroup	*p_topClipManip ;
    SoGroup	*p_dynamicStyles ;
    CustomBBoxNode *p_viewBbox;


  private:
    /** One per style 
     * - necessary because inventor node names get munged,
     * - and to report back to gui what the syles are */
    vector<DisplayDescriptor> p_displayDescs;
    /** One ChemDisplay per style */
    vector<ChemDisplay*> p_displays;
    /** One ChemDisplayParam per style */
    vector<ChemDisplayParam*> p_displayParams;
    void applyGlobalStyles(ChemDisplayParam *cdp);
    void applyStyle(const DisplayDescriptor& dd, ChemDisplayParam *cdp);
    ChemDisplayParam * createDisplayParam(const DisplayDescriptor& dd);
    void removeStyleNodes(int which);
    void removeUnusedStyles();

    bool p_showRings;

   // ---------------------- Property Stuff ----------------------------
   SoSwitch*                 p_MOSwitch;
   SoSwitch*                 p_NMVecSwitch;
   SoSwitch*                 p_NMSwitch;

   // ---------------------- Grid Property Stuff ----------------------------
   // The MO gui has a grid and we have a generic property grid structure too.
   // There is the notion of a single current grid so commands must set
   // this when its their turn to visualize.
   public: 
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

   // ---------------------- Trace data ----------------------------
   // Used by Normal Modes, GeomTrace...


   private: 
   // The current visible grid.  Could be an MO or a generic cube grid
   SingleGrid               *p_curGrid;

   // Cache MO grids because they can be expensive to compute
   deque<SingleGrid*>       p_moGridCache;

   // Cube file grids
   PropVecGrid              *p_vecGrid;   


   // From PropSGFragment - not sure of use
   //IBondList        *p_propBondIndices ;


};

#endif // _SGCONTAINER_ 
