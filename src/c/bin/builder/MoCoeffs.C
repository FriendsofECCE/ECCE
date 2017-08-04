#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <limits>

#include <wx/link.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h> // EVT_TOGGLEBUTTON
#include <wx/grid.h>
#include <wx/slider.h>

#include "util/InternalException.H"
#include "util/EventDispatcher.H"

#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "tdat/SingleGrid.H"
#include "tdat/TProperty.H"

#include "dsm/TGBSConfig.H"
#include "dsm/IPropCalculation.H"
#include "dsm/ICalculation.H"
#include "dsm/PropFactory.H"

#include "inv/ChemKit/ChemColor.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/WindowEvent.H"
#include "wxgui/ewxGrid.H"

#include "MoCoeffs.H"


// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( MoCoeffs, ewxDialog )

    //EVT_GRID_SELECT_CELL(MoCoeffs::OnMoSelection)

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(MoCoeffs, ewxDialog)


MoCoeffs::MoCoeffs()
  : ewxDialog()
{
}


MoCoeffs::MoCoeffs(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
  : ewxDialog(parent, id, title, pos, size, style, name)
{

   Create();
}

MoCoeffs::~MoCoeffs()
{
}



bool MoCoeffs::Create()
{
   //
   // Create simple gui with a spectrum and a tree control
   //

   ////@begin MoGUI content construction
   MoCoeffs* itemDialog1 = this;

   wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
   itemDialog1->SetSizer(itemBoxSizer2);

   wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 0);

   p_grid = new ewxGrid( itemDialog1, ID_MOCOEFFS_TABLE, wxDefaultPosition, wxSize(200, 500), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );


   p_grid->CreateGrid(8,5,wxGrid::wxGridSelectRows);
   p_grid->SetRowLabelSize(0);
   itemBoxSizer3->Add(p_grid, 1, wxGROW|wxALL, 0);


   wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

   wxStdDialogButtonSizer* itemStdDialogButtonSizer6 = new wxStdDialogButtonSizer;

   itemBoxSizer5->Add(itemStdDialogButtonSizer6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
   wxButton* itemButton7 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
   itemStdDialogButtonSizer6->AddButton(itemButton7);


   itemStdDialogButtonSizer6->Realize();


   ////@end MoCoeffs content construction




   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);


   initialize();

   return true;
}

void MoCoeffs::initialize()
{
   p_grid->DeleteRows(0,p_grid->GetNumberRows());
   p_grid->DeleteCols(0,p_grid->GetNumberCols());

   wxListItem itemCol;
   //itemCol.SetAlign(wxLIST_FORMAT_CENTRE);

   itemCol.SetText(_T("#"));

   p_grid->InsertCols(0,1);
   p_grid->SetColLabelValue(0," # ");

   p_grid->InsertCols(1,1);
   p_grid->SetColLabelValue(1,"Atom");

   p_grid->InsertCols(2,1);
   p_grid->SetColLabelValue(2,"Orb");

   p_grid->InsertCols(3,1);
   p_grid->SetColLabelValue(3,"Coef");
}




/**
 * Fill the table and initialize the UI.
 * This code copied from motool_cdlg but the graph code is currently excluded.
 */
