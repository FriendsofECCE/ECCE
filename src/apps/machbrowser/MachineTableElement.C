/**
 *  @file
 *  @author Ken Swanson
 *
 *  Concrete class extending the AbstractTableElement within the wxgui
 *  library.  Defines, and contains the information for, the fields to
 *  be displayed in the WxTableView instance owned by the MachineBrowser.
 *
 */


#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#include "tdat/QueueMgr.H"

#include "MachineTableElement.H"


MachineTableElement::MachineTableElement(MachinePreferences *prefs)
{
    p_machPrefs = prefs;
    p_siteAccess = false;
    p_queryAllwd = false;
}


MachineTableElement::~MachineTableElement()
{
}


MachinePreferences * MachineTableElement::getPreferences()
{
    return p_machPrefs;
}


bool MachineTableElement::isQueryAllowed()
{
    return p_queryAllwd;
}


void MachineTableElement::setQueryAllowed(bool allwd)
{
    p_queryAllwd = allwd;
}


bool MachineTableElement::isSiteAccessible()
{
    return p_siteAccess;
}


void MachineTableElement::setSiteAccessible(bool access)
{
    p_siteAccess = access;
}


string MachineTableElement::getValueAsString(int field)
{
    string text = "";

    RefMachine *refMach = p_machPrefs->getRegisteredMachine();
    const QueueManager *queueMngr = QueueManager::lookup(refMach->refname());

    switch (field)
    {
        case 0:
            text = "" ;
            break;
        case 1:
            text = refMach->refname();
            break;
        case 2:
            text = (p_siteAccess ? "Site" : "User");
            break;
        case 3:
            text = StringConverter::toString((int)(refMach->proccount()));
            break;
        case 4:
            text = (queueMngr == 0) ? "" : queueMngr->queueMgrName();
            break;
        case 5:
            text = reformat(refMach->remshellsString());
            break;
        case 6:
            text = reformat(refMach->codesString());
            break;
        case 7:
            text = getDescription(refMach);
            break;
        default:
            text = "";
    }

    return text;
}


int MachineTableElement::getImageIndex()
{
    return (p_queryAllwd ? 1 : 0);
}


string MachineTableElement::reformat(string s)
{
    string s2 = "";

    if (s.size() > 0)
    {
        StringTokenizer *tkn = new StringTokenizer(s);

        vector<string> elmts = tkn->tokenize(":");

        int n = elmts.size();

        for (int i = 0; i < n; i++)
        {
            if (i > 0)
                s2.append(", ");

            s2.append(elmts[i]);
        }
    }

    return s2;

}


string MachineTableElement::getDescription(RefMachine *rgstn)
{
    string s1 = rgstn->vendor();
    string s2 = rgstn->model();
    string s3 = rgstn->proctype();

    string result = s1;

    if (s2 != "Unspecified")
        result.append(" " + s2);

    if (s3 != "Unspecified")
        result.append(" " + s3);

    return result;
}


int MachineTableElement::compareTo(TableElement *elmnt, int field)
{
    MachinePreferences *prefs = ((MachineTableElement*)elmnt)->getPreferences();
    bool queryAllwd = ((MachineTableElement *)elmnt)->isQueryAllowed();
    bool siteAccess = ((MachineTableElement *)elmnt)->isSiteAccessible();

    int result = 0;

    RefMachine *rgstnCurr = p_machPrefs->getRegisteredMachine();
    RefMachine *rgstnComp = prefs->getRegisteredMachine();

    const QueueManager *qmngrCurr = QueueManager::lookup(p_machPrefs->getItemKey());
    const QueueManager *qmngrComp = QueueManager::lookup(prefs->getItemKey());

    switch (field)
    {
        case 0:
            result = p_queryAllwd - queryAllwd;
            break;

        case 1:
            result = (rgstnCurr->refname()).compare(rgstnComp->refname());
            break;

        case 2:
            result = p_siteAccess - siteAccess;
            break;

        case 3:
            result = rgstnCurr->proccount() - rgstnComp->proccount();
            break;

        case 4:
            result = ((qmngrCurr == NULL) ? "" :
                  qmngrCurr->queueMgrName()).compare((qmngrComp == NULL) ? "" :
                  qmngrComp->queueMgrName());
            break;

        case 5:
            result = (rgstnCurr->remshellsString()).compare(rgstnComp->remshellsString());
            break;

        case 6:
            result = (rgstnCurr->codesString()).compare(rgstnComp->codesString());
            break;

        case 7:
            result = getDescription(rgstnCurr).compare(getDescription(rgstnComp));
            break;
    }

    if (result > 1)
      result = 1;
    else if (result < -1)
      result = -1;

    return result;
}
