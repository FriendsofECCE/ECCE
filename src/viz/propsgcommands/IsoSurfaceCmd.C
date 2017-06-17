#include <iostream>
  using std::cout;
  using std::endl;
#include "inv/nodes/SoSwitch.H"
#include "inv/misc/SoChildList.H"
#include "inv/nodes/SoShapeHints.H"
#include "inv/nodes/SoIndexedTriangleStripSet.H"

#include "util/ErrMsg.H"

#include "tdat/SingleGrid.H"

#include "inv/ChemKit/SFVec3i.H"
#include "inv/ChemKit/ChemLattice3.H"
#include "inv/ChemKit/ChemIso.H"
#include "inv/ChemKit/ChemContour.H"

#include "viz/SGContainer.H"
#include "viz/IsoSurfaceCmd.H"

IsoSurfaceCmd::IsoSurfaceCmd(const string& name, Receiver *receiver,
                             IPropCalculation *calc)
  : PropCmd(name, receiver, calc)
{
  init();
}

IsoSurfaceCmd::~IsoSurfaceCmd() 
{ 
}

void IsoSurfaceCmd::init()
{
   addParameter(new CommandParameter("transparency", 0.5));

   addParameter(new CommandParameter("positiveRed", 0.5));
   addParameter(new CommandParameter("positiveGreen", 0.5));
   addParameter(new CommandParameter("positiveBlue", 0.5));
   addParameter(new CommandParameter("negativeRed", 0.5));
   addParameter(new CommandParameter("negativeBlue", 0.5));
   addParameter(new CommandParameter("negativeGreen", 0.5));

}

