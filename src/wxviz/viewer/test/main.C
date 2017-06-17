/*
 * All rights reserved.
 * 
 * Developers: Interactive Technology Group (GTI)
 * Team: Josep Blat, Eduard Gonzalez, Sergi Gonzalez,
 *       Daniel Soto, Alejandro Ramirez, Oscar Civit.
 * 
 * Homepage: http://www.tecn.upf.es/openMOIV/
 * Contact:  openmoiv@upf.es
 *
 *
 * Redistribution and use in source and binary forms, with or without modification, are 
 * permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer. 
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation and/or 
 *   other materials provided with the distribution. 
 * 
 * - Neither the name of the Universitat Pompeu Fabra nor the names of its
 *   contributors may be used to endorse or promote products derived from this 
 *   software without specific prior written permission. 
 * 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Include necessary Inventor stuff

#include <iostream.h>
#define __SoGtk__


#include <Inventor/nodes/SoSeparator.h>
#include "ChemExaminerViewer.h"

#ifdef __SoQt__
#define SoGUI SoQt
#define GUIWidget QWidget*
#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#endif

#ifdef __SoXt__
#define GUIWidget Widget
#define SoGUI SoXt
#include <Inventor/Xt/SoXt.h>
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>
#endif

#ifdef __SoGtk__
#define GUIWidget GtkWidget*
#define SoGUI SoGtk
#include <Inventor/Gtk/SoGtk.h>
#include <Inventor/Gtk/viewers/SoGtkExaminerViewer.h>
#endif

#ifdef __SoWin__
#define GUIWidget HWND
#define SoGUI SoWin
#include <Inventor/Win/SoWin.h>
#include <Inventor/Win/viewers/SoWinExaminerViewer.h>
#endif

#ifdef __SoFl__
#define GUIWidget Fl_Window*
#define SoGUI SoFl
#include <Inventor/FL/SoFL.h>
#include <Inventor/FL/viewers/SoFlExaminerViewer.h>
#endif

// Include necessarry Molecular Inventor stuff
#include <ChemKit2/ChemColor.h>
#include <ChemKit2/ChemData.h>
#include <ChemKit2/ChemAssociatedData.h>
#include <ChemKit2/ChemSelection.h>
#include <ChemKit2/ChemDisplay.h>
#include <ChemKit2/ChemDisplayParam.h>
#include <ChemKit2/ChemInit.h>
#include <ChemKit2/ChemRadii.h>

int main(int argc, char **argv)
{
	// Initialize Inventor. This returns a main window to use.
	// If unsuccessful, exit.
	GUIWidget myWindow = SoGUI::init(argv[0]); // pass the app name
	
	if (myWindow == 0) exit(1);

	// Initialize Molecular Inventor
	ChemInit::initClasses();

	cout << "Ok here\n";

	// Create the viewer
	ChemExaminerViewer *myViewer = new ChemExaminerViewer(myWindow); //,"OpenMOIV Demo");
	//SoFlExaminerViewer *myViewer = new SoFlExaminerViewer (myWindow,"OpenMOIV Demo");

	cout << "Ok here\n";

	// Create a yellow color and set the viewer's overlay color
	// table to have the yellow color at index 1.
	SbColor lassoColor(1, 1, 0);
	myViewer->setOverlayColorMap(1, 1, &lassoColor);

	// Make the root node of the scene graph
	SoSeparator *root = new SoSeparator;
	root->ref();

	// Create a ChemSelection node which allows for selecting using
	// a drag-rectangle.
	ChemSelection *chemSelection = new ChemSelection;

	chemSelection->parts.setValue(
	ChemSelection::ALL
	);
	chemSelection->lassoType.setValue(ChemSelection::DRAGGER);
	//chemSelection->lassoType.setValue(ChemSelection::LASSO);
	//chemSelection->lassoType.setValue(ChemSelection::NOLASSO);
	chemSelection->radiusSelect.setValue(TRUE);

	chemSelection->radiusSelectSphereDrawStyle.setValue(ChemSelection::LINES);

	ChemData *chemData = new ChemData;
	ChemAssociatedData *chemassociateddata = new ChemAssociatedData;
			
	chemSelection->addChild(chemData);

	ChemColor *color = new ChemColor();
	ChemRadii *radii = new ChemRadii();
	chemSelection->addChild(color);
	chemSelection->addChild(radii);

	ChemDisplayParam* cdp = NULL;
	ChemDisplay* cd = NULL;

	cdp = new ChemDisplayParam();		
	cd = new ChemDisplay();

	chemSelection->addChild(cdp);
	chemSelection->addChild(cd);

	myViewer->setNodes(chemData,chemassociateddata,radii,color,cdp,cd);

	root->addChild(chemSelection);

	myViewer->setSceneGraph(root);

	// Set the title on the viewer window
	char title[255];
	strcpy(title,"OpenMOIV Demo - Empty scene - Using So");
#ifdef __SoQt__
	//Qt non commercial requires substring "Qt" was present
	strcat(title,"Qt");
#endif
#ifdef __SoWin__
	strcat(title,"Win");
#endif
#ifdef __SoFl__
	strcat(title,"Fl");
#endif
#ifdef __SoGtk__
	strcat(title,"Gtk");
#endif

	myViewer->setTitle(title);

	// Set default parameters
	cdp->displayStyle.setValue(ChemDisplayParam::DISPLAY_BALLSTICK);
	cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON);
	cdp->atomSphereDisplayStyle.setValue(ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY);
	cdp->bondCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_LOD_NOCAP);
	color->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);

	myViewer->UpdateMenuItems();

	// Tell the viewer to show itself
	myViewer->show();

	// Display the main window and enter the main event loop
	SoGUI::show(myWindow);
	SoGUI::mainLoop();

	delete myViewer;
	root->unref();

	return 0;
}
#undef Widget
#undef SoGUI