void MoCoeffs::showCoeffs(ICalculation *expt, int moNum)
{
   SetTitle( wxString::Format (_T("ECCE MO Coefficients: %d"), moNum));

   p_grid->BeginBatch();

   // Clear table
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

   wxListItem itemCol;
   //itemCol.SetAlign(wxLIST_FORMAT_CENTRE);


   // Get the required properties 
//   PropVector *orbEnergy = (PropVector*) expt->getProperty("ORBENG");
//   PropVector *orbOcc = (PropVector*) expt->getProperty("ORBOCC");
//   PropVecString *orbSym = (PropVecString*) expt->getProperty("ORBSYM");
//   PropVecString *orbSymBeta = (PropVecString*) expt->getProperty("ORBSYMBETA");
   PropTable *moCoefs = (PropTable*) expt->getProperty("MO");

   //INTERNALEXCEPTION(orbEnergy,"Orbital energy vector is null.");

   Fragment frag;
   expt->getFragment(frag);
   TGBSConfig *gbsConfig = expt->gbsConfig();
   INTERNALEXCEPTION(gbsConfig,"basis set is null.");


   char buf[80];
   const char *coefFormat = "%+.4f";

   // Put up a little header saying what MO it is - include alpha/beta
   //sprintf(buf,"%s MO %d", (type=="alpha" ? "" : type.c_str()), moNum);
   //TODO put label somewher

   bool usingCartesian = (gbsConfig->coordsys()==TGaussianBasisSet::Cartesian);
   int length_d_shell = 5;
   int length_f_shell = 7;
   int length_g_shell = 9;
   int length_h_shell = 11;
   int length_i_shell = 13;
   if (usingCartesian) {
      length_d_shell = 6;
      length_f_shell = 10;
      length_g_shell = 15;
      length_h_shell = 21;
      length_i_shell = 28;
   }

   string atomID;

   size_t idxCoef = 0;

   size_t sCount, pCount, dCount, fCount, gCount, hCount, iCount;
   sCount = pCount = dCount = fCount = gCount = hCount = iCount = 0;

   int rows =  moCoefs->columns();

   // Fill in row numbers
   for (int r=0; r<rows; r++) {
      p_grid->InsertRows(r,1);
      p_grid->SetCellValue(r,0,wxString::Format (_T("%d"), r+1));
   }



   // For each atom:
   size_t numAtoms = frag.numAtoms();
   for (size_t idxAtom=0; idxAtom<numAtoms; idxAtom++) 
   {
      /**
        * Skip atom if it is just a point charge
        */
      if (frag.atomRef(idxAtom)->getBehaviorType() == TAtm::POINT) {
        continue;
      }
      sCount = pCount = dCount = fCount = gCount = hCount = iCount = 0;

      atomID = frag.atomRef(idxAtom)->atomicSymbol();

      vector<const TGaussianBasisSet*> gbslist = gbsConfig->getGBSList(atomID);
//      os_List<GaussianBasisSet*> gbslist;
//      gbsConfig->orbitals(gbslist,atomID);

#if (!defined(INSTALL) && defined(DEBUG))
    cout << " starting atom " << atomID << endl;
#endif 

      size_t numFuncTypes;  // length of funcTypes array ==> # of funs in a GBS 
      size_t idxAtom_plus1 = idxAtom + 1;
      sprintf(buf,"%zu",idxAtom_plus1);
      p_grid->SetCellValue(idxAtom,0,buf);

      sprintf(buf,"%s%zu",atomID.c_str(),idxAtom+1);
      p_grid->SetCellValue(idxCoef,1,buf);

      // loop over basis sets on the current atom (orbital, polarization, ...)
      vector <const TGaussianBasisSet*>::const_iterator gbscurs ;
      for (gbscurs = gbslist.begin(); gbscurs != gbslist.end();
            gbscurs++)
      {
         const TGaussianBasisSet* gbs = *gbscurs ;
  	     size_t numContractedSets = gbs->num_contracted_sets(atomID.c_str());

         for (size_t ics=0; ics<numContractedSets; ics++) {

            vector<TGaussianBasisSet::AngularMomentum> funcTypes =
                gbs->func_types(atomID.c_str(),ics);
            numFuncTypes = funcTypes.size();

            // this loop runs over the shells in the contraction set
            //-------------------------------------------------------------
            for (size_t ishell=0; ishell<numFuncTypes; ishell++) 
            {
               if (funcTypes[ishell] == TGaussianBasisSet::s_shell) {
                  sCount++;

                  // Atomic Orbital
                  size_t sCount_as_ul = sCount;
                  p_grid->SetCellValue(idxCoef,2,
                          wxString::Format (_T("S%zu"), sCount_as_ul));

                  // Coeff
                  sprintf(buf,coefFormat,moCoefs->value(moNum-1,idxCoef));
                  p_grid->SetCellValue(idxCoef,3, buf);

                  idxCoef++;

               }
               //-------------------------------------------------------------
               else if (funcTypes[ishell] == TGaussianBasisSet::p_shell) {
                  pCount++;
                  const char *klabels[] = {"x","y","z"};

                  for (int kdx=0; kdx<3; kdx++) {

                     // Atomic Orbital
                     sprintf(buf,"P%zu%s ",pCount,klabels[kdx]);
                     p_grid->SetCellValue(idxCoef,2, buf);

                     // Coeff
                     sprintf(buf,coefFormat,moCoefs->value(moNum-1,idxCoef));
                     p_grid->SetCellValue(idxCoef, 3, buf);

                     idxCoef++;
                  }
               } else if (funcTypes[ishell] == TGaussianBasisSet::d_shell) {
               //-------------------------------------------------------------
                  dCount++;

                  for (int kdx=0; kdx<length_d_shell; kdx++) {

                     // Atomic Orbital
                     sprintf(buf,"D%zu ",dCount);
                     p_grid->SetCellValue(idxCoef, 2, buf);

                     // Coeff
                     sprintf(buf,coefFormat,moCoefs->value(moNum-1,idxCoef));
                     p_grid->SetCellValue(idxCoef, 3, buf);

                     idxCoef++;
                  }
               } else if (funcTypes[ishell] == TGaussianBasisSet::f_shell) {
               //-------------------------------------------------------------
                  fCount++;

                  for (int kdx=0; kdx<length_f_shell; kdx++) {

                     // Atomic Orbital
                     sprintf(buf,"f%zu ",fCount);
                     p_grid->SetCellValue(idxCoef, 2, buf);

                     // Coeff
                     sprintf(buf,coefFormat,moCoefs->value(moNum-1,idxCoef));
                     p_grid->SetCellValue(idxCoef, 3, buf);

                     idxCoef++;
                  }
               } else if (funcTypes[ishell] == TGaussianBasisSet::g_shell) {
               //-------------------------------------------------------------
                  gCount++;

                  for (int kdx=0; kdx<length_g_shell; kdx++) {

                     // Atomic Orbital
                     sprintf(buf,"g%zu ",fCount);
                     p_grid->SetCellValue(idxCoef, 2, buf);

                     // Coeff
                     sprintf(buf,coefFormat,moCoefs->value(moNum-1,idxCoef));
                     p_grid->SetCellValue(idxCoef, 3, buf);

                     idxCoef++;
                  }
               } else if (funcTypes[ishell] == TGaussianBasisSet::h_shell) {
               //-------------------------------------------------------------
                  hCount++;

                  for (int kdx=0; kdx<length_h_shell; kdx++) {

                     // Atomic Orbital
                     sprintf(buf,"h%zu ",fCount);
                     p_grid->SetCellValue(idxCoef, 2, buf);

                     // Coeff
                     sprintf(buf,coefFormat,moCoefs->value(moNum-1,idxCoef));
                     p_grid->SetCellValue(idxCoef, 3, buf);

                     idxCoef++;
                  }
               } else if (funcTypes[ishell] == TGaussianBasisSet::i_shell) {
               //------------------------------------------------------------
                  iCount++;

                  for (int kdx=0; kdx<length_i_shell; kdx++) {

                     // Atomic Orbital
                     sprintf(buf,"i%zu ",fCount);
                     p_grid->SetCellValue(idxCoef, 2, buf);

                     // Coeff
                     sprintf(buf,coefFormat,moCoefs->value(moNum-1,idxCoef));
                     p_grid->SetCellValue(idxCoef, 3, buf);

                     idxCoef++;
                  }

               } else {
                  cerr << "Ohoh shell higher than i" << endl;
               }

#if (!defined(INSTALL) && defined(DEBUG))
         cout << "     idxCoef at bottom of ishell loop " << idxCoef << endl;
#endif 
            }  // end for ishell
         }  // end for ics
         //        break;
      }  // end for gbscurs
   } // end for idxAtom


   p_grid->EndBatch();


   p_grid->AutoSizeColumns();
   GetSizer()->Fit(this);


}



