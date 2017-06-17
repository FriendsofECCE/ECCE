/**
 * @file
 * @author Ken Swanson
 *
 *  Concrete class extending the AbstractTableElement within the wxgui
 *  library.  Defines, and contains the information for, the fields to
 *  be displayed in the WxTableView instance owned by the MachineBrowser.
 *
 */


#ifndef MACHINETABLEELEMENT_HH
#define MACHINETABLEELEMENT_HH

#include "tdat/RefMachine.H"

#include "dsm/MachinePreferences.H"

#include "wxgui/TableElement.H"


class MachineTableElement: public TableElement
{
    public:
        MachineTableElement(MachinePreferences *prefs);
        virtual ~MachineTableElement();

        MachinePreferences      *getPreferences();

        //  Methods implementing TableElement
        int compareTo(TableElement *elmnt, int field);
        string getValueAsString(int field);
        int getImageIndex();

        bool isQueryAllowed();
        void setQueryAllowed(bool allwd);

        bool isSiteAccessible();
        void setSiteAccessible(bool access);

    private:
        MachinePreferences      *p_machPrefs;
        string reformat(string s);
        static string getDescription(RefMachine *rgstn);
        bool    p_queryAllwd;
        bool    p_siteAccess;

};
#endif      //  MACHINETABLEELEMENT_HH
