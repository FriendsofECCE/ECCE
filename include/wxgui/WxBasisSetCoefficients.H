#ifndef WXBASISSETCOEFFICIENTS_HH
#define WXBASISSETCOEFFICIENTS_HH

#include <string>

#include "wxgui/WxBasisSetCoefficientsGUI.H"

class ewxButton;
class ewxTextCtrl;
class ewxStaticText;
class Preferences;
class TCalculation;
class TGBSConfig;

#define WXBASISSETCOEFFICIENTS_WINDOW_MINWIDTH     550
#define WXBASISSETCOEFFICIENTS_WINDOW_MINHEIGHT    400
#define WXBASISSETCOEFFICIENTS_WINDOW_DFLTLEFT     200
#define WXBASISSETCOEFFICIENTS_WINDOW_DFLTTOP      200
#define WXBASISSETCOEFFICIENTS_WINDOW_DFLTWIDTH    WXBASISSETCOEFFICIENTS_WINDOW_MINWIDTH
#define WXBASISSETCOEFFICIENTS_WINDOW_DFLTHEIGHT   WXBASISSETCOEFFICIENTS_WINDOW_MINHEIGHT

class WxBasisSetCoefficients: public WxBasisSetCoefficientsGUI
{
    DECLARE_CLASS(WxBasisSetCoefficients)


    public:
        /// Constructors, Destructor
        WxBasisSetCoefficients(      wxWindow* parent,
                                     wxWindowID id = SYMBOL_WXBASISSETCOEFFICIENTSGUI_IDNAME,
                               const wxString& caption = SYMBOL_WXBASISSETCOEFFICIENTSGUI_TITLE,
                               const wxPoint& pos = SYMBOL_WXBASISSETCOEFFICIENTSGUI_POSITION,
                               const wxSize& size = SYMBOL_WXBASISSETCOEFFICIENTSGUI_SIZE,
                                     long style = SYMBOL_WXBASISSETCOEFFICIENTSGUI_STYLE);
        virtual ~WxBasisSetCoefficients();

        void saveSettings(Preferences *prefs);
        void loadSettings(Preferences *prefs);
        void closeButtonClickCB(wxCommandEvent& event);

        void showDetails(TCalculation *calc);
        // The same view for a configuration held elsewhere (the editor's).
        void showDetails(TGBSConfig *cnfg, const std::string& codeName);
        // The exponents and coefficients as the code's input lists them.
        static std::string formatted(TGBSConfig *cnfg, const std::string& codeName);

    private:
        void createControls();


        ewxTextCtrl             *p_detailTextCtrl;
        ewxStaticText           *p_headerStaticText;

        ewxButton               *p_closeButton;
        ewxButton               *p_helpButton;
};

#endif  //  WXBASISSETCOEFFICIENTS_HH
