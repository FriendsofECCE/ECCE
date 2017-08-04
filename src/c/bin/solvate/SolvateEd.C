#include <fstream>
  using std::ifstream;

#include "util/BrowserHelp.H"
#include "util/TempStorage.H"
#include "util/Ecce.H"
#include "util/SDirectory.H"

#include "tdat/Fragment.H"
#include "tdat/TPerTab.H"
#include "tdat/LatticeDef.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalculation.H"
#include "dsm/ResourceTool.H"
#include "dsm/ResourceType.H"

#include "wxgui/EcceTool.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WxFeedback.H"

#include "SolvateEd.H"

//#define DEBUG

/** Used to subscribe to messaging, publish, identify */
static const string WHOAMI = SOLVATE;

IMPLEMENT_CLASS( SolvateEd, SolvateEdGUI )

BEGIN_EVENT_TABLE(SolvateEd, SolvateEdGUI)
END_EVENT_TABLE()


SolvateEd::SolvateEd( )
  : SolvateEdGUI(),
    WxDavAuth(),
    JMSPublisher(SOLVATE),
    FeedbackSaveHandler(),
    CalcDropHandler(),
    p_feedback(NULL),
    p_builderTool(NULL),
    p_iCalc(NULL),
    p_frag(NULL),
    p_fullFrag(NULL),
    p_context(""),
    p_scaleRadius(1.0),
    p_nx(1),
    p_ny(1),
    p_nz(1)
{
}


SolvateEd::SolvateEd( wxWindow* parent, wxWindowID id, const wxString& caption,
                const wxPoint& pos, const wxSize& size, long style)
  : SolvateEdGUI(),
    WxDavAuth(),
    JMSPublisher(SOLVATE),
    FeedbackSaveHandler(),
    CalcDropHandler(),
    p_feedback(NULL),
    p_builderTool(NULL),
    p_iCalc(NULL),
    p_frag(NULL),
    p_fullFrag(NULL),
    p_context(""),
    p_scaleRadius(1.0),
    p_nx(1),
    p_ny(1),
    p_nz(1)
{
  Create(parent, id, caption, pos, size, style);
}


SolvateEd::~SolvateEd( )
{
}


bool SolvateEd::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                     const wxPoint& pos, const wxSize& size, long style)
{
  if (!SolvateEdGUI::Create(parent, id, caption, pos, size, style)) {
    wxFAIL_MSG( wxT("Solvation Editor creation failed") );
    return false;
  }

  CreateControls();
  createSolventChoices();
  //GetSizer()->SetSizeHints(this);
  //GetSizer()->SetMinSize(SYMBOL_SOLVATEEDGUI_SIZE);
  Centre();

  // set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(
          ResourceDescriptor::getResourceDescriptor().getTool(SOLVATE)
          ->getIcon()), wxBITMAP_TYPE_XPM));
  EDSIFactory::addAuthEventListener(this);

  // Set input field properties
  ewxTextCtrl *text = 0;
  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_RAD));
  validator.setHardRange("(0.0,)");
  validator.setValue(1.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_MIN_NX));
  validator.setHardRange("[1,)");
  validator.setValue(1);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_MIN_NY));
  validator.setHardRange("[1,)");
  validator.setValue(1);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_MIN_NZ));
  validator.setHardRange("[1,)");
  validator.setValue(1);
  text->SetValidator(validator);

  return true;
}


void SolvateEd::createSolventChoices()
{
  string solPath = Ecce::ecceDataPath();
  solPath += "/client/solvents";
  SDirectory solDir(solPath);

  EcceSortedVector<string, less<string> > files;
  solDir.getFileNames(files);

  ewxComboBox *combo = (ewxComboBox*) FindWindow(ID_COMBOBOX_SOLVATEED_SOLVENT);
  combo->Clear();

  string choice;
  int cnt = files.size();
  for (int idx=0; idx<cnt; idx++) {
    const char* file = files[idx].c_str();

    if (&file[strlen(file)-4] == strstr(file,".xyz")) {
      choice = file;
      choice.resize(choice.length()-4);
      combo->Append(choice.c_str());
    }
  }

  if (cnt > 0) {
    // select the first 
    combo->SetSelection(0);
  }
}


