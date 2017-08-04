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
// ChemExaminerViewer.h: interface for the ChemExaminerViewer class.
//
//////////////////////////////////////////////////////////////////////

#define __SoGtk__

#ifndef _CHEMEXAMINERVIEWER_H
#define _CHEMEXAMINERVIEWER_H

#ifdef __SoQt__
#define GUIWidget QWidget*
#define SoGUIExaminerViewer SoQtExaminerViewer
#define SoGUIFullViewer SoQtFullViewer
#define SoGUIViewer SoQtViewer
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#endif

#ifdef __SoXt__
#define GUIWidget Widget
#define SoGUIExaminerViewer SoXtExaminerViewer
#define SoGUIFullViewer SoXtFullViewer
#define SoGUIViewer SoXtViewer
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>
#endif

#ifdef __SoGtk__
#define GUIWidget GtkWidget*
#define SoGUIExaminerViewer SoGtkExaminerViewer
#define SoGUIFullViewer SoGtkFullViewer
#define SoGUIViewer SoGtkViewer
#include <Inventor/Gtk/SoGtk.h>
#include <Inventor/Gtk/viewers/SoGtkExaminerViewer.h>
#endif

#ifdef __SoWin__
#define GUIWidget HWND
#define SoGUIExaminerViewer SoWinExaminerViewer
#define SoGUIFullViewer SoWinFullViewer
#define SoGUIViewer SoWinViewer
#include <Inventor/Win/viewers/SoWinExaminerViewer.h>
#endif

#ifdef __SoFl__
#define GUIWidget Fl_Window*
#define SoGUIExaminerViewer SoFlExaminerViewer
#define SoGUIFullViewer SoFlFullViewer
#define SoGUIViewer SoFlViewer
#include <Inventor/FL/viewers/SoFlExaminerViewer.h>
#endif

class ChemDisplay;
class ChemRadii;
class ChemColor;
class ChemDisplayParam;
class ChemData;
class ChemAssociatedData;

class ChemExaminerViewer : public SoGUIExaminerViewer
{
public:
  ChemExaminerViewer(
		     GUIWidget parent = NULL //,
		     //	   const char * name = NULL,
		     //    SbBool embed = TRUE,
		     //SoGUIFullViewer::BuildFlag flag = BUILD_ALL,
		     //SoGUIFullViewer::BuildFlag flag = BUILD_DECORATION,
		     //	   SoGUIFullViewer::BuildFlag flag = BUILD_ALL,
		     //    SoGUIViewer::Type type = BROWSER,
		     //	   SbBool build = true
	);

	void UpdateMenuItems();

	void setNodes(ChemData *chemdata,ChemAssociatedData *chemassociateddata,
		ChemRadii* cr,ChemColor* cl,
		ChemDisplayParam* cdp,ChemDisplay* cd);

	void OpenFileDialog();
	void OpenFontDialog();
	void OpenComplexityDialog();
	void OpenFogDialog();
	
private:
	ChemData *chemdata;
	ChemAssociatedData *chemassociateddata;
	ChemRadii* cr;
	ChemColor* cl;
	ChemDisplayParam* cdp;
	ChemDisplay* cd;
	

private:
	int bondisplaystyle;
	int cap;

	void updateCaps(ChemExaminerViewer *viewer);

protected:
	void addPopupMenuItems();

	enum ChemExaminerViewerPopupMenuItem
	{

		MOLECULAR_MENU=50,

		ATOMS_MENU,
		ASDS_MENU,
		ATOM_HEMISPHERE_ITEM,
		ATOM_FULLSPHERE_ITEM,
		ATOM_ARRAYS_HEMISPHERE_ITEM,
		ATOM_ARRAYS_FULLSPHERE_ITEM,
		ATOM_GLUSPHERE_ITEM,
		ATOM_SOSPHERE_ITEM,
		ATOM_BILLBOARD_ITEM,
		ATOM_LOD_ITEM,
		ATOM_LOD_GEOMETRY_ITEM,

