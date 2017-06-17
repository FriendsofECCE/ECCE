/**
 * @file StructLib.H
 */

#ifndef _STRUCTLIB_H_
#define _STRUCTLIB_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "StructLib.C"
#endif

#include "util/EcceURL.H"

#include "wxviz/WxVizTool.H"

#include "StructLibGUI.H"

class ewxBitmapButton;
class ewxChoice;
class ewxStaticText;
class EDSIStructureLib;
class SGViewer;
class wxImageList;

/*!
 * StructLib class declaration
 */
class StructLib: public StructLibGUI, public WxVizTool
{

    DECLARE_DYNAMIC_CLASS( StructLib )

    public:
        enum ResourceTypeConstants
        {
            STANDARD = 0,
            USER = 1,
            SYSTEM = 2
        };

        /// Constructors
        StructLib( );
        StructLib( wxWindow* parent,
             wxWindowID id = SYMBOL_STRUCTLIBGUI_IDNAME,
             const wxPoint& pos = SYMBOL_STRUCTLIBGUI_POSITION,
             const wxSize& size = SYMBOL_STRUCTLIBGUI_SIZE,
             long style = SYMBOL_STRUCTLIBGUI_STYLE );


        void refreshFamiliesButtonClickCB(wxCommandEvent& event);
        void bookmarksChoiceSelectedCB(wxCommandEvent& event);
        void structuresListItemSelectedCB(wxListEvent& event);
        void structuresListItemActivatedCB(wxListEvent& event);
        void windowClosedCB(wxCloseEvent& event);
        void windowSizedCB(wxSizeEvent& event);
        void upOneLevelButtonClickCB(wxCommandEvent& event);
        void mainSplitterSashChangingCB(wxSplitterEvent& event);
        Fragment *getSelectedFragment() const;
        int  getSelectedAtom() const;
        void configure();

    private:
        void connectResource(const EcceURL& url,
                             const ResourceTypeConstants rsrcType = STANDARD);
        void createControls();
        void refreshStructures();
        void initializeViewer();

        EcceURL              p_urlLibraryDefault;        //  Default URL of Structure Library

        EcceURL              p_urlSelectionFolder;      //  Containing Resource of currently displayed collection
        EcceURL              p_urlSelectionBase;        //  Base URL of selection (as displayed in dropdown)

        ewxChoice           *p_bookmarksChoice;
        wxListCtrl          *p_structuresListCtrl;

        SGViewer            *p_viewer;
        SGContainer         *p_container;

        ewxStaticText       *p_nullSelectionStaticText;
        ewxStaticText       *p_noneExistStaticText;

        SGFragment          *p_librarySelection;

        EDSIStructureLib    *p_structures;

        wxSplitterWindow    *p_viewerSplit;
        bool                 p_inSizeAdjust;
        wxImageList         *p_listItemImages;

        ewxBitmapButton     *p_upOneLevelButton;
};

#endif