void SolvateEd::setContext(const string& url)
{
  // Make sure it is raised/uniconified for popup dialogs
  Raise();

  // A calculation with unsaved changes?
  if (p_iCalc && url != p_context
          && p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
    ewxMessageDialog *dialog = new ewxMessageDialog(this,
            "The current calculation has unsaved changes!  Do you "
            "want to save changes before changing context?",
            "ECCE Change Context?", wxYES_NO|wxICON_QUESTION);
    wxWindowID answer = dialog->ShowModal();
    if (answer == wxID_YES) {
      doSave();
    }
  } else if (p_iCalc && url==p_context) {
    // bail because it's the same calculation
    return;
  }

  // decided to set context...
  // first let's make sure all is ok with the object we were handed
  bool doit = false;
  ICalculation *givenCalc =
          dynamic_cast<ICalculation*>(EDSIFactory::getResource(url));
  try {
    if (givenCalc) {
      doit = givenCalc->isValid();
    } else {
      p_feedback->setMessage("Could not retrieve calculation at "
              + url  + ".", WxFeedback::ERROR);
    }
  } catch (CancelException& ex) {
    p_feedback->setMessage("Cancelled validation during context switch.",
            WxFeedback::INFO);
  } catch (RetryException& ex) {
    ewxMessageDialog *dialog = new ewxMessageDialog(this,
            "The maximum number of authentication attempts has been "
            "exceeded.  Contact your ECCE administrator if you have "
            "forgotten your password.", "Authentication Failure");
    dialog->ShowModal();
  }

  if (doit) {
    freeContext();
    bool msgFlag = p_iCalc != (ICalculation*)0;
    p_iCalc = givenCalc;
    doSetContext();
    if (msgFlag) {
      p_feedback->setMessage("Calculation context set to " + p_iCalc->getName()
                             + ".", WxFeedback::INFO);
    }
  } else if (p_iCalc != (ICalculation*)0) {
    p_feedback->setMessage("Calculation context was not changed.",
                           WxFeedback::INFO);
  }
}


void SolvateEd::doSetContext()
{
  // hack to support two builder implementations
  fixBuilderButtonId();

  p_context = p_iCalc->getURL().toString();

  // initialize the feedback area
  p_feedback->clearMessage();
  p_feedback->setContextURL(p_context);
  if (!p_iCalc->messages().empty()) {
    // if the calculation contains messages, something is wrong
    p_feedback->setContextLabel("");
    p_feedback->setMessage(p_iCalc->messages(), WxFeedback::ERROR);
    setEditStatus(WxFeedback::READONLY);
    return;
  }

  p_feedback->setContextLabel(EDSIServerCentral().mapURLtoName(p_context));
  p_feedback->setRunState(p_iCalc->getState());
  if (p_feedback->getRunState() > ResourceDescriptor::STATE_READY) {
    setEditStatus(WxFeedback::READONLY);
  }
  enableSave(false);

  p_frag = p_iCalc->fragment();
  if (p_frag) {
    p_fullFrag = new Fragment(*p_frag);

    // Display system size
    int firstSolventAtom = p_frag->getFirstSolventAtomIndex();
    int numAtoms = p_frag->numAtoms();
    if (firstSolventAtom > 0 && firstSolventAtom < numAtoms-1) {
      refreshSystemDims(true);
    } else {
      refreshSystemDims(false);
    }
  } else {
    refreshSystemDims(false);
  }

  updateChemSysFields();
}


void SolvateEd::freeContext()
{
  if (p_frag) {
    delete p_frag;
    p_frag = 0;
  }

  if (p_fullFrag) {
    delete p_fullFrag;
    p_fullFrag = 0;
  }

  p_context = "";
  p_iCalc = 0;
 
  // clear the feedback area
  p_feedback->clearMessage();
  p_feedback->setContextURL("");
  p_feedback->setContextLabel("");
  p_feedback->setRunState(ResourceDescriptor::STATE_CREATED);
  setEditStatus(WxFeedback::EDIT);
  enableSave(false);

  updateChemSysFields();
}


