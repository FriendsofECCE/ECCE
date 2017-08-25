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

//////////////////////////////////////////////////////////////////////
// ChemExaminerViewer.cpp: implementation of the ChemExamineViewer class.
//
//////////////////////////////////////////////////////////////////////

#define __SoGtk__
using namespace std;
#include <iostream>

#include "ChemExaminerViewer.h"

#ifdef __SoQt__
#define GUIWidget QWidget*
#define SoGUIExaminerViewer SoQtExaminerViewer
#define SoGUIFullViewer SoQtFullViewer
#define SoGUIViewer SoQtViewer
#define SoGUIPopupMenu SoQtPopupMenu
#include <Inventor/Qt/widgets/SoQtPopupMenu.h>
#include <qstring.h>
#include <qfiledialog.h>
#include <qfontdialog.h>
#include <qlayout.h> 
#include <qslider.h> 
#include <qpushbutton.h> 
#include <qlabel.h> 
#endif

#ifdef __SoXt__
#define GUIWidget Widget
#define SoGUIExaminerViewer SoXtExaminerViewer
#define SoGUIFullViewer SoXtFullViewer
#define SoGUIViewer SoXtViewer
#define SoGUIPopupMenu SoXtPopupMenu
#include <Inventor/Xt/widgets/SoXtPopupMenu.h>
#endif


#ifdef __SoGtk__
#define GUIWidget GtkWidget*
#define SoGUIExaminerViewer SoGtkExaminerViewer
#define SoGUIFullViewer SoGtkFullViewer
#define SoGUIViewer SoGtkViewer
#define SoGUIPopupMenu SoGtkPopupMenu
#include <gtk/gtk.h>
#include <Inventor/Gtk/widgets/SoGtkPopupMenu.h>
#endif


#ifdef __SoWin__
#define GUIWidget HWND
#define SoGUIExaminerViewer SoWinExaminerViewer
#define SoGUIFullViewer SoWinFullViewer
#define SoGUIViewer SoWinViewer
#define SoGUIPopupMenu SoWinPopupMenu
#include <Inventor/Win/widgets/SoWinPopupMenu.h>
#endif

#ifdef __SoFl__
#define GUIWidget Fl_Window*
#define SoGUIExaminerViewer SoFlExaminerViewer
#define SoGUIFullViewer SoFlFullViewer
#define SoGUIViewer SoFlViewer
#include <Inventor/FL/viewers/SoFlVrUI.h>
#include <FL/Fl_File_Chooser.H>

#include "OpenMOIVUI.h"
#endif

#include <ChemKit2/ChemDisplay.h>
#include <ChemKit2/ChemRadii.h>
#include <ChemKit2/ChemColor.h>
#include <ChemKit2/ChemDisplayParam.h>
#include <ChemKit2/ChemData.h>
#include <ChemKit2/ChemAssociatedData.h>

#include <ChemKit2/util/ChemMOLImporter.h>
#include <ChemKit2/util/ChemPDBImporter.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


ChemExaminerViewer::ChemExaminerViewer(GUIWidget parent) //,
  //    const char * name)
  //,
  //    SbBool embed,
  //    SoGUIFullViewer::BuildFlag flag,
  //    SoGUIViewer::Type type,
  //	SbBool build)
  : SoGUIExaminerViewer(parent)//,name,embed,flag,type,build)
{
	cout << "not ok\n";

	addPopupMenuItems();

	bondisplaystyle = 0;
	cap = 0;

}


void ChemExaminerViewer::setNodes(ChemData *chemdata,ChemAssociatedData *chemassociateddata,
								  ChemRadii* cr,ChemColor* cl,
								  ChemDisplayParam* cdp,ChemDisplay* cd)
{
	this->chemdata = chemdata;
	this->chemassociateddata = chemassociateddata;
	this->cr = cr;
	this->cl = cl;
	this->cdp = cdp;
	this->cd = cd;

	char * filename = "example.pdb";
	ChemPDBImporter input;
	chemdata->reset();
	input.openFile(filename,chemdata,chemassociateddata);
	
	// fill residue color table
	cl->residueColor.setNum(_RESNUM);
	cl->residueColor.setValues(0,_RESNUM,hydroCol);
	cl->residueLabelColor.setNum(_RESNUM);
	cl->residueLabelColor.setValues(0,_RESNUM,hydroCol);
}

