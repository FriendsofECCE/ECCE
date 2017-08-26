#ifndef WXBASISSETDETAIL_HH
#define WXBASISSETDETAIL_HH


#include "util/EcceMap.hpp"
#include "util/JMSMessage.hpp"
#include "util/JMSPublisher.hpp"
#include "util/Preferences.hpp"

#include "tdat/Fragment.hpp"

#include "dsm/TCalculation.hpp"
#include "dsm/TGaussianBasisSet.hpp"
#include "dsm/TGBSConfig.hpp"
#include "dsm/EDSIGaussianBasisSetLibrary.hpp"

#include "wxgui/WxFeedback.hpp"
#include "wxgui/WxDavAuth.hpp"
#include "wxgui/PerTabPanel.hpp"

#include "WxBasisSetDetailGUI.hpp"

class ewxButton;
class ewxTextCtrl;

#define WXBASISSETDETAIL_WINDOW_MINWIDTH     700
#define WXBASISSETDETAIL_WINDOW_MINHEIGHT    600
#define WXBASISSETDETAIL_WINDOW_DFLTLEFT     200
#define WXBASISSETDETAIL_WINDOW_DFLTTOP      200
#define WXBASISSETDETAIL_WINDOW_DFLTWIDTH    WXBASISSETDETAIL_WINDOW_MINWIDTH
#define WXBASISSETDETAIL_WINDOW_DFLTHEIGHT   WXBASISSETDETAIL_WINDOW_MINHEIGHT


class WxBasisSetDetail: public WxBasisSetDetailGUI
{
    DECLARE_CLASS(WxBasisSetDetail)
    //DECLARE_EVENT_TABLE()


    public:
        /// Constructors, Destructor
        WxBasisSetDetail(      wxWindow* parent,
                               wxWindowID id = SYMBOL_WXBASISSETDETAILGUI_IDNAME,
                         const wxString& caption = SYMBOL_WXBASISSETDETAILGUI_TITLE,
                         const wxPoint& pos = SYMBOL_WXBASISSETDETAILGUI_POSITION,
                         const wxSize& size = SYMBOL_WXBASISSETDETAILGUI_SIZE,
                               long style = SYMBOL_WXBASISSETDETAILGUI_STYLE);
        virtual ~WxBasisSetDetail();

        void closeButtonClickCB(wxCommandEvent& event);

        void showDetails(string gbsname, gbs_details *details);
     //   void windowSizeCB(wxSizeEvent& event);
//        void graphPanelPaintCB(wxPaintEvent& event);
        void saveSettings(Preferences *prefs);
        void loadSettings(Preferences *prefs);
        void display(const string& fpath);

    private:
        void createControls();


        ewxTextCtrl             *p_rfrncsTextCtrl;
        ewxTextCtrl             *p_dscpnTextCtrl;
        wxStaticBitmap          *p_graphBitmap;

        ewxButton               *p_closeButton;
        ewxButton               *p_helpButton;

        wxImage                 *p_graphImage;

        wxScrolledWindow        *p_graphPane;

        string                   p_cptnBase;


//        static int p_indentLevel;
//        static void DebugEnterMethod(string dscpn);
//        static void DebugWriteMessage(string mesg);
//        static void DebugLeaveMethod();

};

#endif  //  WXBASISSETDETAIL_HH