		BONDS_MENU,
		BCDS_MENU,
		BOND_CYLINDER_ITEM,
		BOND_ARRAYS_CYLINDER_ITEM,
		BOND_GLUCYLINDER_ITEM,
		BOND_SOCYLINDER_ITEM,
		BOND_SEMI_ITEM,
		BOND_LOD_ITEM,

		RESIDUES_MENU,
		RDS_MENU,
		RESIDUE_NONE_ITEM,
		RESIDUE_CAWIRE_ITEM,
		RESIDUE_CASTICK_ITEM,
		RESIDUE_LINERIBBON_ITEM,
		RESIDUE_FLATRIBBON_ITEM,
		RESIDUE_SOLIDRIBBON_ITEM,
		RESIDUE_SCHEMATIC_ITEM,

		DRAWSTYLE_MENU,
		DRAWSTYLE_DISPLAY_NONE_ITEM,
		DRAWSTYLE_DISPLAY_CPK_ITEM,
		DRAWSTYLE_DISPLAY_BALLSTICK_ITEM,
		DRAWSTYLE_DISPLAY_STICK_ITEM,
		DRAWSTYLE_DISPLAY_WIREFRAME_ITEM,
		DRAWSTYLE_DISPLAY_BALLWIRE_ITEM,

		COLOR_ATOMS_MENU,
		CI_ITEM,
		CA_ITEM,
		CO_ITEM,

		RADII_ATOMS_MENU,
		RI_ITEM,
		RA_ITEM,
		RO_ITEM,

		COLOR_BOND_MENU,
		OV_ITEM,
		BB_ITEM,
		BI_ITEM,
		BA_ITEM,
		BAH_ITEM,

		MDLFILE_ITEM,
		FONTDLG_ITEM,
		COMPLEXITY_ITEM,
		FOGDLG_ITEM,

		CAP_MENU,
		CAP_NOCAP_ITEM,
		CAP_FLATCAP_ITEM,
		CAP_ROUNDCAP_ITEM,

		COLOR_RESIDUES_MENU,
		ICOLOR_ITEM,
		ROV_ITEM,
		RPI_ITEM,
		RPC_ITEM,

		ATOM_LABEL_COLOR_MENU,
		ALO_ITEM,
		ALPA_ITEM,
		ALPAL_ITEM,
		ALPALI_ITEM,

		BOND_LABEL_COLOR_MENU,
		BLO_ITEM,
		BLPB_ITEM,
		BLPBL_ITEM,
		BLPBLI_ITEM,

		RESIDUE_LABEL_COLOR_MENU,
		RLO_ITEM,
		RLPI_ITEM,
		RLPC_ITEM,

		LABEL_DISPLAY_ATOMLABEL_ITEM,
		LABEL_DISPLAY_BONDLABEL_ITEM,
		LABEL_DISPLAY_RESIDUELABEL_ITEM,

		CLIPPING_ATOM_ITEM,
		CLIPPING_BOND_ITEM,

		FOG_MENU,
		FOGNONE_ITEM,
		FOGHAZE_ITEM,
		FOGFOG_ITEM,
		FOGSMOKE_ITEM,
	};

	enum SoViewerPopupMenuRadioGroup
	{
		ATOM_GROUP=105,
		BOND_GROUP,
		CAP_GROUP,
		DRAWSTYLE_GROUP,
		COLOR_GROUP,
		RADII_GROUP,
		RESIDUE_GROUP,
		COLOR_BOND_GROUP,
		COLOR_RESIDUES_GROUP,
		ATOM_LABEL_COLOR_GROUP,
		BOND_LABEL_COLOR_GROUP,
		RESIDUE_LABEL_COLOR_GROUP,
		FOG_GROUP,
	};

	static void chemSelectionCallback(int menuitemid,void * userdata);
};

#undef Widget
#undef SoGUIExaminerViewer
#undef SoGUIFullViewer
#undef SoGUIViewer


#endif