void ChemExaminerViewer::addPopupMenuItems()
{
#ifndef __SoFl__
	SoGUIPopupMenu* menu = this->prefmenu;
	
	menu->addSeparator(0);

	//menu->newMenu("Molecular",MOLECULAR_MENU);
	//menu->setMenuTitle(MOLECULAR_MENU, ("Molecular"));
  
	//menu->addMenu(0, MOLECULAR_MENU);//add to root
#define MOLECULAR_MENU 0

	menu->newMenuItem("Open Molecule...",MDLFILE_ITEM);
	menu->setMenuItemTitle(MDLFILE_ITEM, ("Open Molecule..."));
	menu->addMenuItem(MOLECULAR_MENU,MDLFILE_ITEM);

	menu->newMenuItem("Setup Font...",FONTDLG_ITEM);
  menu->setMenuItemTitle(FONTDLG_ITEM, ("Setup font..."));
	menu->addMenuItem(MOLECULAR_MENU,FONTDLG_ITEM);

	menu->newMenuItem("Setup Complexity...",COMPLEXITY_ITEM);
  menu->setMenuItemTitle(COMPLEXITY_ITEM, ("Setup Complexity..."));

	// 3. Display styles menu
	
	menu->newMenu("Draw Styles",DRAWSTYLE_MENU);
  menu->setMenuTitle(DRAWSTYLE_MENU, ("Draw Styles"));
	menu->addMenu(MOLECULAR_MENU,DRAWSTYLE_MENU);

	menu->newMenuItem("Display None",DRAWSTYLE_DISPLAY_NONE_ITEM);
  menu->setMenuItemTitle(DRAWSTYLE_DISPLAY_NONE_ITEM, ("Display None"));
	menu->addMenuItem(DRAWSTYLE_MENU,DRAWSTYLE_DISPLAY_NONE_ITEM);

	menu->newMenuItem("Display CPK",DRAWSTYLE_DISPLAY_CPK_ITEM);
  menu->setMenuItemTitle(DRAWSTYLE_DISPLAY_CPK_ITEM, ("Display CPK"));
	menu->addMenuItem(DRAWSTYLE_MENU,DRAWSTYLE_DISPLAY_CPK_ITEM);

	menu->newMenuItem("Display Ball Stick",DRAWSTYLE_DISPLAY_BALLSTICK_ITEM);
  menu->setMenuItemTitle(DRAWSTYLE_DISPLAY_BALLSTICK_ITEM, ("Display Ball Stick"));
	menu->addMenuItem(DRAWSTYLE_MENU,DRAWSTYLE_DISPLAY_BALLSTICK_ITEM);

	menu->newMenuItem("Display Stick",DRAWSTYLE_DISPLAY_STICK_ITEM);
  menu->setMenuItemTitle(DRAWSTYLE_DISPLAY_STICK_ITEM, ("Display Stick"));
	menu->addMenuItem(DRAWSTYLE_MENU,DRAWSTYLE_DISPLAY_STICK_ITEM);

	menu->newMenuItem("Display Wireframe",DRAWSTYLE_DISPLAY_WIREFRAME_ITEM);
  menu->setMenuItemTitle(DRAWSTYLE_DISPLAY_WIREFRAME_ITEM, ("Display Wireframe"));
	menu->addMenuItem(DRAWSTYLE_MENU,DRAWSTYLE_DISPLAY_WIREFRAME_ITEM);

	menu->newMenuItem("Display Ballwire",DRAWSTYLE_DISPLAY_BALLWIRE_ITEM);
  menu->setMenuItemTitle(DRAWSTYLE_DISPLAY_BALLWIRE_ITEM, ("Display Ballwire"));
	menu->addMenuItem(DRAWSTYLE_MENU,DRAWSTYLE_DISPLAY_BALLWIRE_ITEM);

	// Fog menu
	menu->newMenu("Fog",FOG_MENU);
  menu->setMenuTitle(FOG_MENU, ("Fog"));
	menu->addMenu(MOLECULAR_MENU, FOG_MENU);

	menu->newMenuItem("Setup Fog...",FOGDLG_ITEM);
  menu->setMenuItemTitle(FOGDLG_ITEM, ("Setup Fog..."));
	menu->addMenuItem(FOG_MENU,FOGDLG_ITEM);

	menu->newMenuItem("Fog None",FOGNONE_ITEM);
  menu->setMenuItemTitle(FOGNONE_ITEM, ("Fog none"));
	menu->addMenuItem(FOG_MENU,FOGNONE_ITEM);

	menu->newMenuItem("Fog Haze",FOGHAZE_ITEM);
  menu->setMenuItemTitle(FOGHAZE_ITEM, ("Fog Haze"));
	menu->addMenuItem(FOG_MENU,FOGHAZE_ITEM);

	menu->newMenuItem("Fog Fog",FOGFOG_ITEM);
  menu->setMenuItemTitle(FOGFOG_ITEM, ("Fog Fog"));
	menu->addMenuItem(FOG_MENU,FOGFOG_ITEM);

	menu->newMenuItem("Fog Smoke",FOGSMOKE_ITEM);
  menu->setMenuItemTitle(FOGSMOKE_ITEM, ("Fog Smoke"));
	menu->addMenuItem(FOG_MENU,FOGSMOKE_ITEM);

	// Atoms menu

	menu->newMenu("Atoms",ATOMS_MENU);
  menu->setMenuTitle(ATOMS_MENU, ("Atoms"));
	menu->addMenu(MOLECULAR_MENU, ATOMS_MENU);

	
	// SPHERE DISPLAY STYLE
	menu->newMenu("ASDS",ASDS_MENU);
  menu->setMenuTitle(ASDS_MENU, ("Atom Sphere Display Style"));
	menu->addMenu(ATOMS_MENU, ASDS_MENU);

	menu->addMenuItem(ATOMS_MENU,COMPLEXITY_ITEM);

	// atom items

	menu->newMenuItem("Hemisphere",ATOM_HEMISPHERE_ITEM);
  menu->setMenuItemTitle(ATOM_HEMISPHERE_ITEM, ("Hemisphere"));
	menu->addMenuItem(ASDS_MENU,ATOM_HEMISPHERE_ITEM);

	menu->newMenuItem("Fullsphere",ATOM_FULLSPHERE_ITEM);
  menu->setMenuItemTitle(ATOM_FULLSPHERE_ITEM, ("Fullsphere"));
	menu->addMenuItem(ASDS_MENU,ATOM_FULLSPHERE_ITEM);

	menu->newMenuItem("glDrawArrays Hemisphere",ATOM_ARRAYS_HEMISPHERE_ITEM);
  menu->setMenuItemTitle(ATOM_ARRAYS_HEMISPHERE_ITEM, ("glDrawArrays Hemisphere"));
	menu->addMenuItem(ASDS_MENU,ATOM_ARRAYS_HEMISPHERE_ITEM);

	menu->newMenuItem("glDrawArrays Fullsphere",ATOM_ARRAYS_FULLSPHERE_ITEM);
  menu->setMenuItemTitle(ATOM_ARRAYS_FULLSPHERE_ITEM, ("glDrawArrays Fullsphere"));
	menu->addMenuItem(ASDS_MENU,ATOM_ARRAYS_FULLSPHERE_ITEM);

	menu->newMenuItem("gluSphere",ATOM_GLUSPHERE_ITEM);
  menu->setMenuItemTitle(ATOM_GLUSPHERE_ITEM, ("gluSphere"));
	menu->addMenuItem(ASDS_MENU,ATOM_GLUSPHERE_ITEM);

	menu->newMenuItem("SoSphere",ATOM_SOSPHERE_ITEM);
  menu->setMenuItemTitle(ATOM_SOSPHERE_ITEM, ("SoSphere"));
	menu->addMenuItem(ASDS_MENU,ATOM_SOSPHERE_ITEM);

	menu->newMenuItem("Billboard",ATOM_BILLBOARD_ITEM);
  menu->setMenuItemTitle(ATOM_BILLBOARD_ITEM, ("Billboard"));
	menu->addMenuItem(ASDS_MENU,ATOM_BILLBOARD_ITEM);

	menu->newMenuItem("Fast Level Of Detail",ATOM_LOD_ITEM);
  menu->setMenuItemTitle(ATOM_LOD_ITEM, ("Fast Level Of Detail"));
	menu->addMenuItem(ASDS_MENU,ATOM_LOD_ITEM);

	menu->newMenuItem("Level of Detail (Geometry)",ATOM_LOD_GEOMETRY_ITEM);
  menu->setMenuItemTitle(ATOM_LOD_GEOMETRY_ITEM, ("Level Of Detail (Geometry)"));
	menu->addMenuItem(ASDS_MENU,ATOM_LOD_GEOMETRY_ITEM);

	//  bonds menu
	menu->newMenu("Bonds",BONDS_MENU);
  menu->setMenuTitle(BONDS_MENU, ("Bonds"));
	menu->addMenu(MOLECULAR_MENU,BONDS_MENU);

	// BOND CYLINDER DISPLAY STYLE

	menu->newMenu("Bond Cylinder Display Style",BCDS_MENU);
  menu->setMenuTitle(BCDS_MENU, ("Bond Cylinder Display Style"));
	menu->addMenu(BONDS_MENU,BCDS_MENU);

	menu->addMenuItem(BONDS_MENU,COMPLEXITY_ITEM);

	// bonds items

	menu->newMenuItem("Cylinder",BOND_CYLINDER_ITEM);
  menu->setMenuItemTitle(BOND_CYLINDER_ITEM, ("Cylinder"));
	menu->addMenuItem(BCDS_MENU,BOND_CYLINDER_ITEM);

	menu->newMenuItem("glDrawArrays",BOND_ARRAYS_CYLINDER_ITEM);
  menu->setMenuItemTitle(BOND_ARRAYS_CYLINDER_ITEM, ("glDrawArrays"));
	menu->addMenuItem(BCDS_MENU,BOND_ARRAYS_CYLINDER_ITEM);

	menu->newMenuItem("gluCylinder",BOND_GLUCYLINDER_ITEM);
  menu->setMenuItemTitle(BOND_GLUCYLINDER_ITEM, ("gluCylinder"));
	menu->addMenuItem(BCDS_MENU,BOND_GLUCYLINDER_ITEM);

	menu->newMenuItem("SoCylinder",BOND_SOCYLINDER_ITEM);
  menu->setMenuItemTitle(BOND_SOCYLINDER_ITEM, ("SoCylinder"));
	menu->addMenuItem(BCDS_MENU,BOND_SOCYLINDER_ITEM);

	menu->newMenuItem("SemiCylinder",BOND_SEMI_ITEM);
  menu->setMenuItemTitle(BOND_SEMI_ITEM, ("SemiCylinder"));
	menu->addMenuItem(BCDS_MENU,BOND_SEMI_ITEM);

	menu->newMenuItem("Level Of Detail",BOND_LOD_ITEM);
  menu->setMenuItemTitle(BOND_LOD_ITEM, ("Level Of Detail"));
	menu->addMenuItem(BCDS_MENU,BOND_LOD_ITEM);

		// cap menu
	menu->newMenu("Cap",CAP_MENU);
  menu->setMenuTitle(CAP_MENU, ("Cap"));
	menu->addMenu(BONDS_MENU,CAP_MENU);

	// cap items

	menu->newMenuItem("No cap",CAP_NOCAP_ITEM);
  menu->setMenuItemTitle(CAP_NOCAP_ITEM, ("No Cap"));
	menu->addMenuItem(CAP_MENU,CAP_NOCAP_ITEM);

	menu->newMenuItem("Flat cap",CAP_FLATCAP_ITEM);
  menu->setMenuItemTitle(CAP_FLATCAP_ITEM, ("Flat Cap"));
	menu->addMenuItem(CAP_MENU,CAP_FLATCAP_ITEM);

	menu->newMenuItem("Round cap",CAP_ROUNDCAP_ITEM);
  menu->setMenuItemTitle(CAP_ROUNDCAP_ITEM, ("Round Cap"));
	menu->addMenuItem(CAP_MENU,CAP_ROUNDCAP_ITEM);

	// residue menu

	menu->newMenu("Residues",RESIDUES_MENU);
  menu->setMenuTitle(RESIDUES_MENU, ("Residues"));
	menu->addMenu(MOLECULAR_MENU,RESIDUES_MENU);

	// residue display style

	menu->newMenu("RDS",RDS_MENU);
  menu->setMenuTitle(RDS_MENU, ("Residue Display Style"));
	menu->addMenu(RESIDUES_MENU,RDS_MENU);

	// residue items

	menu->newMenuItem("None",RESIDUE_NONE_ITEM);
  menu->setMenuItemTitle(RESIDUE_NONE_ITEM, ("None"));
	menu->addMenuItem(RDS_MENU,RESIDUE_NONE_ITEM);

	menu->newMenuItem("CAwire",RESIDUE_CAWIRE_ITEM);
  menu->setMenuItemTitle(RESIDUE_CAWIRE_ITEM, ("CAwire"));
	menu->addMenuItem(RDS_MENU,RESIDUE_CAWIRE_ITEM);

	menu->newMenuItem("CAstick",RESIDUE_CASTICK_ITEM);
  menu->setMenuItemTitle(RESIDUE_CASTICK_ITEM, ("CAstick"));
	menu->addMenuItem(RDS_MENU,RESIDUE_CASTICK_ITEM);
	
	menu->newMenuItem("Line ribbon",RESIDUE_LINERIBBON_ITEM);
  menu->setMenuItemTitle(RESIDUE_LINERIBBON_ITEM, ("Line Ribbon"));
	menu->addMenuItem(RDS_MENU,RESIDUE_LINERIBBON_ITEM);

	menu->newMenuItem("Flat ribbon",RESIDUE_FLATRIBBON_ITEM);
  menu->setMenuItemTitle(RESIDUE_FLATRIBBON_ITEM, ("Flat Ribbon"));
	menu->addMenuItem(RDS_MENU,RESIDUE_FLATRIBBON_ITEM);

	menu->newMenuItem("Solid ribbon",RESIDUE_SOLIDRIBBON_ITEM);
  menu->setMenuItemTitle(RESIDUE_SOLIDRIBBON_ITEM, ("Solid Ribbon"));
	menu->addMenuItem(RDS_MENU,RESIDUE_SOLIDRIBBON_ITEM);

	menu->newMenuItem("Schematic",RESIDUE_SCHEMATIC_ITEM);
  menu->setMenuItemTitle(RESIDUE_SCHEMATIC_ITEM, ("Schematic"));
	menu->addMenuItem(RDS_MENU,RESIDUE_SCHEMATIC_ITEM);

	// COLOR menu
	menu->newMenu("Color",COLOR_ATOMS_MENU);
  menu->setMenuTitle(COLOR_ATOMS_MENU, ("Color"));
	menu->addMenu(ATOMS_MENU,COLOR_ATOMS_MENU);

	// color items
	menu->newMenuItem("CI",CI_ITEM);
	menu->setMenuItemTitle(CI_ITEM, ("Color Indexed"));
	menu->addMenuItem(COLOR_ATOMS_MENU,CI_ITEM);

	menu->newMenuItem("CA",CA_ITEM);
	menu->setMenuItemTitle(CA_ITEM, ("Color Per Atom"));
	menu->addMenuItem(COLOR_ATOMS_MENU,CA_ITEM);

	menu->newMenuItem("CO",CO_ITEM);
	menu->setMenuItemTitle(CO_ITEM, ("Color Overall"));
	menu->addMenuItem(COLOR_ATOMS_MENU,CO_ITEM);

	// RADII menu
	menu->newMenu("Radii",RADII_ATOMS_MENU);
  menu->setMenuTitle(RADII_ATOMS_MENU, ("Radii"));
	menu->addMenu(ATOMS_MENU,RADII_ATOMS_MENU);

	// radii items
	menu->newMenuItem("RI",RI_ITEM);
	menu->setMenuItemTitle(RI_ITEM, ("Radii Indexed"));
	menu->addMenuItem(RADII_ATOMS_MENU,RI_ITEM);

	menu->newMenuItem("RA",RA_ITEM);
	menu->setMenuItemTitle(RA_ITEM, ("Radii Per Atom"));
	menu->addMenuItem(RADII_ATOMS_MENU,RA_ITEM);

	menu->newMenuItem("RO",RO_ITEM);
	menu->setMenuItemTitle(RO_ITEM, ("Radii Overall"));
	menu->addMenuItem(RADII_ATOMS_MENU,RO_ITEM);

	// cylinder color menu

	menu->newMenu("Bond color",COLOR_BOND_MENU);
  menu->setMenuTitle(COLOR_BOND_MENU, ("Color"));
	menu->addMenu(BONDS_MENU,COLOR_BOND_MENU);

	// cylinder color items
	menu->newMenuItem("OV",OV_ITEM);
	menu->setMenuItemTitle(OV_ITEM, ("Bond Overall"));
	menu->addMenuItem(COLOR_BOND_MENU,OV_ITEM);

	menu->newMenuItem("BB",BB_ITEM);
	menu->setMenuItemTitle(BB_ITEM, ("Bond Per Bond"));
	menu->addMenuItem(COLOR_BOND_MENU,BB_ITEM);

	menu->newMenuItem("BI",BI_ITEM);
	menu->setMenuItemTitle(BI_ITEM, ("Bond Per Bond Indexed"));
	menu->addMenuItem(COLOR_BOND_MENU,BI_ITEM);

	menu->newMenuItem("BA",BA_ITEM);
	menu->setMenuItemTitle(BA_ITEM, ("Bond Per Atom"));
	menu->addMenuItem(COLOR_BOND_MENU,BA_ITEM);

	menu->newMenuItem("BAH",BAH_ITEM);
	menu->setMenuItemTitle(BAH_ITEM, ("Bond Per Atom Half-bonded"));
	menu->addMenuItem(COLOR_BOND_MENU,BAH_ITEM);

	// residues color menu
	menu->newMenu("Residues color",COLOR_RESIDUES_MENU);
  menu->setMenuTitle(COLOR_RESIDUES_MENU, ("Color"));
	menu->addMenu(RESIDUES_MENU,COLOR_RESIDUES_MENU);

	// residues color items

	menu->newMenuItem("ROV",ROV_ITEM);
	menu->setMenuItemTitle(ROV_ITEM, ("Residues Overall"));
	menu->addMenuItem(COLOR_RESIDUES_MENU,ROV_ITEM);

	menu->newMenuItem("RPI",RPI_ITEM);
	menu->setMenuItemTitle(RPI_ITEM, ("Residues Per Index"));
	menu->addMenuItem(COLOR_RESIDUES_MENU,RPI_ITEM);

	menu->newMenuItem("RPC",RPC_ITEM);
	menu->setMenuItemTitle(RPC_ITEM, ("Residues Per Chain"));
	menu->addMenuItem(COLOR_RESIDUES_MENU,RPC_ITEM);

	// ATOM LABELS COLOR MENU
	menu->newMenu("Labels",ATOM_LABEL_COLOR_MENU);
  menu->setMenuTitle(ATOM_LABEL_COLOR_MENU, ("Labels"));
	menu->addMenu(ATOMS_MENU,ATOM_LABEL_COLOR_MENU);

	menu->newMenuItem("Atom Label Overall",ALO_ITEM);
  menu->setMenuItemTitle(ALO_ITEM, ("Atom Label Overall"));
	menu->addMenuItem(ATOM_LABEL_COLOR_MENU,ALO_ITEM);

	menu->newMenuItem("Atom Label Per Atom",ALPA_ITEM);
  menu->setMenuItemTitle(ALPA_ITEM, ("Atom Label Per Atom"));
	menu->addMenuItem(ATOM_LABEL_COLOR_MENU,ALPA_ITEM);

	menu->newMenuItem("Atom Label Per Atom Label",ALPAL_ITEM);
  menu->setMenuItemTitle(ALPAL_ITEM, ("Atom Label Per Atom Label"));
	menu->addMenuItem(ATOM_LABEL_COLOR_MENU,ALPAL_ITEM);

	menu->newMenuItem("Atom Label Per Atom Label Indexed",ALPALI_ITEM);
  menu->setMenuItemTitle(ALPALI_ITEM, ("Atom Label Per Atom Label Indexed"));
	menu->addMenuItem(ATOM_LABEL_COLOR_MENU,ALPALI_ITEM);

	// BOND LABELS COLOR MENU
	menu->newMenu("Labels",BOND_LABEL_COLOR_MENU);
  menu->setMenuTitle(BOND_LABEL_COLOR_MENU, ("Labels"));
	menu->addMenu(BONDS_MENU,BOND_LABEL_COLOR_MENU);

	menu->newMenuItem("Bond Label Overall",BLO_ITEM);
  menu->setMenuItemTitle(BLO_ITEM, ("Bond Label Overall"));
	menu->addMenuItem(BOND_LABEL_COLOR_MENU,BLO_ITEM);

	menu->newMenuItem("Bond Label Per Bond",BLPB_ITEM);
  menu->setMenuItemTitle(BLPB_ITEM, ("Bond Label Per Bond"));
	menu->addMenuItem(BOND_LABEL_COLOR_MENU,BLPB_ITEM);

	menu->newMenuItem("Bond Label Per Bond Label",BLPBL_ITEM);
  menu->setMenuItemTitle(BLPBL_ITEM, ("Bond Label Per Bond Label"));
	menu->addMenuItem(BOND_LABEL_COLOR_MENU,BLPBL_ITEM);

	menu->newMenuItem("Bond Label Per Bond Label Indexed",BLPBLI_ITEM);
  menu->setMenuItemTitle(BLPBLI_ITEM, ("Bond Label Per Bond Label Indexed"));
	menu->addMenuItem(BOND_LABEL_COLOR_MENU,BLPBLI_ITEM);

	// RESIDUE LABELS COLOR MENU
	menu->newMenu("Labels",RESIDUE_LABEL_COLOR_MENU);
  menu->setMenuTitle(RESIDUE_LABEL_COLOR_MENU, ("Labels"));
	menu->addMenu(RESIDUES_MENU,RESIDUE_LABEL_COLOR_MENU);

	menu->newMenuItem("Residue Label Overall",RLO_ITEM);
  menu->setMenuItemTitle(RLO_ITEM, ("Residue Label Overall"));
	menu->addMenuItem(RESIDUE_LABEL_COLOR_MENU,RLO_ITEM);

	menu->newMenuItem("Residue Label Per Index",RLPI_ITEM);
  menu->setMenuItemTitle(RLPI_ITEM, ("Residue Label Per Index"));
	menu->addMenuItem(RESIDUE_LABEL_COLOR_MENU,RLPI_ITEM);

	menu->newMenuItem("Residue Label Per Chain",RLPC_ITEM);
  menu->setMenuItemTitle(RLPC_ITEM, ("Residue Label Per Chain"));
	menu->addMenuItem(RESIDUE_LABEL_COLOR_MENU,RLPC_ITEM);

	menu->newMenuItem("Display Atom Label",LABEL_DISPLAY_ATOMLABEL_ITEM);
  menu->setMenuItemTitle(LABEL_DISPLAY_ATOMLABEL_ITEM, ("Display Labels"));
	menu->addMenuItem(ATOMS_MENU,LABEL_DISPLAY_ATOMLABEL_ITEM);

	menu->newMenuItem("Display Bond Label",LABEL_DISPLAY_BONDLABEL_ITEM);
  menu->setMenuItemTitle(LABEL_DISPLAY_BONDLABEL_ITEM, ("Display Labels"));
	menu->addMenuItem(BONDS_MENU,LABEL_DISPLAY_BONDLABEL_ITEM);

	menu->newMenuItem("Display Residues Label",LABEL_DISPLAY_RESIDUELABEL_ITEM);
  menu->setMenuItemTitle(LABEL_DISPLAY_RESIDUELABEL_ITEM, ("Display Labels"));
	menu->addMenuItem(RESIDUES_MENU,LABEL_DISPLAY_RESIDUELABEL_ITEM);

	menu->newMenuItem("Clip Atoms",CLIPPING_ATOM_ITEM);
  menu->setMenuItemTitle(CLIPPING_ATOM_ITEM, ("Clip Atoms"));
	menu->addMenuItem(ATOMS_MENU,CLIPPING_ATOM_ITEM);

	menu->newMenuItem("Clip Bonds",CLIPPING_BOND_ITEM);
  menu->setMenuItemTitle(CLIPPING_BOND_ITEM, ("Clip Bonds"));
	menu->addMenuItem(BONDS_MENU,CLIPPING_BOND_ITEM);

	menu->newMenuItem("Interpolate color",ICOLOR_ITEM);
	menu->setMenuItemTitle(ICOLOR_ITEM, ("Interpolate Color"));
	menu->addMenuItem(RESIDUES_MENU,ICOLOR_ITEM);

	// radio groups
	menu->newRadioGroup(ATOM_GROUP);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_HEMISPHERE_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_FULLSPHERE_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_ARRAYS_HEMISPHERE_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_ARRAYS_FULLSPHERE_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_BILLBOARD_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_GLUSPHERE_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_SOSPHERE_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_LOD_ITEM);
	menu->addRadioGroupItem(ATOM_GROUP, ATOM_LOD_GEOMETRY_ITEM);
	
	menu->newRadioGroup(BOND_GROUP);
	menu->addRadioGroupItem(BOND_GROUP, BOND_CYLINDER_ITEM);
	menu->addRadioGroupItem(BOND_GROUP, BOND_ARRAYS_CYLINDER_ITEM);
	menu->addRadioGroupItem(BOND_GROUP, BOND_GLUCYLINDER_ITEM);
	menu->addRadioGroupItem(BOND_GROUP, BOND_SOCYLINDER_ITEM);
	menu->addRadioGroupItem(BOND_GROUP, BOND_SEMI_ITEM);
	menu->addRadioGroupItem(BOND_GROUP, BOND_LOD_ITEM);

	menu->newRadioGroup(CAP_GROUP);
	menu->addRadioGroupItem(CAP_GROUP, CAP_NOCAP_ITEM);
	menu->addRadioGroupItem(CAP_GROUP, CAP_FLATCAP_ITEM);
	menu->addRadioGroupItem(CAP_GROUP, CAP_ROUNDCAP_ITEM);

	menu->newRadioGroup(RESIDUE_GROUP);
	menu->addRadioGroupItem(RESIDUE_GROUP, RESIDUE_NONE_ITEM);
	menu->addRadioGroupItem(RESIDUE_GROUP, RESIDUE_CAWIRE_ITEM);
	menu->addRadioGroupItem(RESIDUE_GROUP, RESIDUE_CASTICK_ITEM);
	menu->addRadioGroupItem(RESIDUE_GROUP, RESIDUE_LINERIBBON_ITEM);
	menu->addRadioGroupItem(RESIDUE_GROUP, RESIDUE_FLATRIBBON_ITEM);
	menu->addRadioGroupItem(RESIDUE_GROUP, RESIDUE_SOLIDRIBBON_ITEM);
	menu->addRadioGroupItem(RESIDUE_GROUP, RESIDUE_SCHEMATIC_ITEM);

	menu->newRadioGroup(COLOR_GROUP);
	menu->addRadioGroupItem(COLOR_GROUP, CI_ITEM);
	menu->addRadioGroupItem(COLOR_GROUP, CA_ITEM);
	menu->addRadioGroupItem(COLOR_GROUP, CO_ITEM);

	menu->newRadioGroup(RADII_GROUP);
	menu->addRadioGroupItem(RADII_GROUP, RI_ITEM);
	menu->addRadioGroupItem(RADII_GROUP, RA_ITEM);
	menu->addRadioGroupItem(RADII_GROUP, RO_ITEM);

	menu->newRadioGroup(COLOR_BOND_GROUP);
	menu->addRadioGroupItem(COLOR_BOND_GROUP, OV_ITEM);
	menu->addRadioGroupItem(COLOR_BOND_GROUP, BA_ITEM);
	menu->addRadioGroupItem(COLOR_BOND_GROUP, BI_ITEM);
	menu->addRadioGroupItem(COLOR_BOND_GROUP, BB_ITEM);
	menu->addRadioGroupItem(COLOR_BOND_GROUP, BAH_ITEM);

	menu->newRadioGroup(COLOR_RESIDUES_GROUP);
	menu->addRadioGroupItem(COLOR_RESIDUES_GROUP, ROV_ITEM);
	menu->addRadioGroupItem(COLOR_RESIDUES_GROUP, RPI_ITEM);
	menu->addRadioGroupItem(COLOR_RESIDUES_GROUP, RPC_ITEM);

	menu->newRadioGroup(DRAWSTYLE_GROUP);
	menu->addRadioGroupItem(DRAWSTYLE_GROUP, DRAWSTYLE_DISPLAY_NONE_ITEM);
	menu->addRadioGroupItem(DRAWSTYLE_GROUP, DRAWSTYLE_DISPLAY_CPK_ITEM);
	menu->addRadioGroupItem(DRAWSTYLE_GROUP, DRAWSTYLE_DISPLAY_BALLSTICK_ITEM);
	menu->addRadioGroupItem(DRAWSTYLE_GROUP, DRAWSTYLE_DISPLAY_STICK_ITEM);
	menu->addRadioGroupItem(DRAWSTYLE_GROUP, DRAWSTYLE_DISPLAY_WIREFRAME_ITEM);
	menu->addRadioGroupItem(DRAWSTYLE_GROUP, DRAWSTYLE_DISPLAY_BALLWIRE_ITEM);

	menu->newRadioGroup(ATOM_LABEL_COLOR_GROUP);
	menu->addRadioGroupItem(ATOM_LABEL_COLOR_GROUP, ALO_ITEM);
	menu->addRadioGroupItem(ATOM_LABEL_COLOR_GROUP, ALPA_ITEM);
	menu->addRadioGroupItem(ATOM_LABEL_COLOR_GROUP, ALPAL_ITEM);
	menu->addRadioGroupItem(ATOM_LABEL_COLOR_GROUP, ALPALI_ITEM);

	menu->newRadioGroup(BOND_LABEL_COLOR_GROUP);
	menu->addRadioGroupItem(BOND_LABEL_COLOR_GROUP, BLO_ITEM);
	menu->addRadioGroupItem(BOND_LABEL_COLOR_GROUP, BLPB_ITEM);
	menu->addRadioGroupItem(BOND_LABEL_COLOR_GROUP, BLPBL_ITEM);
	menu->addRadioGroupItem(BOND_LABEL_COLOR_GROUP, BLPBLI_ITEM);
	
	menu->newRadioGroup(RESIDUE_LABEL_COLOR_GROUP);
	menu->addRadioGroupItem(RESIDUE_LABEL_COLOR_GROUP, RLO_ITEM);
	menu->addRadioGroupItem(RESIDUE_LABEL_COLOR_GROUP, RLPI_ITEM);
	menu->addRadioGroupItem(RESIDUE_LABEL_COLOR_GROUP, RLPC_ITEM);

	menu->newRadioGroup(FOG_GROUP);
	menu->addRadioGroupItem(FOG_GROUP, FOGNONE_ITEM);
	menu->addRadioGroupItem(FOG_GROUP, FOGHAZE_ITEM);
	menu->addRadioGroupItem(FOG_GROUP, FOGFOG_ITEM);
	menu->addRadioGroupItem(FOG_GROUP, FOGSMOKE_ITEM);

	menu->addMenuSelectionCallback(ChemExaminerViewer::chemSelectionCallback,(void *) this);
	