string SolvateEd::getContext() const
{
  return p_context;
}

void SolvateEd::requestBuilder(const string& actionTopic)
{
  string url = getContext();

  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("action","start");
  publisher.publish("ecce_activity",*msg);
  delete msg;

  // Send the start app request to gateway:
  Target gateway(GATEWAY, "");
  msg = publisher.newMessage(gateway);
  // Ensure that this message will carry original sender info for
  // sending point to point responses after startup
  msg->markAsPointToPoint();


  // which builder do we invoke?  find out now!
  Resource *resource = EDSIFactory::getResource(url);
  if (resource) {
    ResourceType *resourceType = resource->getDescriptor();
    if (resourceType) {
      vector<ResourceTool*> tools = resourceType->getTools();
      for (int i = 0; i < tools.size(); i++) {
        string name = tools[i]->getName();
        if (name == BUILDER) {
          publisher.invoke(*msg, name, 0, url, actionTopic);
          break;
        }
      }
    }
  }

  delete msg;
}

void SolvateEd::setEditStatus(WxFeedback::EditStatus status)
{
  p_feedback->setEditStatus(status);
}


Fragment *SolvateEd::getFrag() const
{
  return p_frag;
}


WxFeedback *SolvateEd::getFeedback()
{
  return p_feedback;
}


void SolvateEd::processSave()
{
  doSave();
}


void SolvateEd::processDrop(const string& url)
{
  setContext(url);
}


/**
 * The user clicked on one of the EcceTools (Builder, Basis, Launcher).
 */
void SolvateEd::OnToolClick(wxCommandEvent &event)
{
  doSave();
  startApp(event.GetId(), 0, p_context);
}


void SolvateEd::OnCloseWindow( wxCloseEvent& event )
{
  doClose(event.CanVeto());
}


void SolvateEd::OnSaveClick( wxCommandEvent& event )
{
  doSave();
  event.Skip();
}


void SolvateEd::OnTextctrlSlvRadEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_RAD));
  p_scaleRadius = atof(text->GetValue().c_str());
  event.Skip();
}

void SolvateEd::OnTextctrlSlvMinNxEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_MIN_NX));
  p_nx = atoi(text->GetValue().c_str());
  event.Skip();
}


void SolvateEd::OnTextctrlSlvMinNyEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_MIN_NY));
  p_ny = atoi(text->GetValue().c_str());
  event.Skip();
}


void SolvateEd::OnTextctrlSlvMinNzEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_SLV_MIN_NZ));
  p_nz = atoi(text->GetValue().c_str());
  event.Skip();
}


void SolvateEd::OnExitClick( wxCommandEvent& event )
{
  doClose();
}


void SolvateEd::OnHelpClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showPage(help.URL("CalculationEditor"));
}


void SolvateEd::OnMenuFeedbackClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showFeedbackPage();
}


void SolvateEd::OnButtonSolvateEdClearSlvClick( wxCommandEvent& event )
{
  if (p_iCalc) {
    Fragment *frag = getFrag();
    int firstSolventAtom = frag->getFirstSolventAtomIndex();
    int numAtoms = frag->numAtoms();
    int i;
    for (i=numAtoms-1; i>=firstSolventAtom; i--) {
      frag->deleteAtom(i,false);
    }
    frag->deleteAllBonds();
    frag->addCovalentBonds(true);
    p_iCalc->fragment(frag);
    refreshSystemDims(false);
    if (p_feedback->getRunState() > ResourceDescriptor::STATE_READY)
      setRunState(ResourceDescriptor::STATE_READY);
    requestBuilder("refresh");
  }
  event.Skip();
}