bool IsoSurfaceCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   // Get computed grid info.  
   SingleGrid *gridStruct = sg->getCurrentGrid();
   if (gridStruct != 0) {

      string fieldType = gridStruct->type();
      unsigned int resX = gridStruct->dimensions()[0];
      unsigned int resY = gridStruct->dimensions()[1];
      unsigned int resZ = gridStruct->dimensions()[2];
      double xStart = gridStruct->origin()[0];
      double yStart = gridStruct->origin()[1];
      double zStart = gridStruct->origin()[2];
      double xEnd = gridStruct->corner()[0];
      double yEnd = gridStruct->corner()[1];
      double zEnd = gridStruct->corner()[2];
      float *field = gridStruct->fieldData();

      float xDelta = (xEnd-xStart)/(resX-1);
      float yDelta = (yEnd-yStart)/(resY-1);
      float zDelta = (zEnd-zStart)/(resZ-1);

      unsigned int gridRes = resX * resY * resZ;

      // Get bounding box for (uniform) ChemLattice3
      // X range
      float bBox[6];
      bBox[0] = xStart;
      bBox[1] = xEnd;
      // Y range
      bBox[2] = yStart;
      bBox[3] = yEnd;
      // Z range
      bBox[4] = zStart;
      bBox[5] = zEnd;

      int32_t dims[3];
      dims[0] = resX;
      dims[1] = resY;
      dims[2] = resZ;


      sg->clearGridScene();
      SoSwitch *moRoot = sg->getMORoot();

      // Too bad you have to know the internal structure...
      SoSwitch *isoSwitch = (SoSwitch*)(sg->getMORoot()->getChild(0));

      // Create some default colors for isosurfaces
      double transparency = getParameter("transparency")->getDouble();

      double posR = getParameter("positiveRed")->getDouble();
      double posG = getParameter("positiveGreen")->getDouble();
      double posB = getParameter("positiveBlue")->getDouble();
      double negR = getParameter("negativeRed")->getDouble();
      double negG = getParameter("negativeGreen")->getDouble();
      double negB = getParameter("negativeBlue")->getDouble();

      uint32_t posColor = SbColor(posR, posG, posB).getPackedValue(transparency);
      uint32_t negColor = SbColor(negR, negG, negB).getPackedValue(transparency);
      uint32_t zeroColor = SbColor(1.0, 0.7, 0.0).getPackedValue(transparency);
      uint32_t positiveLobeColor,negativeLobeColor;

      // How does MI handle freeing its memory (e.g, lattice)?
      ChemLattice3 *lattice;

      if ((resX == resY) && (resX == resZ)) {
         // Lattice is UNIFORM
         lattice = new ChemLattice3(LATTICE_DATA_FLOAT32, LATTICE_COORD_UNIFORM);
         lattice->coord.setValues(0, 6, bBox);
      }

      else {
         // Lattice is RECTILINEAR
         lattice = new ChemLattice3(LATTICE_DATA_FLOAT32, LATTICE_COORD_RECTILINEAR);
         // Specify x,y, and z coordinates
         float *coords = new float[resX+resY+resZ];

         unsigned int kdx = 0;
         int idx;
         for (idx=0; idx<resX; idx++) {
            coords[kdx] = xStart + idx*xDelta;
            kdx++;
         }
         for (idx=0; idx<resY; idx++) {
            coords[kdx] = yStart + idx*yDelta;
            kdx++;
         }
         for (idx=0; idx<resZ; idx++) {
            coords[kdx] = zStart + idx*zDelta;
            kdx++;
         }
         lattice->coord.setValues(0, resX+resY+resZ, coords);

         delete coords;
      }


      // Set the lattice dimensions
      lattice->dimension.setValue(dims);

      lattice->nDataVar = 1;    // NB! if a color field is used, make this =2

      // Copy the appropriate field into the (Molecular Inventor) lattice data

      ((SoMFFloat *)(lattice->data))->setNum(gridRes);
      float *latticeData = ((SoMFFloat *)(lattice->data))->startEditing();
      EE_RT_ASSERT(latticeData,EE_FATAL,"Unexpected Null pointer.");

      for (int idx=0; idx<gridRes; idx++) {
         latticeData[idx] = field[idx];
      }
      ((SoMFFloat *)(lattice->data))->finishEditing();

      float isovalue = 0.05;

      // Since we only deal with absolute values of isovalues:
      if (isovalue > 0.0) {
         positiveLobeColor = posColor;
         negativeLobeColor = negColor;
      }
      else {    // = 0.0
         positiveLobeColor = zeroColor;
         negativeLobeColor = zeroColor;
      }

      //-------------------------------------------------------------
      // chemIso = shapeHints + isosurf1 [+ isosurf2]

      SoSeparator *chemIso = new SoSeparator;

      SoShapeHints *shapeHints = new SoShapeHints;
      shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
      shapeHints->faceType = SoShapeHints::CONVEX;
      // SOLID causes one-sided lighting on the isosurface.
      // Let's make it two-sided for now (no SOLID shapeType) and
      // eventually allow the user to toggle this via a VOD Details dialog.
      shapeHints->shapeType = SoShapeHints::SOLID;

      chemIso->addChild(shapeHints);

      ChemIso *isosurf1 = new ChemIso;
      isosurf1->regenerate(true);

      isosurf1->data = lattice;
      isosurf1->color = NULL;

      isosurf1->dataVar = 0;
      isosurf1->colorVar = 0;

      isosurf1->threshold.setValue(isovalue);

#ifndef MI10
      /* Well, I guess I sort of screwed you over here, but nothing that can't be
         corrected easily.  What I did was, instead of deriving ChemIso from
         SoIndexedTriangleStripSet, ChemIso now has an instance of
         SoIndexedTriangleStripSet as a child.  Therefore, you need to do the
         following to your code: */

      SoChildList *childList = isosurf1->getChildren();
      SoIndexedTriangleStripSet *triStrip = (SoIndexedTriangleStripSet *)(*childList)[0];
      SoVertexProperty *vp =
         (SoVertexProperty *)triStrip->vertexProperty.getValue();
      vp->orderedRGBA.setValue(positiveLobeColor);

      /* The same sort of thing would apply to ChemContour and ChemContour2 but
         instead of SoIndexedTriangleStripSet, it would be an instance of an
         SoIndexedLineSet. */
#else
      SoVertexProperty *vp =
         (SoVertexProperty *)isosurf1->vertexProperty.getValue();
      vp->orderedRGBA.setValue(positiveLobeColor);
#endif


      chemIso->addChild(isosurf1);


      // For other than Density field, we want to display both positive and
      // negative lobes (isosurfaces)
      if ( fieldType != "Density") {
         ChemIso *isosurf2 = new ChemIso;
         isosurf2->regenerate(true);

         isosurf2->data = lattice;
         isosurf2->color = NULL;

         //    isosurf2->minValue.setValue(fieldMin);
         //    isosurf2->maxValue.setValue(fieldMax);

         isosurf2->dataVar = 0;
         isosurf2->colorVar = 0;
         isosurf2->threshold.setValue(-isovalue);

#ifndef MI10
         childList = isosurf2->getChildren();
         triStrip = (SoIndexedTriangleStripSet *)(*childList)[0];
         vp = (SoVertexProperty *)triStrip->vertexProperty.getValue();
         vp->orderedRGBA.setValue(negativeLobeColor);
#else
         vp = (SoVertexProperty *)isosurf2->vertexProperty.getValue();
         vp->orderedRGBA.setValue(negativeLobeColor);
#endif

         isosurf2->generateNormals = true;
         isosurf2->flipNormals = true;

         chemIso->addChild(isosurf2);

         // This seems to be equivalent to having a 'regenerate' on each isosurf
         //    chemIso->enableNotify(true);
      }
      //-------------------------------------------------------------
      // "Mesh"

      ChemContour *chemMesh = new ChemContour;

      chemMesh->lattice = lattice;

      chemMesh->iAxis.setValue(true);
      chemMesh->jAxis.setValue(true);
      chemMesh->kAxis.setValue(true);
      chemMesh->dataVar = 0;

      chemMesh->levels.set1Value(0, isovalue);
      chemMesh->orderedRGBA.set1Value(0, positiveLobeColor);

      // For other than Density field, we want to display both positive and
      // negative lobes (isosurfaces)
      if (fieldType != "Density") {
         chemMesh->levels.set1Value(1, -isovalue);
         chemMesh->orderedRGBA.set1Value(1, negativeLobeColor);
      }

      // Putting the 'regenerate' here (at the end) seems to be very crucial
      // for this node to get re-displayed.
      chemMesh->regenerate(true);

      //-------------------------------------------------------------
      //-------------------------------------------------------------
      // "Contours"

      ChemContour *chemContour = new ChemContour;

      chemContour->lattice = lattice;

      chemContour->iAxis.setValue(true);
      chemContour->jAxis.setValue(false);
      chemContour->kAxis.setValue(false);
      chemContour->dataVar = 0;

      chemContour->levels.set1Value(0, isovalue);
      chemContour->orderedRGBA.set1Value(0, positiveLobeColor);
      //  vp = (SoVertexProperty *)chemContour->vertexProperty.getValue();
      //  vp->orderedRGBA.setValue(positiveLobeColor);

      // For other than Density field, we want to display both positive and
      // negative lobes (isosurfaces)
      if (fieldType != "Density") {
         chemContour->levels.set1Value(1, -isovalue);
         chemContour->orderedRGBA.set1Value(1, negativeLobeColor);
      }

      // Putting the 'regenerate' here (at the end) seems to be very crucial
      // for this node to get re-displayed.
      chemContour->regenerate(true);


      //-------------------------------------------------------------

      // Note that the order here much match the order in SurfDisplayType.
      isoSwitch->addChild(chemIso);
      isoSwitch->addChild(chemMesh);
      isoSwitch->addChild(chemContour);

      isoSwitch->whichChild = 0;    // Set according to Surface display type

      moRoot->whichChild = 0;
   }
   return true;

}