#undef MOLECULAR_MENU
#endif

#ifdef __SoFl__
//	
	ViewerInterface* ui = getViewerUI();
	Fl_Menu_Button* menu = ui->vmPopup;//getPopupMenu();
	//viewerInterface->setDecoration(0);

	ui->user_data(this);

	menu->textfont(0);
	menu->textsize(12);
	
	int flags = 0;
	int idx = menu->add("&Open Molecule...", 0, (Fl_Callback*)chemSelectionCallback, 
		(void*)MDLFILE_ITEM,flags);

	menu->mode(idx-1,FL_MENU_DIVIDER);//add separator after preferences item

	menu->add("Setup fon&t...", 0, (Fl_Callback*)chemSelectionCallback, 
		(void*)FONTDLG_ITEM,flags);

	menu->add("Setup compl&exity...", 0, (Fl_Callback*)chemSelectionCallback, 
		(void*)COMPLEXITY_ITEM,flags);

	menu->add("Draw styles/Display None",0,(Fl_Callback*)chemSelectionCallback,
		(void*)DRAWSTYLE_DISPLAY_NONE_ITEM,FL_MENU_RADIO);
	menu->add("Draw styles/Display CPK",0,(Fl_Callback*)chemSelectionCallback,
		(void*)DRAWSTYLE_DISPLAY_CPK_ITEM,FL_MENU_RADIO);
	menu->add("Draw styles/Display Ballstick",0,(Fl_Callback*)chemSelectionCallback,
		(void*)DRAWSTYLE_DISPLAY_BALLSTICK_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Draw styles/Display Wireframe",0,(Fl_Callback*)chemSelectionCallback,
		(void*)DRAWSTYLE_DISPLAY_WIREFRAME_ITEM,FL_MENU_RADIO);
	menu->add("Draw styles/Display Stick",0,(Fl_Callback*)chemSelectionCallback,
		(void*)DRAWSTYLE_DISPLAY_STICK_ITEM,FL_MENU_RADIO);
	menu->add("Draw styles/Display Ballwire",0,(Fl_Callback*)chemSelectionCallback,
		(void*)DRAWSTYLE_DISPLAY_BALLWIRE_ITEM,FL_MENU_RADIO);

	menu->add("Fog/Setup Fog...",0,(Fl_Callback*)chemSelectionCallback,
		(void*)FOGDLG_ITEM,flags);

	menu->add("Fog/Fog None",0,(Fl_Callback*)chemSelectionCallback,
		(void*)FOGDLG_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Fog/Fog Haze",0,(Fl_Callback*)chemSelectionCallback,
		(void*)FOGHAZE_ITEM,FL_MENU_RADIO);
	menu->add("Fog/Fog Fog",0,(Fl_Callback*)chemSelectionCallback,
		(void*)FOGFOG_ITEM,FL_MENU_RADIO);
	menu->add("Fog/Fog Smoke",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)FOGSMOKE_ITEM,FL_MENU_RADIO);

	menu->add("Atoms/Atom Sphere Display Style/Hemisphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_HEMISPHERE_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Atom Sphere Display Style/Hemisphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_HEMISPHERE_ITEM,FL_MENU_RADIO);	
	menu->add("Atoms/Atom Sphere Display Style/Fullsphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_FULLSPHERE_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Atom Sphere Display Style/glDrawArrays Hemisphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_ARRAYS_HEMISPHERE_ITEM,FL_MENU_RADIO);	
	menu->add("Atoms/Atom Sphere Display Style/glDrawArrays Fullsphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_ARRAYS_FULLSPHERE_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Atom Sphere Display Style/gluSphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_GLUSPHERE_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Atom Sphere Display Style/gluSphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_GLUSPHERE_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Atom Sphere Display Style/SoSphere",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_SOSPHERE_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Atom Sphere Display Style/Billboard",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_BILLBOARD_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Atom Sphere Display Style/Fast Level Of Detail",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_LOD_ITEM,FL_MENU_RADIO);
	idx = menu->add("Atoms/Atom Sphere Display Style/Level Of Detail (Geometry)",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ATOM_LOD_GEOMETRY_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);

	menu->add("Bonds/Bond Cylinder Display Style/Cylinder",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BOND_CYLINDER_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Bond Cylinder Display Style/glDrawArrays",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BOND_ARRAYS_CYLINDER_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Bond Cylinder Display Style/gluCylinder",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BOND_GLUCYLINDER_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Bond Cylinder Display Style/SoCylinder",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BOND_SOCYLINDER_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Bond Cylinder Display Style/SemiCylinder",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BOND_SEMI_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Bond Cylinder Display Style/Level Of Detail",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BOND_LOD_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);

	menu->add("Bonds/Cap/No cap",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)CAP_NOCAP_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Bonds/Cap/Flat cap",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)CAP_FLATCAP_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Cap/Round cap",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)CAP_ROUNDCAP_ITEM,FL_MENU_RADIO);
		
	menu->add("Residues/Residue Display Style/None",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RESIDUE_NONE_ITEM,FL_MENU_RADIO);
	menu->add("Residues/Residue Display Style/CA Wire",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RESIDUE_CAWIRE_ITEM,FL_MENU_RADIO);
	menu->add("Residues/Residue Display Style/CA Stick",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RESIDUE_CASTICK_ITEM,FL_MENU_RADIO);	
	menu->add("Residues/Residue Display Style/Line Ribbon",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RESIDUE_LINERIBBON_ITEM,FL_MENU_RADIO);
	menu->add("Residues/Residue Display Style/Flat Ribbon",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RESIDUE_FLATRIBBON_ITEM,FL_MENU_RADIO);
	menu->add("Residues/Residue Display Style/Solid Ribbon",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RESIDUE_SOLIDRIBBON_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Residues/Residue Display Style/Schematic",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RESIDUE_SCHEMATIC_ITEM,FL_MENU_RADIO);

	menu->add("Atoms/Color/Color Indexed",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)CI_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);		
	menu->add("Atoms/Color/Color Per Atom",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)CA_ITEM,FL_MENU_RADIO);		
	menu->add("Atoms/Color/Color Overall",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)CO_ITEM,FL_MENU_RADIO);		

	menu->add("Atoms/Radii/Radii Indexed",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RI_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);		
	menu->add("Atoms/Radii/Radii Per Atom",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RA_ITEM,FL_MENU_RADIO);		
	menu->add("Atoms/Radii/Radii Overall",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RO_ITEM,FL_MENU_RADIO);		

	menu->add("Bonds/Color/Bond Overall",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)OV_ITEM,FL_MENU_RADIO);		
	menu->add("Bonds/Color/Bond Per Bond",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BB_ITEM,FL_MENU_RADIO);		
	menu->add("Bonds/Color/Bond Per Bond Indexed",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BI_ITEM,FL_MENU_RADIO);		
	menu->add("Bonds/Color/Bond Per Atom",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BA_ITEM,FL_MENU_RADIO);		
	menu->add("Bonds/Color/Bond Per Atom Half-bonded",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BAH_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);

	menu->add("Residues/Color/Residue Overall",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ROV_ITEM,FL_MENU_RADIO);
	menu->add("Residues/Color/Residue Per Index",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RPI_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Residues/Color/Residue Per Chain",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RPC_ITEM,FL_MENU_RADIO);

	menu->add("Atoms/Labels/Atom Label Overall",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ALO_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Atoms/Labels/Atom Label Per Atom",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ALPA_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Labels/Atom Label Per Atom Label",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ALPAL_ITEM,FL_MENU_RADIO);
	menu->add("Atoms/Labels/Atom Label Per Atom Label Indexed",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ALPALI_ITEM,FL_MENU_RADIO);

	menu->add("Bonds/Labels/Bond Label Overall",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BLO_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Bonds/Labels/Bond Label Per Bond",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BLPB_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Labels/Bond Label Per Bond Label",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BLPBL_ITEM,FL_MENU_RADIO);
	menu->add("Bonds/Labels/Bond Label Indexed",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)BLPBLI_ITEM,FL_MENU_RADIO);

	menu->add("Residues/Labels/Residue Label Overall",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RLO_ITEM,FL_MENU_RADIO|FL_MENU_VALUE);
	menu->add("Residues/Labels/Residue Label Per Index",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RLPI_ITEM,FL_MENU_RADIO);
	menu->add("Residues/Labels/Residue Label Per Chain",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)RLPC_ITEM,FL_MENU_RADIO);

	menu->add("Atoms/Display Labels",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)LABEL_DISPLAY_ATOMLABEL_ITEM,FL_MENU_TOGGLE);
	menu->add("Bonds/Display Labels",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)LABEL_DISPLAY_BONDLABEL_ITEM,FL_MENU_TOGGLE);
	menu->add("Residues/Display Labels",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)LABEL_DISPLAY_RESIDUELABEL_ITEM,FL_MENU_TOGGLE);

	menu->add("Residues/Interpolate Color",0,(Fl_Callback*)chemSelectionCallback,	
		(void*)ICOLOR_ITEM,FL_MENU_TOGGLE|FL_MENU_VALUE);