void SolvateEd::OnButtonSolvateEdSolvateClick( wxCommandEvent& event )
{
  if (p_iCalc) {
    static const int BUFSIZE = 240;
    TPerTab tpt;
    char lbuf[BUFSIZE];
    int numSolvAtoms = 0;
    int solvSize = 0;
    double xbox, ybox, zbox, xbox2, ybox2, zbox2, xinc, yinc, zinc;
    double coord[3];

    // Open solvent file and create a temporary fragment with solvent
    // TODO: Some sort of error handling
    string solPath = Ecce::ecceDataPath();
    solPath += "/client/solvents/";
    ewxComboBox *combo = (ewxComboBox*) FindWindow(ID_COMBOBOX_SOLVATEED_SOLVENT);
    solPath += combo->GetStringSelection().c_str();
    solPath += ".xyz";
    //cout<< "Opening solvent file: " << solPath << endl;

    ifstream infile(solPath.c_str());
    infile.getline(lbuf,BUFSIZE-1,'\n');
    (void)sscanf(lbuf,"%d %d",&numSolvAtoms,&solvSize);
    //if (narg < 2) {
    //  cout<< "Insufficient entries in first line of solvent file"<<endl;
    //}
    // TODO: Error if numSolvAtoms%solvSize != 0
    //if (numSolvAtoms%solvSize !=0) {
    //  cout<< "Size of solvent molecule, "<<solvSize<<", is not an even divisor"
    //    <<" of total number of atoms in solvent file "<<numSolvAtoms<<endl;
    //}
    infile.getline(lbuf,BUFSIZE-1,'\n');
    sscanf(lbuf,"%lf %lf %lf",&xbox,&ybox,&zbox);
    int i=0;
    int iat;
    char atype[5];
    double x, y, z;
    string sym;
    Fragment solvFrag;
    while (i<numSolvAtoms && infile.good()) {
      infile.getline(lbuf,BUFSIZE-1,'\n');
      sscanf(lbuf,"%s %lf %lf %lf",atype,&x,&y,&z);
      coord[0] = x;
      coord[1] = y;
      coord[2] = z;
      if (strlen(atype) > 1) {
        char tmp[3];
        tmp[0] = atype[0];
        tmp[1] = atype[1];
        tmp[2] = '\0';
        sym = tmp;
        if (!tpt.isValid(sym)) {
          sym.clear();
          tmp[1] = '\0';
          sym = tmp;
          //if (!tpt.isValid(sym)) {
          //  cout<<"No valid atom symbol found for solvent atom type "<<atype<<endl;
          //}
        }
      } else {
        sym = atype;
      }
      solvFrag.addAtom(sym.c_str(),coord);
      TAtm *atm = solvFrag.atomRef(i);
      atm->atomType(atype);
      i++;
    }
    //cout <<" Completed read of solvent file"<<endl;
    solvFrag.recenterFrag();
    //cout <<" Recentered solvent fragment"<<endl;
    // TODO: Add some code to make sure all atoms on same molecular are within a
    // minimum image of each other

    // Estimate size of current fragment
    Fragment *frag = getFrag();
    if (frag) {
      frag->recenterFrag();
      //cout <<" Recentered solute fragment"<<endl;
      double xmin, ymin, zmin, xmax, ymax, zmax;
      int numAtoms = frag->numAtoms();
      if (numAtoms > 0) {
        TAtm *atm = frag->atomRef(0);
        const double *coords = atm->coordinates();
        xmin = coords[0];
        xmax = coords[0];
        ymin = coords[1];
        ymax = coords[1];
        zmin = coords[2];
        zmax = coords[2];
        string type;
        for (i=0; i<numAtoms; i++) {
          atm = frag->atomRef(i);
          //set atom type field to default value if it is empty
          type = atm->atomType();
          //cout<<"solute atom type: "<<"("<<type<<")"<<endl;
          if (type == "    ") {
            type = atm->atomicSymbol();
            type.append("1");
            while (type.size()<4) {
              type.append(" ");
            }
            atm->atomType(type.c_str());
          }
          const double *coords = atm->coordinates();
          if (coords[0] < xmin) xmin = coords[0];
          if (coords[0] > xmax) xmax = coords[0];
          if (coords[1] < ymin) ymin = coords[1];
          if (coords[1] > ymax) ymax = coords[1];
          if (coords[2] < zmin) zmin = coords[2];
          if (coords[2] > zmax) zmax = coords[2];
        }
        //cout <<"xmin: "<<xmin<<" xmax: "<<xmax<<" ymin: "<<ymin<<" ymax: "<<ymax
        //  <<" zmin: "<<zmin<<" zmax: "<<zmax<<endl;
        //cout <<"xbox: "<<xbox<<" ybox: "<<ybox<<" zbox: "<<zbox<<endl;
        // Find out how many replicas of solvent box you will need to completely
        // solvate existing fragment
        // TODO: Add some padding?
        int nx = (int)((xmax-xmin)/xbox);
        nx++;
        int ny = (int)((ymax-ymin)/ybox);
        ny++;
        int nz = (int)((zmax-zmin)/zbox);
        nz++;
        // If nx, ny, nz are less than minimum values from GUI, reset them
        if (nx < p_nx) nx = p_nx;
        if (ny < p_ny) ny = p_ny;
        if (nz < p_nz) nz = p_nz;
        //cout <<"nx: "<<nx<<" ny: "<<ny<<" nz: "<<nz<<endl;
        xinc = xbox;
        yinc = ybox;
        zinc = zbox;
        xbox = xbox*((double)nx);
        ybox = ybox*((double)ny);
        zbox = zbox*((double)nz);
        //cout <<"xbox: "<<xbox<<" ybox: "<<ybox<<" zbox: "<<zbox<<endl;
        xbox2 = 0.5*xinc;
        ybox2 = 0.5*yinc;
        zbox2 = 0.5*zinc;
        //cout <<"xbox2: "<<xbox2<<" ybox2: "<<ybox2<<" zbox2: "<<zbox2<<endl;
        // Loop through solvent boxes and original fragment to find out which
        // solvent atoms can be added to the fragment
        int jx, jy, jz;
        int ismol,isat,it;
        int numMol = numSolvAtoms/solvSize;
        int lastAtom;
        bool ok;
        TAtm *solv;
        TAtm *newsolv;
        TAtm *solut;
        double solvrad, solurad;
        double rx, ry, rz, r;
        int istart, iend;
        double coordnew[3];
        bool newAtom = true;
        int newAtoms = frag->numAtoms();
        // TODO: If performance is bad, will need to use a linked cell list for
        // overlap calculation
        for (jx=0; jx<nx; jx++){
          for (jy=0; jy<ny; jy++){
            for (jz=0; jz<nz; jz++){
              isat = 0;
              // Loop over solvent molecules
              //         cout<<"Starting loop over solvent molecules"<<endl;
              for (ismol=0; ismol<numMol; ismol++) {
                ok = true;
                // Loop over atoms in solvent molecule
                //          cout<<"Starting loop over solvent molecule atoms"<<endl;
                istart = isat;
                for (it=0; it<solvSize; it++) {
                  solv = solvFrag.atomRef(isat);
                  solvrad = tpt.vwr(solv->atomicNumber());
                  double const *coordv = solv->coordinates();
                  // Loop over solute atoms and check for overlaps
                  for (iat=0; iat<numAtoms; iat++) {
                    solut = frag->atomRef(iat);
                    solurad = tpt.vwr(solut->atomicNumber());
                    double const *coordu = solut->coordinates();
                    // evaluate minimum image separation
                    rx = coordv[0]-coordu[0]-0.5*((double)(nx-1))*xinc+((double)jx)*xinc;
                    ry = coordv[1]-coordu[1]-0.5*((double)(ny-1))*yinc+((double)jy)*yinc;
                    rz = coordv[2]-coordu[2]-0.5*((double)(nz-1))*zinc+((double)jz)*zinc;
                    if (rx/xbox > 0.5) {
                      rx = rx - xbox;
                    } else if (rx/xbox < -0.5) {
                      rx = rx + xbox;
                    }
                    if (ry/ybox > 0.5) {
                      ry = ry - ybox;
                    } else if (ry/ybox < -0.5) {
                      ry = ry + ybox;
                    }
                    if (rz/zbox > 0.5) {
                      rz = rz - zbox;
                    } else if (rz/zbox < -0.5) {
                      rz = rz + zbox;
                    }
                    r = sqrt(rx*rx+ry*ry+rz*rz);
                    if (r < p_scaleRadius*(solvrad+solurad)) {
                      ok = false;
                      break;
                    }
                  }
                  isat++;
                }
                iend = isat;
                //           cout<<"Completed loop over solvent molecule atoms ok: "<<ok<<endl;
                if (ok) {
                  //Add solvent molecule to fragment if no overlap detected
                  for (it=istart; it<iend; it++) {
                    solv = solvFrag.atomRef(it);
                    //              cout<<"atomType: "<<solv->atomicSymbol().c_str()<<endl;
                    double const *coordv = solv->coordinates();
                    coordnew[0] = coordv[0]+((double)jx)*xinc-0.5*((double)(nx-1))*xinc;
                    coordnew[1] = coordv[1]+((double)jy)*yinc-0.5*((double)(ny-1))*yinc;
                    coordnew[2] = coordv[2]+((double)jz)*zinc-0.5*((double)(nz-1))*zinc;
                    frag->addAtom(solv->atomicSymbol().c_str(),coordnew);
                    newsolv = frag->atomRef(newAtoms);
                    string tag = solv->atomType();
                    newsolv->atomType(tag.c_str());
                    newAtoms++;
                    if (newAtom) {
                      lastAtom = frag->numAtoms();
                      lastAtom--;
                      frag->setFirstSolventAtomIndex(lastAtom);
                      //cout<<"First solvent atom: "<<lastAtom<<endl;
                      newAtom = false;
                    }
                  }
                }
                //            cout<<"Add solvent molecule atoms to solute fragment"<<endl;
              }
            }
          }
        }

        // Create a LatticeDef object to describe the box and add it to fragment
        LatticeDef *lattice = new LatticeDef(xbox,ybox,zbox,90.0,90.0,90.0);
        frag->setLattice(lattice);
        frag->setSolventNumAtoms(solvSize);
        frag->deleteAllBonds();
        frag->addCovalentBonds(true);
        refreshSystemDims(true);
        //cout << "Number of atoms: "<<frag->numAtoms()<<endl;
        p_iCalc->fragment(frag);
        setRunState(ResourceDescriptor::STATE_COMPLETED);
        requestBuilder("refresh");
      } else {
        p_feedback->setMessage("Chemical system to solvate contains no atoms.",
                               WxFeedback::ERROR);
      }
    } else {
      p_feedback->setMessage("Chemical system to solvate must be specified.",
                             WxFeedback::ERROR);
    }
  }
  event.Skip();
}


