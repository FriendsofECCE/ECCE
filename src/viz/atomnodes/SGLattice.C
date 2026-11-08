#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"

#include "inv/actions/SoPickAction.H"

#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoSeparator.H"

#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoCylinder.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoTransform.H"
#include "viz/freeglut_font_data.H"
#include "viz/ViewerText.H"

#include "viz/SGLattice.H"
#include "tdat/BoxShape.H"

#include <iostream>
  using std::cout;
  using std::endl;


SO_NODE_SOURCE(SGLattice);

static uint32_t s_edgeColor = SbColor(0.5, 0.5, 0.5).getPackedValue();
static bool s_labelsShown = true;

void SGLattice::setEdgeColor(float r, float g, float b)
{
   s_edgeColor = SbColor(r, g, b).getPackedValue();
}

void SGLattice::setLabelsShown(bool on) { s_labelsShown = on; }
bool SGLattice::labelsShown() { return s_labelsShown; }

void SGLattice::initClass()
{
   SO_NODE_INIT_CLASS(SGLattice, SoLineSet, "LineSet");
}


SGLattice::SGLattice()
{
  SO_NODE_CONSTRUCTOR(SGLattice);
  constructor();
}


void SGLattice::constructor()
{
   p_noLabels = true;
   for (int i = 0; i < 3; ++i) p_labelVisible[i] = false;
   // set up 12 lines (2 points each)
   // This is the default box but of course it changes with replication
   int cnt = 12;
   int32_t newValues[12] = { 2,2,2, 2,2,2, 2,2,2, 2,2,2 };
   numVertices.setValues(0,cnt,newValues);
   p_line = new SoVertexProperty;
   vertexProperty.setValue(p_line);

   // The bulk of the lattice is one color but the primary lattice 
   // vectors each have their own color cause it looks nice.
   p_xcolor = SbColor(1.0, 0.0, 0.0).getPackedValue();
   p_ycolor = SbColor(0.0, 1.0, 0.0).getPackedValue();
   p_zcolor = SbColor(0.0, 0.0, 1.0).getPackedValue();
   p_color = SbColor(0.5, 0.5, 0.5).getPackedValue();


   // Each vertex will have its own color so that we can color 
   // a1, a2, a3 primary lines differently.  See SoMaterialBinding
   p_line->materialBinding = SoVertexProperty::PER_VERTEX;

   updateColors();


}