#endif
}

void ChemExaminerViewer::chemSelectionCallback(// static
  int menuitemid,
  void * userdata)
{

#ifndef __SoFl__
	ChemExaminerViewer * viewer = (ChemExaminerViewer *) userdata;
#define MENUITEM(id,b) viewer->prefmenu->setMenuItemMarked(id,b)
#endif

#ifdef __SoFl__
	Fl_Menu_* o = (Fl_Menu_*) menuitemid;

	ViewerInterface* self = (ViewerInterface*)o->user_data();
	ChemExaminerViewer* viewer = (ChemExaminerViewer*)self->user_data();

	menuitemid = (int)userdata;
#define MENUITEM(id,b) 0
#endif

	if (menuitemid != MDLFILE_ITEM 
	&& menuitemid != COMPLEXITY_ITEM
	&& menuitemid != FONTDLG_ITEM
	&& menuitemid != FOGDLG_ITEM)
		MENUITEM(menuitemid,true);

	if (viewer == 0)
		return;

	switch (menuitemid)
	{
		case MDLFILE_ITEM:
			viewer->OpenFileDialog();
			break;

	  case FONTDLG_ITEM:
			viewer->OpenFontDialog();
			break;

		case COMPLEXITY_ITEM:
			viewer->OpenComplexityDialog();
			break;

		case FOGDLG_ITEM:
			viewer->OpenFogDialog();
			break;

		case ICOLOR_ITEM:
			viewer->cdp->residueInterpolateColor.setValue(!viewer->cdp->residueInterpolateColor.getValue());
			MENUITEM(menuitemid, viewer->cdp->residueInterpolateColor.getValue());
			break;


		case ATOM_HEMISPHERE_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_HEMISPHERES; // egb && sgb >>
			break;

		case ATOM_FULLSPHERE_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_FULLSPHERES; // egb && sgb >>
			break;

		case ATOM_ARRAYS_HEMISPHERE_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES; // egb && sgb >>
			break;

		case ATOM_ARRAYS_FULLSPHERE_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES; // egb && sgb >>
			break;

		case ATOM_GLUSPHERE_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_GLUSPHERE; // egb && sgb >>
			break;

		case ATOM_SOSPHERE_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_SOSPHERE; // egb && sgb >>
		break;

		case ATOM_BILLBOARD_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_BILLBOARD; // egb && sgb >>
			break;

		case ATOM_LOD_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_LOD; // egb && sgb >>
			break;

		case ATOM_LOD_GEOMETRY_ITEM:
			viewer->cdp->atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY; // egb && sgb >>
			break;

			// >> bonds items

		case BOND_CYLINDER_ITEM:
		{
			viewer->bondisplaystyle = ChemDisplayParam::BONDCYLINDER_NOCAP;
			viewer->cdp->bondCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_NOCAP); // egb && sgb >>
			viewer->updateCaps(viewer);
		}
		break;

		case BOND_ARRAYS_CYLINDER_ITEM:
		{
			viewer->bondisplaystyle = ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP;
			viewer->cdp->bondCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP); // egb && sgb >>
			viewer->updateCaps(viewer);
		}
		break;

		case BOND_GLUCYLINDER_ITEM:
		{
			viewer->bondisplaystyle = ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP;
			viewer->cdp->bondCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP); // egb && sgb >>
			viewer->updateCaps(viewer);
		}
		break;

		case BOND_SOCYLINDER_ITEM:
		{
			viewer->bondisplaystyle = ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP;
			viewer->cdp->bondCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP); // egb && sgb >>
			viewer->updateCaps(viewer);
		}
		break;

		case BOND_SEMI_ITEM:
		{
			viewer->bondisplaystyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP;
			viewer->cdp->bondCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP); // egb && sgb >>
			viewer->updateCaps(viewer);
		}
		break;

		case  BOND_LOD_ITEM:
		{
			viewer->bondisplaystyle = ChemDisplayParam::BONDCYLINDER_LOD_NOCAP;
			viewer->cdp->bondCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_LOD_NOCAP); // egb && sgb >>
			viewer->updateCaps(viewer);
		}
		break;

		//<<bonds items

		case RESIDUE_NONE_ITEM:
		{
			//viewer->cdp->residueCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP); // egb && sgb >>
			viewer->cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_NONE);
			
		}
		break;

		case RESIDUE_CAWIRE_ITEM:
		{
			//viewer->cdp->residueCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_NOCAP); // egb && sgb >>
			viewer->cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE); // egb && sgb >>
			
		}
		break;

		case RESIDUE_CASTICK_ITEM:
		{
			//viewer->cdp->residueCylinderDisplayStyle.setValue(ChemDisplayParam::BONDCYLINDER_ROUNDCAP); // egb && sgb >>
			viewer->cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_CASTICK); // egb && sgb >>
			
		}
		break;

		case RESIDUE_LINERIBBON_ITEM:
		{
			viewer->cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON);
		}
		break;

		case RESIDUE_FLATRIBBON_ITEM:
		{
			viewer->cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON);
		}
		break;

		case RESIDUE_SOLIDRIBBON_ITEM:
		{
			viewer->cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON);
		}
		break;

		case RESIDUE_SCHEMATIC_ITEM:
		{
			viewer->cdp->residueDisplayStyle.setValue(ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC);
		}
		break;

		case CI_ITEM:
		{
			viewer->cl->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM_INDEXED);		
		}
		break;

		case CA_ITEM:
		{
			viewer->cl->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);		
		}
		break;

		case CO_ITEM:
		{
			viewer->cl->atomColorBinding.setValue(ChemColor::ATOM_OVERALL);		
		}
		break;

		case RI_ITEM:
		{
			viewer->cr->atomRadiiBinding.setValue(ChemRadii::RADII_PER_ATOM_INDEXED);		
		}
		break;

		case RA_ITEM:
		{
			viewer->cr->atomRadiiBinding.setValue(ChemRadii::RADII_PER_ATOM);		
		}
		break;

		case RO_ITEM:
		{
			viewer->cr->atomRadiiBinding.setValue(ChemRadii::RADII_OVERALL);		
		}
		break;

		case OV_ITEM:
		{
			viewer->cl->bondColorBinding.setValue(ChemColor::BOND_OVERALL);		
			break;
		}

		case BB_ITEM:
		{
			viewer->cl->bondColorBinding.setValue(ChemColor::BOND_PER_BOND);		
			break;
		}

		case BI_ITEM:
		{
			viewer->cl->bondColorBinding.setValue(ChemColor::BOND_PER_BOND_INDEXED);		
			break;
		}

		case BA_ITEM:
		{
			viewer->cl->bondColorBinding.setValue(ChemColor::BOND_PER_ATOM);		
			break;
		}

		case BAH_ITEM:
		{
			viewer->cl->bondColorBinding.setValue(ChemColor::BOND_PER_ATOM_HALF_BONDED);		
			break;
		}

		case ROV_ITEM:
		{
			viewer->cl->residueColorBinding.setValue(ChemColor::RESIDUE_OVERALL);
			break;
		}
		case RPI_ITEM:
		{
			viewer->cl->residueColorBinding.setValue(ChemColor::RESIDUE_PER_INDEX);
			break;
		}
		case RPC_ITEM:
		{
			viewer->cl->residueColorBinding.setValue(ChemColor::RESIDUE_PER_CHAIN);
			break;
		}		

		case DRAWSTYLE_DISPLAY_NONE_ITEM:
			viewer->cdp->displayStyle = ChemDisplayParam::DISPLAY_NONE; // egb && sgb >>
			break;

		case DRAWSTYLE_DISPLAY_CPK_ITEM:
			viewer->cdp->displayStyle = ChemDisplayParam::DISPLAY_CPK; // egb && sgb >>
			break;

		case DRAWSTYLE_DISPLAY_STICK_ITEM:
			viewer->cdp->displayStyle = ChemDisplayParam::DISPLAY_STICK; // egb && sgb >>
			break;

		case DRAWSTYLE_DISPLAY_BALLSTICK_ITEM:
			viewer->cdp->displayStyle = ChemDisplayParam::DISPLAY_BALLSTICK; // egb && sgb >>
			break;

		case DRAWSTYLE_DISPLAY_WIREFRAME_ITEM:
			viewer->cdp->displayStyle = ChemDisplayParam::DISPLAY_WIREFRAME; // egb && sgb >>
			break;

		case DRAWSTYLE_DISPLAY_BALLWIRE_ITEM:
			viewer->cdp->displayStyle = ChemDisplayParam::DISPLAY_BALLWIRE; // egb && sgb >>
			break;

		case LABEL_DISPLAY_ATOMLABEL_ITEM:
			viewer->cdp->showAtomLabels = ! viewer->cdp->showAtomLabels.getValue();
			MENUITEM(menuitemid, viewer->cdp->showAtomLabels.getValue());
			break;

		case LABEL_DISPLAY_BONDLABEL_ITEM:
			viewer->cdp->showBondLabels = !viewer->cdp->showBondLabels.getValue();
			MENUITEM(menuitemid, viewer->cdp->showBondLabels.getValue());
			break;

		case LABEL_DISPLAY_RESIDUELABEL_ITEM:
			viewer->cdp->showResidueLabels = !viewer->cdp->showResidueLabels.getValue();
			MENUITEM(menuitemid, viewer->cdp->showResidueLabels.getValue());
			break;

		case CLIPPING_ATOM_ITEM:
			viewer->cdp->clipAtoms.setValue(! viewer->cdp->clipAtoms.getValue());
			MENUITEM(menuitemid, viewer->cdp->clipAtoms.getValue());
			break;

		case CLIPPING_BOND_ITEM:
			viewer->cdp->clipBonds.setValue(! viewer->cdp->clipBonds.getValue());
			MENUITEM(menuitemid, viewer->cdp->clipBonds.getValue());
			break;

		case CAP_NOCAP_ITEM:
		{
			viewer->cap = 0;
			viewer->updateCaps(viewer);
		}
		break;

		case CAP_FLATCAP_ITEM:
		{	
			viewer->cap = 1;
			viewer->updateCaps(viewer);
		}
		break;

		case CAP_ROUNDCAP_ITEM:
		{
			viewer->cap = 2;
			viewer->updateCaps(viewer);
		}
		break;

		case ALO_ITEM:
		{
			viewer->cl->atomLabelColorBinding.setValue(ChemColor::ATOM_LABEL_OVERALL);
			break;
		}

		case ALPA_ITEM:
		{
			viewer->cl->atomLabelColorBinding.setValue(ChemColor::ATOM_LABEL_PER_ATOM);
			break;
		}

		case ALPAL_ITEM:
		{
			viewer->cl->atomLabelColorBinding.setValue(ChemColor::ATOM_LABEL_PER_ATOM_LABEL);
			break;
		}

		case ALPALI_ITEM:
		{
			viewer->cl->atomLabelColorBinding.setValue(ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED);
			break;
		}

		case BLO_ITEM:
		{
			viewer->cl->bondLabelColorBinding.setValue(ChemColor::BOND_LABEL_OVERALL);
			break;
		}

		case BLPB_ITEM:
		{
			viewer->cl->bondLabelColorBinding.setValue(ChemColor::BOND_LABEL_PER_BOND);
			break;
		}

		case BLPBL_ITEM:
		{
			viewer->cl->bondLabelColorBinding.setValue(ChemColor::BOND_LABEL_PER_BOND_LABEL);
			break;
		}

		case BLPBLI_ITEM:
		{
			viewer->cl->bondLabelColorBinding.setValue(ChemColor::BOND_LABEL_PER_BOND_LABEL_INDEXED);
			break;
		}

		case RLO_ITEM:
		{
			viewer->cl->residueLabelColorBinding.setValue(ChemColor::RESIDUE_LABEL_OVERALL);
			break;
		}

		case RLPI_ITEM:
		{
			viewer->cl->residueLabelColorBinding.setValue(ChemColor::RESIDUE_LABEL_PER_INDEX);
			break;
		}

		case RLPC_ITEM:
		{
			viewer->cl->residueLabelColorBinding.setValue(ChemColor::RESIDUE_LABEL_PER_CHAIN);
			break;
		}

		case FOGNONE_ITEM:
		{
			viewer->cdp->fogType.setValue(ChemDisplayParam::FOG_NONE);
			break;
		}

		case FOGHAZE_ITEM:
		{
			viewer->cdp->fogType.setValue(ChemDisplayParam::FOG_HAZE);
			break;
		}

		case FOGFOG_ITEM:
		{
			viewer->cdp->fogType.setValue(ChemDisplayParam::FOG_FOG);
			break;
		}

		case FOGSMOKE_ITEM:
		{
			viewer->cdp->fogType.setValue(ChemDisplayParam::FOG_SMOKE);
			break;
		}



		default:
			break;
	}
} // menuSelectionCallback()