void SolvateEd::moveMCB(wxCommandEvent& event)
{
  p_feedback->setContextURL(p_iCalc->getURL());
}


void SolvateEd::deleteMCB(wxCommandEvent& event)
{
  freeContext();
}


void SolvateEd::stateMCB(wxCommandEvent& event)
{
  if (p_iCalc->getState() > ResourceDescriptor::STATE_READY) {
    setEditStatus(WxFeedback::READONLY);
  } else {
    setEditStatus(WxFeedback::EDIT);
  }
  enableChemSysFields();
}


void SolvateEd::subjectMCB(wxCommandEvent& event)
{
  // reinitialize fragment(s)
  if (p_frag) {
    delete p_frag;
    p_frag = 0;
  }
  if (p_fullFrag) {
    delete p_fullFrag;
    p_fullFrag = 0;
  }

  p_frag = p_iCalc->fragment();
  if (p_frag) {

    p_fullFrag = new Fragment(*p_frag);
    if (p_frag->useSymmetry()) {
      p_fullFrag->generateFullMolecule();
    }
  }

  updateChemSysFields();
  if (p_iCalc->getState() == ResourceDescriptor::STATE_CREATED)
    setRunState(ResourceDescriptor::STATE_READY);

  // auto-save
  enableSave();
  doSave();
}