void SGLattice::GLRender(SoGLRenderAction *action)
{
   SoLineSet::GLRender(action);

   // First see if the object is visible and should be rendered
   // now. This is a method on SoShape that checks for INVISIBLE
   // draw style, BOUNDING_BOX complexity, and delayed
   // transparency.
   if (! shouldGLRender(action) || p_noLabels || !s_labelsShown)
      return;

   // Labels and diagnostic positions share the same visibility and endpoints.
   glPushAttrib(GL_CURRENT_BIT);
   const char *text[] = {"a", "b", "c"};
   const float colors[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
   for (int i = 0; i < 3; ++i) {
      double xyz[3];
      if (!labelPosition(i, xyz)) continue;
      glColor3fv(colors[i]);
      glRasterPos3dv(xyz);
      drawViewerText(fontOffset, text[i]);
   }
   glPopAttrib();
}


bool SGLattice::labelPosition(int i, double xyz[3])
{
   if (p_noLabels || i < 0 || i > 2 || !p_labelVisible[i]) return false;
   MPoint& p = (i == 0 ? p_a1str : (i == 1 ? p_a2str : p_a3str));
   const float off = 0.05f;
   xyz[0] = p.x() - off;
   xyz[1] = p.y() - off;
   xyz[2] = p.z() - off;
   return true;
}


bool SGLattice::drawnBounds(double lo[3], double hi[3])
{
   int n = p_line->vertex.getNum();
   if (n < 1) return false;
   for (int k = 0; k < 3; k++) lo[k] = hi[k] = p_line->vertex[0][k];
   for (int i = 1; i < n; i++)
      for (int k = 0; k < 3; k++) {
         double v = p_line->vertex[i][k];
         if (v < lo[k]) lo[k] = v;
         if (v > hi[k]) hi[k] = v;
      }
   return true;
}


void SGLattice::setLattice(LatticeDef *lattice)
{

   float xyz[3];
   p_noLabels = true;
   for (int i = 0; i < 3; ++i) p_labelVisible[i] = false;

   if (lattice) {
      int na1, na2, na3;
      lattice->getReplicationFactors(na1, na2, na3);

      vector<MPoint> *edges = lattice->toLines();


      int totPoints = 0;
      int nlines = edges->size();

      // Get the vectors that tell us the increment in each direction
      vector<MPoint> *vecs = lattice->toVectors();
      MPoint a1 = (*vecs)[0];
      MPoint a2 = (*vecs)[1];
      MPoint a3 = (*vecs)[2];
      delete vecs;

      // Define the base location of the axis labels.
      p_a1str = (*edges)[1];
      p_a2str = (*edges)[3];
      p_a3str = (*edges)[5];
      p_noLabels = false;
      // A collapsed edge has no axis direction to label (for example a 2-D cell).
      p_labelVisible[0] = a1.length() > 1.0e-10;
      p_labelVisible[1] = a2.length() > 1.0e-10;
      p_labelVisible[2] = a3.length() > 1.0e-10;

      // A GROMACS dodecahedron or truncated octahedron is simulated as its
      // compact cell around the box centre, not as the vector parallelepiped.
      boxshape::Vec va = {{a1.x(), a1.y(), a1.z()}};
      boxshape::Vec vb = {{a2.x(), a2.y(), a2.z()}};
      boxshape::Vec vc = {{a3.x(), a3.y(), a3.z()}};
      boxshape::Shape shape;
      if (boxshape::isGromacsCompactBox(va, vb, vc) &&
          boxshape::wignerSeitz(va, vb, vc, shape)) {
         // gmx trjconv -ur compact centres on (a+b+c)/2, gmx solvate and
         // editconf -c fill the brick around the diagonal's half: the one
         // nearer the atoms is where they are.
         MPoint fc = lattice->getFragCenter();
         double cx = 0.5*(va[0]+vb[0]+vc[0]);
         double cy = 0.5*(va[1]+vb[1]+vc[1]);
         double cz = 0.5*(va[2]+vb[2]+vc[2]);
         const double rx = 0.5*va[0], ry = 0.5*vb[1], rz = 0.5*vc[2];
         const double dt = (cx-fc.x())*(cx-fc.x()) + (cy-fc.y())*(cy-fc.y()) +
                           (cz-fc.z())*(cz-fc.z());
         const double dr = (rx-fc.x())*(rx-fc.x()) + (ry-fc.y())*(ry-fc.y()) +
                           (rz-fc.z())*(rz-fc.z());
         if (dr < dt) { cx = rx; cy = ry; cz = rz; }
         // The first three lines are coloured as the axes; keep their slots
         // empty so the cell edges all get the plain cell colour.
         edges->clear();
         p_noLabels = true;
         for (int e=0; e<6; e++) edges->push_back(MPoint(0.,0.,0.));
         for (size_t e=0; e<shape.edges.size(); e++) {
            const boxshape::Vec& s = shape.vertices[shape.edges[e].first];
            const boxshape::Vec& t = shape.vertices[shape.edges[e].second];
            edges->push_back(MPoint(s[0]+cx, s[1]+cy, s[2]+cz));
            edges->push_back(MPoint(t[0]+cx, t[1]+cy, t[2]+cz));
         }
         nlines = edges->size();
      }


      p_line->vertex.startEditing();
      double xt, yt, zt;
      for (int a=0; a<na1; a++) {
         for (int aa=0; aa<na2; aa++) {
            for (int aaa=0; aaa<na3; aaa++) {
               xt = a1.x() * a + a2.x() * aa + a3.x() * aaa;
               yt = a1.y() * a + a2.y() * aa + a3.y() * aaa;
               zt = a1.z() * a + a2.z() * aa + a3.z() * aaa;
               //cout << "deltas" << xt << " " << yt << " " << zt << endl;
               for (int idx=0; idx<nlines; idx+=2) {
                  MPoint s = (*edges)[idx];
                  //s.print("start ");
                  xyz[0] = s.x() + xt;
                  xyz[1] = s.y() + yt;
                  xyz[2] = s.z() + zt;
                  //cout << "start " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
                  p_line->vertex.set1Value(totPoints++, xyz);

                  MPoint e = (*edges)[idx+1];
                  //e.print("end ");
                  xyz[0] = e.x() + xt;
                  xyz[1] = e.y() + yt;
                  xyz[2] = e.z() + zt;
                  //cout << "end " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
                  p_line->vertex.set1Value(totPoints++, xyz);
               }
            }
         }
      }
      p_line->vertex.finishEditing();

      // Update the counts and the definition of the lines
      // Two points per line
      numVertices.setNum(totPoints/2); // num lines
      numVertices.startEditing();
      for (int idx=0; idx<totPoints/2; idx++) {
         numVertices.set1Value(idx,2);
      }
      numVertices.finishEditing();
      p_line->vertex.setNum(totPoints);

      // Update the color binding array
      updateColors();

      delete edges;

   } else {
      p_line->vertex.setNum(0);
      numVertices.setNum(0);
   }
   touch();

   /*
   cout << "SGLattice lines " << endl;
   int num = p_line->vertex.getNum();
   for (int x=0; x<num; x++) {
      SbVec3f f = p_line->vertex[x];
      cout << "line " << x << " " << f[0] << "," << f[1] << "," << f[2] << endl;
   }
   */
}

/**
 * Set the color array so that:
 *    the first line is red,
 *    the second line is green,
 *    the third line is blue,
 *    the remainder are gray.
 */
void SGLattice::updateColors()
{
   p_color = s_edgeColor;
   int cnt =  p_line->vertex.getNum();
   p_line->orderedRGBA.set1Value(0,p_xcolor);
   p_line->orderedRGBA.set1Value(1,p_xcolor);
   p_line->orderedRGBA.set1Value(2,p_ycolor);
   p_line->orderedRGBA.set1Value(3,p_ycolor);
   p_line->orderedRGBA.set1Value(4,p_zcolor);
   p_line->orderedRGBA.set1Value(5,p_zcolor);
   for (int idx=6; idx<cnt; idx++) {
      p_line->orderedRGBA.set1Value(idx,p_color);
   }
}


SGLattice::~SGLattice()
{
}