void ChemExaminerViewer::OpenFileDialog()
{
	char filename[255];
	char fileExt[4];
#ifdef __SoQt__
	QString s = QFileDialog::getOpenFileName(
								"",
								"Molecules (*.pdb *.mol)",
								NULL,
								"Obrir molècula"
								"Escolleix una molècula" );

	if (s.isEmpty())
		return;

	strcpy(filename,s);
	strcpy(fileExt,s.right(3).lower());
#endif

#ifdef __SoWin__
	TCHAR strFileName[512]="";
	TCHAR strFileName2[512]="";
	OPENFILENAME ofn = { sizeof(OPENFILENAME), getNormalWidget(), NULL,
						 "Molecule Files (*.pdb *.mol)\0*.pdb;*.mol\0\0",
						 NULL, 0, 1, strFileName2, 512, strFileName, 512,
						 "", "Open Molecule", OFN_FILEMUSTEXIST, 0, 1,
						 ".PDB", 0, NULL, NULL };

	GetOpenFileName(&ofn);
	strcpy(filename,ofn.lpstrFile);
	int of = strlen(filename);
	strcpy(fileExt,(ofn.lpstrFile+of-3));
#endif

#ifdef __SoFl__
	const char* flname = fl_file_chooser( "Select a molecular file", "*.{mol,pdb}", 0);
	if (flname == 0) return;
	strcpy(filename,flname);
	int of = strlen(filename);
	strcpy(fileExt,(filename+of-3));
#endif


#ifdef __SoGtk__

 	GtkWidget *file_selector;
 	gchar * flname;

 	/* Create the selector */
 	file_selector = gtk_file_selection_new("Please select a file for editing.");
// 	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
// 			    "clicked", GTK_SIGNAL_FUNC (flname), NULL);

// 	/* Ensure that the dialog box is destroyed when the user clicks a button. */
// 	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
// 				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
// 				   (GtkObject*) file_selector);
// 	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
// 				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
// 				   (GtkObject*) file_selector);
// 	/* Display that dialog */
// 	gtk_widget_show (file_selector);

// 	flname = gtk_file_selection_get_filename (GTK_FILE_SELECTION(file_selector));
// 	if (flname == 0) return;
// 	strcpy(filename,flname);
// 	int of = strlen(filename);
// 	strcpy(fileExt,(filename+of-3));

#endif


	if (!strcmp(fileExt,"mol"))
	{
		ChemMOLImporter input;
		chemdata->reset();
		input.openFile(filename,chemdata,chemassociateddata);
	}
	else if (!strcmp(fileExt,"pdb"))
	{
		ChemPDBImporter input;
		chemdata->reset();
		input.openFile(filename,chemdata,chemassociateddata);

	    // fill residue color table
		cl->residueColor.setNum(_RESNUM);
		cl->residueColor.setValues(0,_RESNUM,hydroCol);
		cl->residueLabelColor.setNum(_RESNUM);
		cl->residueLabelColor.setValues(0,_RESNUM,hydroCol);
	}
	else
	{
	}

	viewAll();
#ifdef __SoQt__ //qt non commercial requires substring "qt" was present
	strcat(filename," - Using SoQt");
#endif
	setTitle(filename);
}