void SolvateEd::CreateControls()
{
  p_feedback = new WxFeedback(this);
  p_feedback->setDropHandler(this);
  p_feedback->setSaveHandler(this);
  GetSizer()->Add(p_feedback, 0, wxEXPAND, 0);

  replaceBuilderButton();
}


void SolvateEd::replaceBuilderButton()
{
  // Hack to support duality of Builders (wx vs motif)
  // Create an EcceTool now to fill the space.  When setContext is called
  // with a valid calculation, we'll determine which Builder is to be 
  // invoked and reset the ID of the EcceTool.
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceTool *tool = rd.getTool(BUILDER);
  p_builderTool = new EcceTool(this, tool);
  Connect( tool->getId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( SolvateEd::OnToolClick ) );

  // find and replace the Builder button placeholder
  wxWindow *placeHolder = FindWindow(ID_BUTTON_SOLVATEED_BUILDER);
  placeHolder->GetContainingSizer()->Replace(placeHolder, p_builderTool);
  placeHolder->Destroy();
}


void SolvateEd::fixBuilderButtonId()
{
  vector<ResourceTool*> tools = p_iCalc->getDescriptor()->getTools();
  vector<ResourceTool*>::iterator tool;
  for (tool = tools.begin(); tool != tools.end(); tool++) {
    string name = (*tool)->getName();
    if (name == BUILDER) {
      break;
    }
  }

  Disconnect( p_builderTool->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
              wxCommandEventHandler( SolvateEd::OnToolClick ) );
  p_builderTool->SetId((*tool)->getId());
  Connect( p_builderTool->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( SolvateEd::OnToolClick ) );
}


