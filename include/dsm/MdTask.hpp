/**
* @file
*
*
*/
#ifndef _MDTASK_HH
#define _MDTASK_HH

#include <string>
  using std::string;

#include "util/DavException.H"
#include "util/EcceURL.H"
#include "util/InvalidException.H"

#include "tdat/Fragment.H"
#include "tdat/FragmentSummary.H"

#include "dsm/PropertyTask.H"
#include "dsm/EDSIFactory.H"


/**
 * Class providing an access architecture fo applications requiring
 * MD Tasks.
 * MDTasks have the following special characteristics:
 * <ul>
 * <li>they have an original fragment (with model, chains, alt loc selections),
 * an input fragment, and an output fragment.</li>
 * <li>they are governed by a session which tracks relationships
 * between tasks</li>
 * <li>once a system is defined through a prepare task, the fragments cannot
 * be modified by ecce or users - it is under the control of nwchem</li>
 * </ul>
 */
class MdTask : virtual public PropertyTask 
{

  public:

    friend class EDSIFactory;

    virtual void removeOutputFragment();
    virtual void removeInputFragment();
    virtual void removeOriginalFragment();

    // IPropCalculation virtuals
    virtual vector<string> propertyNames();
       
    // ChemistryTask virtuals
    virtual bool fragment(Fragment* frag); 
    virtual bool hasFragment(); 
    virtual bool getFragment(Fragment& frag);
    virtual bool hasOriginalFragment();
    virtual bool setOriginalFragment(istream *is,
                                     int selectedModel = 1,
                                     const string& alternatelocation = " ",
                                     const string& chainId = " ",
                                     const string& name = "");
    virtual bool getOriginalFragment(Fragment& frag);

    istream *getInputFragmentAsFile();
    bool getTaskFragment(Fragment& frag);
    bool getFragmentSummary(FragmentSummary& fragSum);
    bool getTaskFragmentSummary(FragmentSummary& fragSum);
    bool hasInputFragment();

    string getSystemName() const throw(InvalidException, DavException);
    string getCalcName() const; 
    string getOriginalFragmentName() const;
    string getOutputFragmentName() const;
    string getInputFragmentName() const;
    string getRestartName() const throw(InvalidException, DavException);
    string getTopologyName() const throw(InvalidException, DavException);
    string getMdOutputName() const;

    MdTask *getPdbProvider();

    bool canModifyInputProvider();

    bool hasPmf() const;

    virtual bool remove(Session * session, bool deleteTarget);

    // Resource virtuals
    virtual bool remove();
    virtual bool move(EcceURL& target);
    virtual bool copy(EcceURL& target);
    virtual void getDeleteMessage(bool & canDelete, string & message);

    // TaskJob virtuals
    virtual bool canChangeState(string & message) const;
    virtual bool resetForRerun(const ResourceDescriptor::RUNSTATE&
                               tostate = ResourceDescriptor::STATE_ILLEGAL);
    virtual bool resetForRestart();

  protected:
    bool getFragmentSummary(FragmentSummary& fragSum, const EcceURL& url);
    bool getOriginalFragmentProperties(FragmentSummary& fragSum);

    MdTask(const EcceURL& url) throw(InvalidException);
    bool getFragment(Fragment& frag, const EcceURL& url);

  private:

    bool getOutputFragment(istream *is, Fragment& frag) const;
    bool getOutputFragmentSummary(istream *is, FragmentSummary& fragSum) const;
 
};


#endif
