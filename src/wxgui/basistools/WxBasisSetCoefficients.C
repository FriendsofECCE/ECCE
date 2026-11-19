#include <iostream>
//    using std::cout;
//    using std::endl;
#include <fstream>
    using std::ofstream;
#include <string>
    using std::string;
#include <set>
    using std::set;
    using std::less;
#include <vector>

#include "wx/dc.h"
#include "wx/file.h"

#include "util/Color.H"
#include "util/Ecce.H"
#include "util/EcceSortedVector.H"
#include "util/ErrMsg.H"
#include "util/ETimer.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "tdat/Fragment.H"
#include "tdat/TFormula.H"
#include "tdat/TTCPair.H"
#include "tdat/TTheory.H"

#include "dsm/GBSRules.H"
#include "dsm/TCalculation.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/CodeFactory.H"
#include "dsm/DavDebug.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"

#include "wxgui/WxBasisSetCoefficients.H"


IMPLEMENT_CLASS( WxBasisSetCoefficients, WxBasisSetCoefficientsGUI )


WxBasisSetCoefficients::WxBasisSetCoefficients(      wxWindow* parent,
                                                     wxWindowID id,
                                               const wxString& caption,
                                               const wxPoint& pos,
                                               const wxSize& size,
                                                     long style)
                    : WxBasisSetCoefficientsGUI(parent, id, caption, pos, size, style)
{
    this->createControls();
}


WxBasisSetCoefficients::~WxBasisSetCoefficients()
{
}


void WxBasisSetCoefficients::createControls()
{
    long itemID;

    itemID = ID_STATIC_WXBASISSETCOEFFICIENTS_HEADER;
    p_headerStaticText = (ewxStaticText *)(FindWindowById(itemID));

    itemID = ID_TEXTCTRL_WXBASISSETCOEFFICIENTS_DETAIL;
    p_detailTextCtrl = (ewxTextCtrl *)(FindWindowById(itemID));
    // mouseover was highlighting all text due to ewxDefaultValidator, remove
    p_detailTextCtrl->SetValidator(wxDefaultValidator);

    itemID = ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE;
    p_closeButton = (ewxButton *)(FindWindowById(itemID));

    itemID = ID_BUTTON_WXBASISSETCOEFFICIENTS_HELP;
    p_helpButton = (ewxButton *)(FindWindowById(itemID));
}


void WxBasisSetCoefficients::showDetails(TCalculation *calc)
{
    const JCode* code = calc->application();
    showDetails(calc->gbsConfig(), code ? code->name() : string());
}


string WxBasisSetCoefficients::formatted(TGBSConfig *cnfg, const string& codeName)
{
    if (cnfg == NULL || codeName.empty()) return "";
    char* data = (char*)cnfg->dump(codeName.c_str(), false);
    return data ? string(data) : string();
}


void WxBasisSetCoefficients::showDetails(TGBSConfig *cnfg, const string& codeName)
{
    if ((cnfg != NULL) && !codeName.empty())
    {
        p_detailTextCtrl->SetValue(formatted(cnfg, codeName));
        p_headerStaticText->SetLabel(cnfg->name() + " formatted for " + codeName);
    }
    else
    {
        p_detailTextCtrl->SetValue("");
        p_headerStaticText->SetLabel("");
    }

    this->Refresh();
    this->Show();
}

void WxBasisSetCoefficients::saveSettings(Preferences *prefs)
{
    const string topic = "BASISSETCOEFFICIENTS";

    wxRect r = this->GetRect();
    prefs->setInt(topic + ".LEFT", r.GetLeft());
    prefs->setInt(topic + ".TOP", r.GetTop());

    wxSize size = this->GetSize();
    prefs->setInt(topic + ".WIDTH", size.GetWidth());
    prefs->setInt(topic + ".HEIGHT", size.GetHeight());

    prefs->saveFile();
}


void WxBasisSetCoefficients::loadSettings(Preferences *prefs)
{
    int x = WXBASISSETCOEFFICIENTS_WINDOW_DFLTLEFT;
    int y = WXBASISSETCOEFFICIENTS_WINDOW_DFLTTOP;
    int w = WXBASISSETCOEFFICIENTS_WINDOW_DFLTWIDTH;
    int h = WXBASISSETCOEFFICIENTS_WINDOW_DFLTHEIGHT;

    const string topic = "CONTRACTIONEDITOR";
    string key;

    key = topic + ".LEFT";

    if (prefs->isDefined(key))
        prefs->getInt(key, x);

    key = topic + ".TOP";

    if (prefs->isDefined(key))
        prefs->getInt(key, y);

     key = topic + ".WIDTH";

     if (prefs->isDefined(key))
        prefs->getInt(key, w);

     key = topic + ".HEIGHT";

     if (prefs->isDefined(key))
        prefs->getInt(key, h);

     this->SetSize(x, y, w, h);
}


void WxBasisSetCoefficients::closeButtonClickCB( wxCommandEvent& event )
{
    this->Close();
}