void SolvateEd::updateChemSysFields()
{
  refreshChemSysFields();
  enableChemSysFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void SolvateEd::refreshChemSysFields()
{
  refreshChemSysThumb();
}


void SolvateEd::refreshSystemDims(bool show)
{
  ewxStaticText *text;
  char buf[32];
  if (show) {
    double x, y, z, alpha, beta, gamma;
    if (p_frag->getLattice()) {
      p_frag->getLattice()->getLattice(x,y,z,alpha,beta,gamma);
    }
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_DIM_HEADER));
    text->Show(true);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_X_LABEL));
    text->Show(true);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_X_DIM));
    text->Show(true);
    sprintf(buf,"%.6f",x);
    text->SetLabel(buf);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Y_LABEL));
    text->Show(true);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Y_DIM));
    text->Show(true);
    sprintf(buf,"%.6f",y);
    text->SetLabel(buf);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Z_LABEL));
    text->Show(true);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Z_DIM));
    text->Show(true);
    sprintf(buf,"%.6f",z);
    text->SetLabel(buf);
  } else {
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_DIM_HEADER));
    text->Show(false);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_X_LABEL));
    text->Show(false);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_X_DIM));
    text->Show(false);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Y_LABEL));
    text->Show(false);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Y_DIM));
    text->Show(false);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Z_LABEL));
    text->Show(false);
    text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_Z_DIM));
    text->Show(false);
  }

  // Set number of atoms
  text = ((ewxStaticText*)FindWindow(ID_STATIC_SLV_TATOMS));
  if (p_frag) {
    sprintf(buf,"%d",(int)p_frag->numAtoms());
    text->SetLabel(buf); 
  } else {
    sprintf(buf,"%d",0);
    text->SetLabel(buf); 
  }
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


/**
 * Set the bitmap for the chemical system or just the vanilla bitmap.
 */
