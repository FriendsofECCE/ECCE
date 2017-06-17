#include <iostream>
  using std::cout;
  using std::endl;

#include "inv/nodes/SoIndexedTriangleStripSet.H"
#include "inv/misc/SoChildList.H"
#include "inv/nodes/SoSwitch.H"

#include "tdat/SingleGrid.H"

#include "inv/ChemKit/ChemIso.H"
#include "inv/ChemKit/ChemContour.H"

#include "viz/SGContainer.H"
#include "viz/IsoValueCmd.H"

IsoValueCmd::IsoValueCmd(const string& name,Receiver *receiver,
                         IPropCalculation *calc)
  : PropCmd(name,receiver,calc)
{
  init();
}

IsoValueCmd::~IsoValueCmd() { }

void IsoValueCmd::init()
{
   addParameter(new CommandParameter("Value", 0.01));
   addParameter(new CommandParameter("transparency", 0.5));
   addParameter(new CommandParameter("positiveRed", 1.0));
   addParameter(new CommandParameter("positiveGreen", 0.0));
   addParameter(new CommandParameter("positiveBlue", 0.0));
   addParameter(new CommandParameter("negativeRed", 0.0));
   addParameter(new CommandParameter("negativeBlue", 0.0));
   addParameter(new CommandParameter("negativeGreen", 1.0));
}

bool IsoValueCmd::execute() throw(EcceException)
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   double transparency = getParameter("transparency")->getDouble();

   double isoval;
   double sliderval = getParameter("Value")->getDouble();

   // Isoval is a log value - convert it to normal value
   isoval = pow(10.0, sliderval);

   if (sg->getCurrentGrid()) {
      if (isoval < sg->getCurrentGrid()->absFieldMin() ||
            sliderval == -5.0) {
         isoval = 0.0;
      }

      // This requires knowing the hardwired scene in VRExpt:
      SoSwitch *isoSwitch = (SoSwitch*)((sg->getMORoot())->getChild(0));

      double posR = getParameter("positiveRed")->getDouble();
      double posG = getParameter("positiveGreen")->getDouble();
      double posB = getParameter("positiveBlue")->getDouble();
      double negR = getParameter("negativeRed")->getDouble();
      double negG = getParameter("negativeGreen")->getDouble();
      double negB = getParameter("negativeBlue")->getDouble();

      uint32_t posColor = SbColor(posR, posG, posB).getPackedValue(transparency);
      uint32_t negColor = SbColor(negR, negG, negB).getPackedValue(transparency);
      uint32_t zeroColor = SbColor(1.0, 1.0, 0.0).getPackedValue(transparency);
      uint32_t positiveLobeColor,negativeLobeColor;
      if (isoval > 0.0) {
         positiveLobeColor = posColor;
         negativeLobeColor = negColor;
      }
      else {   // = 0.0
         positiveLobeColor = zeroColor;
         negativeLobeColor = zeroColor;
      }

      // Even though the isovalue is acting on only one of the possible surface
      // types (solid, mesh, contour), we will update the isovalue (and colors, 
      // since going to a 0.0 value means we want to have a different colored 
      // surface) on all three surface types.  This will allow us to not worry 
      // about updating the isovalue when the user changes the surface type.  
      // It's a trade-off with the speed of updating here perhaps.

      if (isoSwitch->getNumChildren() > 0) {
         // ----- Solid
         SoNode  *node = (SoNode *)isoSwitch->getChild(0);
         unsigned long numChildren = ((SoSeparator*)node)->getNumChildren();
         ChemIso *isosurf1 = (ChemIso*)((SoSeparator*)node)->getChild(1);
         isosurf1->threshold.setValue(isoval);
#ifndef MI10
         SoChildList *childList = isosurf1->getChildren();
         SoIndexedTriangleStripSet *triStrip = (SoIndexedTriangleStripSet *)(*childList)[0];
         SoVertexProperty *xx=(SoVertexProperty *)triStrip->vertexProperty.getValue();
         xx->orderedRGBA.setValue(positiveLobeColor);
#else
         isosurf1->orderedRGBA.setValue(positiveLobeColor);
#endif

         if (numChildren==3) {
            ChemIso *isosurf2 = (ChemIso*)((SoSeparator*)node)->getChild(2);
            isosurf2->threshold.setValue(-isoval);

#ifndef MI10
            childList = isosurf2->getChildren();
            SoIndexedTriangleStripSet *triStrip = (SoIndexedTriangleStripSet *)(*childList)[0];
            SoVertexProperty *yy=(SoVertexProperty *)triStrip->vertexProperty.getValue();
            yy->orderedRGBA.setValue(negativeLobeColor);
#else
            isosurf2->orderedRGBA.setValue(negativeLobeColor);
#endif
         }


         // ----- Mesh
         node = (SoNode *)isoSwitch->getChild(1);
         ChemContour *mesh = (ChemContour *)node;

         unsigned long numLevels = mesh->levels.getNum();

         mesh->levels.set1Value(0, isoval);
         mesh->orderedRGBA.set1Value(0, positiveLobeColor);

         if (numLevels==2) {
            mesh->levels.set1Value(1, -isoval);
            mesh->orderedRGBA.set1Value(1, negativeLobeColor);
         }


         // ----- Contour
         node = (SoNode *)isoSwitch->getChild(2);
         ChemContour *contours = (ChemContour *)node;

         numLevels = contours->levels.getNum();

         contours->levels.set1Value(0, isoval);
         contours->orderedRGBA.set1Value(0, positiveLobeColor);

         if (numLevels==2) {
            contours->levels.set1Value(1, -isoval);
            contours->orderedRGBA.set1Value(1, negativeLobeColor);
         }
      }
   }

   return true;
}
