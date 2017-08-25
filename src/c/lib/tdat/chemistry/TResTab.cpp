///////////////////////////////////////////////////////////////////////////////
//
//   The TResTab provides a simple method of comparing a residue name against
//   a list of known residues and determining if the residue corresponds to
//   an amino acid or a DNA or RNA base pair. This functionality is necessary
//   for setting up molecular dynamics calculations of biochemical systems.
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::cout;
  using std::endl;
#include <stdio.h>
#include <fstream>
  using std::ifstream;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include <string.h>

#include "tdat/TResTab.H"
#include "tdat/TResItem.H"
#include "util/Ecce.H"
#include "util/STLUtil.H"

vector<TResItem*>* TResTab::p_resItemList = 0;


///////////////////////////////////////////////////////////////////////////////
//  Destructor
///////////////////////////////////////////////////////////////////////////////
TResTab::~TResTab()
{
}
///////////////////////////////////////////////////////////////////////////////
//  Constructor
///////////////////////////////////////////////////////////////////////////////
TResTab::TResTab()
{
   p_resItemList = 0;
   initialize();
}

void TResTab::initialize()
{
   if (p_resItemList != 0) return;

   // Check to see if list of TResItems already exists. If not, construct one.


   string infile = Ecce::ecceDataPrefPath();
   infile += "/ResidueList";
   ifstream is(infile.c_str());
   char cbuf[256];
   char name[256];
   char biostatus[256];
   char c_charge[256];
   char c_hydrophobicity[256];
   int charge, hydrophobicity;
   p_resItemList = new vector<TResItem*>;
   TResItem* res;
   TResItem::Classification classification;
   while (is.getline(cbuf,255)) {
      sscanf(cbuf,"%s%s%s%s",name,biostatus,c_charge,c_hydrophobicity);
      if (strcmp(biostatus,"AminoAcid")==0) {
         classification = TResItem::AminoAcid;
      } else if (strcmp(biostatus,"RNAorDNA")==0) {
         classification = TResItem::RNAorDNA;
      } else if (strcmp(biostatus,"RNA")==0) {
         classification = TResItem::RNA;
      } else if (strcmp(biostatus,"DNA")==0) {
         classification = TResItem::DNA;
      } else {
         classification = TResItem::Other;
      }
      if (classification != TResItem::Other) {
         res = new TResItem(name,classification);
         charge = atoi(c_charge);
         res->charge(charge);
         hydrophobicity = atoi(c_hydrophobicity);
         res->hydrophobicity(hydrophobicity);
         p_resItemList->push_back(res);
      }
   }
   is.close();
}

void
TResTab::finalize()
{
  if (p_resItemList != 0) {
    for (size_t i=0; i<p_resItemList->size(); i++) {
      delete (*p_resItemList)[i];
    }
    p_resItemList->clear();

  }
  p_resItemList = 0;
}

vector<string> TResTab::getAminoAcids() 
{
   initialize();

   vector<string> ret;
   if (p_resItemList != 0) {
      for (size_t i=0; i<p_resItemList->size(); i++) {
         if ((*p_resItemList)[i]->classification() == TResItem::AminoAcid) {
            ret.push_back((*p_resItemList)[i]->name());
         }
      }
   }

   return ret;
}


///////////////////////////////////////////////////////////////////////////////
// Return classification for a given residue name
///////////////////////////////////////////////////////////////////////////////
TResItem::Classification TResTab::verify(const char* resname)
{
  size_t i;
  string restring = resname;
  for (i=0; i<p_resItemList->size(); i++) {
    if ((*p_resItemList)[i]->name() == restring) {
      return (*p_resItemList)[i]->classification();
    }
  }
  return TResItem::Other;
}

///////////////////////////////////////////////////////////////////////////////
// Return charge associated with a given residue name
///////////////////////////////////////////////////////////////////////////////
int TResTab::charge(const char* resname)
{
  size_t i;
  int ret = 0;
  string restring = resname;
  for (i=0; i<p_resItemList->size(); i++) {
    if ((*p_resItemList)[i]->name() == restring) {
      return (*p_resItemList)[i]->charge();
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Return hydrophobicity associated with a given residue name
///////////////////////////////////////////////////////////////////////////////
int TResTab::hydrophobicity(const char* resname)
{
  size_t i;
  int ret = 0;
  string restring = resname;
  for (i=0; i<p_resItemList->size(); i++) {
    if ((*p_resItemList)[i]->name() == restring) {
      return (*p_resItemList)[i]->hydrophobicity();
    }
  }
  return ret;
}