void SolvateEd::refreshChemSysThumb()
{
  bool restoreStandardBitmap = false;

  if (p_iCalc) {
    SFile *thumbnail = TempStorage::getTempFile();
    if (p_iCalc->getThumbnail(thumbnail)) {
      wxBitmap bitmap(_T(thumbnail->path()), wxBITMAP_TYPE_JPEG);
      if (bitmap.Ok()) 
        p_builderTool->setBitMap(bitmap);
      else
        restoreStandardBitmap = true;
    } else {
      restoreStandardBitmap = true;
    }
    thumbnail->remove();
    delete thumbnail;
  } else {
    restoreStandardBitmap = true;
  }

  if (restoreStandardBitmap) {
    ResourceTool *tool = ResourceDescriptor::getResourceDescriptor()
                                    .getTool(p_builderTool->GetId());
    p_builderTool->setBitMap(ewxBitmap(tool->getIcon(), wxBITMAP_TYPE_XPM));
  }
}


/**
 * Enable/Disable fields based on whether a chemical system has been specified.
 */
void SolvateEd::enableChemSysFields()
{
  bool hasCalc(p_iCalc);

  p_builderTool->Enable(hasCalc);
}


void SolvateEd::enableSave(const bool& enable)
{
  if (p_iCalc && p_feedback->getEditStatus() != WxFeedback::READONLY) {
    setEditStatus(enable ? WxFeedback::MODIFIED : WxFeedback::EDIT);
    GetMenuBar()->GetMenu(GetMenuBar()->FindMenu("File"))
                                      ->Enable(wxID_SAVE, enable);
  } else {
    GetMenuBar()->GetMenu(GetMenuBar()->FindMenu("File"))
                                      ->Enable(wxID_SAVE, false);
  }
}


void SolvateEd::urlSubjectNotify() const
{
  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("url", p_iCalc->getURL().toString());
  publish("ecce_url_subject", *msg);
  delete msg;
}


void SolvateEd::setRunState(ResourceDescriptor::RUNSTATE state) const
{
  p_iCalc->setState(state);
  p_feedback->setRunState(state);
}




wxWindowID SolvateEd::doClose(const bool& allowCancel)
{
  wxWindowID answer = wxID_ANY;

  if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
    long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
    buttonFlags |= allowCancel ? wxCANCEL : 0;
    ewxMessageDialog *dialog = new ewxMessageDialog(this,
            "The current calculation has unsaved changes!  Do you "
            "want to save changes before quitting?",
            "Save Solvation Editor Changes?", buttonFlags);
    answer = dialog->ShowModal();
    if (answer != wxID_CANCEL) {
      if (answer == wxID_YES) {
        doSave();
      }
      Destroy();
    }
  } else {
    Destroy();
  }

  return answer;
}


void SolvateEd::doSave()
{
  // check that there's really been something changed
  if (p_feedback->getEditStatus() == WxFeedback::MODIFIED)
  {
    // save fragment
    // name, charge, and use irreducible are part of the frag
    bool saveFrag = false;
    if (p_frag) {
      Fragment *currentFragment = p_iCalc->fragment();
      if (!currentFragment) {
        p_iCalc->fragment(p_frag);
        saveFrag = true;
      } else {
        // update the old fragment with new values
        p_iCalc->fragment(currentFragment);
        saveFrag = true;

        delete currentFragment;
      }
    }

    // reset the save status indicator
    enableSave(false);

    p_feedback->setMessage("Calculation saved as " + p_iCalc->getName() + ".",
                           WxFeedback::INFO);

    if (saveFrag)
      urlSubjectNotify();
  }
}


void SolvateEd::startApp(int id, int force, const string& url) const
{
  startApp(ResourceDescriptor::getResourceDescriptor().getTool(id)->getName(),
           force, url);
}


void SolvateEd::startApp(const string& app, int force, const string& url) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("action", "start");
  publish("ecce_activity", *msg);
  delete msg;

  Target gateway(GATEWAY, "");
  msg = newMessage(gateway);
  msg->addProperty("appname", app);
  msg->addIntProperty("forcenew", force);
  msg->addProperty("calcurl", url);
  publish("ecce_get_app", *msg);
  delete msg;
}
