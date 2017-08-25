/**
 * @file PartialCharge.H
 */

#ifndef _PARTIALCHARGE_H_
#define _PARTIALCHARGE_H_

#include "wxgui/WxJMSListener.H"

#include "PartialChargeGUI.H"


class CalcEd;
class ESPConstraint;
class JMSMessage;
class WxJMSMessageDispatch;


class PartialCharge: public PartialChargeGUI,
                     public WxJMSListener
{    
    DECLARE_CLASS(PartialCharge)

public:
    PartialCharge();
    PartialCharge( wxWindow* parent,
            wxWindowID id = SYMBOL_PARTIALCHARGEGUI_IDNAME,
	          const wxString& caption = SYMBOL_PARTIALCHARGEGUI_TITLE,
		        const wxPoint& pos = SYMBOL_PARTIALCHARGEGUI_POSITION,
		        const wxSize& size = SYMBOL_PARTIALCHARGEGUI_SIZE,
		        long style = SYMBOL_PARTIALCHARGEGUI_STYLE );
    ~PartialCharge();

    bool Create( wxWindow* parent,
            wxWindowID id = SYMBOL_PARTIALCHARGEGUI_IDNAME,
            const wxString& caption = SYMBOL_PARTIALCHARGEGUI_TITLE,
            const wxPoint& pos = SYMBOL_PARTIALCHARGEGUI_POSITION,
            const wxSize& size = SYMBOL_PARTIALCHARGEGUI_SIZE,
            long style = SYMBOL_PARTIALCHARGEGUI_STYLE );
    void initializeGUI();

    // event handlers
    virtual void OnButtonConstrainAllMethylGroupsClick( wxCommandEvent& event );
    virtual void OnButtonConstrainAllHydrogensClick( wxCommandEvent& event );
    virtual void OnChoiceConstraintsSelected( wxCommandEvent& event );
    virtual void OnButtonGroup1Click( wxCommandEvent& event );
    virtual void OnButtonGroup2Click( wxCommandEvent& event );
    virtual void OnButtonAddCmdClick( wxCommandEvent& event );
    virtual void OnButtonDeleteCmdClick( wxCommandEvent& event );
    virtual void OnListboxCmdlistSelected( wxCommandEvent& event );
    virtual void OnButtonShowUnconstrainedAtomsClick( wxCommandEvent& event );
    virtual void OnButtonShowMultiplyConstrainedAtomsClick( wxCommandEvent& event );
    virtual void OnCloseClick( wxCommandEvent& event );
    virtual void OnHelpClick( wxCommandEvent& event );

protected:
    // Messaging
    void initializeMessaging();
    void selectionMCB(JMSMessage& msg);

    void requestBuilder(const string& actionTopic="",
                        map<string, string> *properties = NULL);

    void constrainAllMethylGroups();
    void constrainAllMethylHydrogen();
    void methylConstraint(const int& numAtoms, const string& atomList);

    void parseAtomList(string atom_list, vector<int> &indices,
	                     vector<string> &symbols);
    void parseConstraint(string constraint, vector<int> &indices);
    ESPConstraint* stringToConstraint(string constraint);
    string constraintToString(const ESPConstraint *esp);
    void updateConstraints();

private:
    WxJMSMessageDispatch *p_msgDispatcher;
    int p_group;
    CalcEd *p_calcEd;
    double p_atomCharge;
    double p_totalCharge;
};

#endif  // _PARTIALCHARGEGUI_H_