void ChemExaminerViewer::OpenFontDialog()
{
#ifdef __SoQt__
	bool ok = false;

	QFont font = QFontDialog::getFont(&ok);
	if (ok)
	{
		cdp->fontName.setValue(/*font.rawName()*/font.family());
		cdp->fontSize.setValue(font.pointSize());
	}
#endif
#ifdef __SoWin__

	CHOOSEFONT cf; // common dialog box structure
	LOGFONT lf;    // logical font structure

	// Initialize CHOOSEFONT
	ZeroMemory(&cf, sizeof(cf));
	cf.lStructSize = sizeof (cf);
	cf.hwndOwner = getNormalWidget();
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_EFFECTS;

	if (ChooseFont(&cf)==TRUE)
	{
		cdp->fontName.setValue(cf.lpLogFont->lfFaceName);
		cdp->fontSize.setValue(cf.iPointSize/10.0f);
	}
#endif

#ifdef __SoFl__
	FlSetupFontDialog* w  = new FlSetupFontDialog(cdp);
	w->setupFont();
#endif
}

void ChemExaminerViewer::OpenComplexityDialog()
{
#ifdef __SoQt__
	QDialog *dlg = new QDialog(0,"Choose geometric complexity",true);
	dlg->resize(400,125);
	dlg->setCaption("Canviar complexitat");

	QBoxLayout * vert = new QVBoxLayout(dlg,10,10);
	
	QBoxLayout * atom = new QHBoxLayout(vert,10);

	QLabel *alabel = new QLabel("atomSphereComplexity",dlg);
	atom->addWidget(alabel);

	QSlider *aslider = new QSlider(dlg);
	aslider->setOrientation(Qt::Horizontal);
	aslider->setRange(1,100);
	aslider->setValue(cdp->atomSphereComplexity.getValue()*100);
	atom->addWidget(aslider);

	QPushButton *abutton = new QPushButton("Apply",dlg);
	QObject::connect( abutton, SIGNAL(clicked()), dlg, SLOT(accept()) );

	atom->addWidget(abutton);

	QBoxLayout * bond = new QHBoxLayout(vert,10);

	QLabel *blabel = new QLabel("bondCylinderComplexity",dlg);
	bond->addWidget(blabel);

	QSlider *bslider = new QSlider(dlg);
	bslider->setOrientation(Qt::Horizontal);
	bslider->setRange(1,100);
	bslider->setValue(cdp->bondCylinderComplexity.getValue()*100);

	bond->addWidget(bslider);

	QPushButton *bbutton = new QPushButton("Apply",dlg);
	QObject::connect( bbutton, SIGNAL(clicked()), dlg, SLOT(accept()) );
	bond->addWidget(bbutton);

	if (dlg->exec() == QDialog::Accepted)
	{
		cdp->atomSphereComplexity = (aslider->value()-1)/100.0f;
		cdp->bondCylinderComplexity = (bslider->value()-1 )/100.0f;
	}
#endif
#ifdef __SoWin__
	//MessageBox("Option not implemented in this SoGUI");
#endif

#ifdef __SoFl__
	FlSetupComplexityDialog* pDlg = new FlSetupComplexityDialog(cdp);
	pDlg->setupComplexity();
#endif
}

void ChemExaminerViewer::OpenFogDialog()
{
#ifdef __SoQt__
	QDialog *dlg = new QDialog(NULL,"fog dialog",true);
	dlg->resize(400,125);
	dlg->setCaption("Fog parameters");

	QBoxLayout * vert = new QVBoxLayout(dlg,10,10);
	
	QBoxLayout * fslayout = new QHBoxLayout(vert,10);

	QLabel *fslabel = new QLabel("fogStart",dlg);
	fslayout->addWidget(fslabel);

	QSlider *fslider = new QSlider(dlg);
	fslider ->setOrientation(Qt::Horizontal);
	fslider ->setRange(0,200);
	fslider ->setValue(cdp->fogStart.getValue());
	fslayout->addWidget(fslider);

	QPushButton *fsbutton = new QPushButton("Aplicar",dlg);
	QObject::connect( fsbutton, SIGNAL(clicked()), dlg, SLOT(accept()) );
	fslayout->addWidget(fsbutton);

	QBoxLayout * fvlayout = new QHBoxLayout(vert,10);

	QLabel *fvlabel = new QLabel("fogVisibility",dlg);
	fvlayout->addWidget(fvlabel);

	QSlider *fvslider = new QSlider(dlg);
	fvslider->setOrientation(Qt::Horizontal);
	fvslider->setRange(0,200);
	fvslider->setValue(cdp->fogVisibility.getValue());

	fvlayout->addWidget(fvslider);

	QPushButton *fvbutton = new QPushButton("Aplicar",dlg);
	QObject::connect( fvbutton, SIGNAL(clicked()), dlg, SLOT(accept()) );
	fvlayout->addWidget(fvbutton);

	//
	QBoxLayout * fdlayout = new QHBoxLayout(vert,10);

	QLabel *fdlabel = new QLabel("fogDensity",dlg);
	fdlayout->addWidget(fdlabel);

	QSlider *fdslider = new QSlider(dlg);
	fdslider->setOrientation(Qt::Horizontal);
	fdslider->setRange(0,10);
	fdslider->setValue(cdp->fogDensity.getValue());

	fdlayout->addWidget(fdslider);

	QPushButton *fdbutton = new QPushButton("Aplicar",dlg);
	QObject::connect( fdbutton, SIGNAL(clicked()), dlg, SLOT(accept()) );
	fdlayout->addWidget(fdbutton);

	if (dlg->exec() == QDialog::Accepted)
	{
		cdp->fogStart.setValue((float)fslider->value());
		cdp->fogVisibility.setValue((float)fvslider->value());
		cdp->fogDensity.setValue((float)fdslider->value());
	}
#endif
#ifdef __SoWin__
	printf("Option not implemented in this SoGUI\n");
#endif

#ifdef __SoFl__

	FlSetupFogDialog* pDlg = new FlSetupFogDialog(cdp);
	pDlg->setupFog();
#endif
}

void ChemExaminerViewer::updateCaps(ChemExaminerViewer *viewer)
{
	viewer->cdp->bondCylinderDisplayStyle.setValue(
		viewer->bondisplaystyle + viewer->cap
		);
}

void ChemExaminerViewer::UpdateMenuItems()
{
	if (cdp == NULL || cd == NULL) return;

#ifndef __SoFl__
	SoGUIPopupMenu* menu = this->prefmenu;
	if (menu == 0)

		menu->setMenuItemMarked(ATOM_HEMISPHERE_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_HEMISPHERES);
		menu->setMenuItemMarked(ATOM_FULLSPHERE_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_FULLSPHERES);
		menu->setMenuItemMarked(ATOM_ARRAYS_HEMISPHERE_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES);
		menu->setMenuItemMarked(ATOM_ARRAYS_FULLSPHERE_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES);
		menu->setMenuItemMarked(ATOM_GLUSPHERE_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_GLUSPHERE);
		menu->setMenuItemMarked(ATOM_SOSPHERE_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_SOSPHERE);
		menu->setMenuItemMarked(ATOM_BILLBOARD_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_BILLBOARD);
		menu->setMenuItemMarked(ATOM_LOD_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_LOD);
		menu->setMenuItemMarked(ATOM_LOD_GEOMETRY_ITEM,cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY);
		
		menu->setMenuItemMarked(BOND_CYLINDER_ITEM,cdp->bondCylinderDisplayStyle.getValue() == ChemDisplayParam::BONDCYLINDER_NOCAP);
		menu->setMenuItemMarked(BOND_ARRAYS_CYLINDER_ITEM,cdp->bondCylinderDisplayStyle.getValue() == ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP);
		menu->setMenuItemMarked(BOND_GLUCYLINDER_ITEM,cdp->bondCylinderDisplayStyle.getValue() == ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP);
		menu->setMenuItemMarked(BOND_SOCYLINDER_ITEM,cdp->bondCylinderDisplayStyle.getValue() == ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP);
		menu->setMenuItemMarked(BOND_SEMI_ITEM,cdp->bondCylinderDisplayStyle.getValue() == ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP);
		menu->setMenuItemMarked(BOND_LOD_ITEM,cdp->bondCylinderDisplayStyle.getValue() == ChemDisplayParam::BONDCYLINDER_LOD_NOCAP);

		menu->setMenuItemMarked(CAP_NOCAP_ITEM,cdp->bondCylinderDisplayStyle.getValue() % 3 == 0);
		menu->setMenuItemMarked(CAP_FLATCAP_ITEM,cdp->bondCylinderDisplayStyle.getValue() % 3 == 1);
		menu->setMenuItemMarked(CAP_ROUNDCAP_ITEM,cdp->bondCylinderDisplayStyle.getValue() % 3 == 2);

		bondisplaystyle = cdp->bondCylinderDisplayStyle.getValue();
		updateCaps(this);
		
		menu->setMenuItemMarked(RESIDUE_NONE_ITEM,cdp->residueDisplayStyle.getValue() == ChemDisplayParam::DISPLAY_RESIDUES_NONE);
		menu->setMenuItemMarked(RESIDUE_CAWIRE_ITEM,cdp->residueDisplayStyle.getValue() == ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE);
		menu->setMenuItemMarked(RESIDUE_CASTICK_ITEM,cdp->residueDisplayStyle.getValue() == ChemDisplayParam::DISPLAY_RESIDUES_CASTICK);
		menu->setMenuItemMarked(RESIDUE_LINERIBBON_ITEM,cdp->residueDisplayStyle.getValue() == ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON);
		menu->setMenuItemMarked(RESIDUE_FLATRIBBON_ITEM,cdp->residueDisplayStyle.getValue() == ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON);
		menu->setMenuItemMarked(RESIDUE_SOLIDRIBBON_ITEM,cdp->residueDisplayStyle.getValue() == ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON);
		menu->setMenuItemMarked(RESIDUE_SCHEMATIC_ITEM,cdp->residueDisplayStyle.getValue() == ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC);

		menu->setMenuItemMarked(DRAWSTYLE_DISPLAY_NONE_ITEM,cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_NONE);
		menu->setMenuItemMarked(DRAWSTYLE_DISPLAY_CPK_ITEM,cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_CPK);
		menu->setMenuItemMarked(DRAWSTYLE_DISPLAY_BALLSTICK_ITEM,cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK);
		menu->setMenuItemMarked(DRAWSTYLE_DISPLAY_STICK_ITEM,cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_STICK);
		menu->setMenuItemMarked(DRAWSTYLE_DISPLAY_WIREFRAME_ITEM,cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_WIREFRAME);
		menu->setMenuItemMarked(DRAWSTYLE_DISPLAY_BALLWIRE_ITEM,cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE);

		menu->setMenuItemMarked(CI_ITEM,cl->atomColorBinding.getValue() == ChemColor::ATOM_PER_ATOM_INDEXED);
		menu->setMenuItemMarked(CA_ITEM,cl->atomColorBinding.getValue() == ChemColor::ATOM_PER_ATOM);
		menu->setMenuItemMarked(CO_ITEM,cl->atomColorBinding.getValue() == ChemColor::ATOM_OVERALL);

		menu->setMenuItemMarked(RI_ITEM,cr->atomRadiiBinding.getValue() == ChemRadii::RADII_PER_ATOM_INDEXED);
		menu->setMenuItemMarked(RA_ITEM,cr->atomRadiiBinding.getValue() == ChemRadii::RADII_PER_ATOM);
		menu->setMenuItemMarked(RO_ITEM,cr->atomRadiiBinding.getValue() == ChemRadii::RADII_OVERALL);
		
		menu->setMenuItemMarked(OV_ITEM,cl->bondColorBinding.getValue() == ChemColor::BOND_OVERALL);
		menu->setMenuItemMarked(BB_ITEM,cl->bondColorBinding.getValue() == ChemColor::BOND_PER_BOND);
		menu->setMenuItemMarked(BI_ITEM,cl->bondColorBinding.getValue() == ChemColor::BOND_PER_BOND_INDEXED);
		menu->setMenuItemMarked(BA_ITEM,cl->bondColorBinding.getValue() == ChemColor::BOND_PER_ATOM);
		menu->setMenuItemMarked(BAH_ITEM,cl->bondColorBinding.getValue() == ChemColor::BOND_PER_ATOM_HALF_BONDED);

		menu->setMenuItemMarked(LABEL_DISPLAY_ATOMLABEL_ITEM,cdp->showAtomLabels.getValue());
		menu->setMenuItemMarked(LABEL_DISPLAY_BONDLABEL_ITEM,cdp->showBondLabels.getValue());
		menu->setMenuItemMarked(LABEL_DISPLAY_RESIDUELABEL_ITEM,cdp->showResidueLabels.getValue());

		menu->setMenuItemMarked(CLIPPING_ATOM_ITEM,cdp->clipAtoms.getValue());
		menu->setMenuItemMarked(CLIPPING_BOND_ITEM,cdp->clipBonds.getValue());

		menu->setMenuItemMarked(ROV_ITEM,cl->residueColorBinding.getValue() == ChemColor::RESIDUE_OVERALL);
		menu->setMenuItemMarked(RPI_ITEM,cl->residueColorBinding.getValue() == ChemColor::RESIDUE_PER_INDEX);
		menu->setMenuItemMarked(RPC_ITEM,cl->residueColorBinding.getValue() == ChemColor::RESIDUE_PER_CHAIN);

		menu->setMenuItemMarked(ALO_ITEM,cl->atomLabelColorBinding.getValue() == ChemColor::ATOM_LABEL_OVERALL);
		menu->setMenuItemMarked(ALPA_ITEM,cl->atomLabelColorBinding.getValue() == ChemColor::ATOM_LABEL_PER_ATOM);
		menu->setMenuItemMarked(ALPAL_ITEM,cl->atomLabelColorBinding.getValue() == ChemColor::ATOM_LABEL_PER_ATOM_LABEL);
		menu->setMenuItemMarked(ALPALI_ITEM,cl->atomLabelColorBinding.getValue() == ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED);

		menu->setMenuItemMarked(BLO_ITEM,cl->bondLabelColorBinding.getValue() == ChemColor::BOND_LABEL_OVERALL);
		menu->setMenuItemMarked(BLPB_ITEM,cl->bondLabelColorBinding.getValue() == ChemColor::BOND_LABEL_PER_BOND);
		menu->setMenuItemMarked(BLPBL_ITEM,cl->bondLabelColorBinding.getValue() == ChemColor::BOND_LABEL_PER_BOND_LABEL);
		menu->setMenuItemMarked(BLPBLI_ITEM,cl->bondLabelColorBinding.getValue() == ChemColor::BOND_LABEL_PER_BOND_LABEL_INDEXED);

		menu->setMenuItemMarked(RLO_ITEM,cl->residueLabelColorBinding.getValue() == ChemColor::RESIDUE_LABEL_OVERALL);
		menu->setMenuItemMarked(RLPI_ITEM,cl->residueLabelColorBinding.getValue() == ChemColor::RESIDUE_LABEL_PER_INDEX);
		menu->setMenuItemMarked(RLPC_ITEM,cl->residueLabelColorBinding.getValue() == ChemColor::RESIDUE_LABEL_PER_CHAIN);


		menu->setMenuItemMarked(FOGNONE_ITEM,cdp->fogType.getValue() == ChemDisplayParam::FOG_NONE);
		menu->setMenuItemMarked(FOGHAZE_ITEM,cdp->fogType.getValue() == ChemDisplayParam::FOG_HAZE);
		menu->setMenuItemMarked(FOGFOG_ITEM,cdp->fogType.getValue() == ChemDisplayParam::FOG_FOG);
		menu->setMenuItemMarked(FOGSMOKE_ITEM,cdp->fogType.getValue() == ChemDisplayParam::FOG_SMOKE);
#endif
#ifdef __SoFl__
		ViewerInterface* ui = getViewerUI();
		// API can't let to access menu items
#endif
}

#undef SoGUIExaminerViewer
#undef SoGUIFullViewer
#undef SoGUIViewer
#undef SoGUIPopupMenu
#undef MENUITEM
