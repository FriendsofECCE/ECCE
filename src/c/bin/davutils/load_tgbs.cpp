/**********************************************************************
 Program: load_tgbs.C

 Summary: This program takes DAV-formatted *.BAS, *.POT,
           *.BAS.meta, and *.POT.meta files and saves them to the
          development web server under the GaussianBasisSetLibrary
	  directory.

	  BEFORE running this program, you need to run gbsDAVConverter
	  (perl script).  This will create a subdirectory called 
	  testNewData.  This directory should be the source directory
	  you use to run this program.
***********************************************************************/

#include <strings.h>

#include <strstream>
  using std::ostrstream;
#include <fstream>
  using std::istream;
#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;

#include <vector>
#include <string>

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/SDirectory.H"
#include "util/EcceURL.H"
#include "util/CmdLineAuthListener.H"
#include "util/Ecce.H"
#include "util/EcceSortedVector.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/VDoc.H"


// function declaration:
   void load_gbs_data(EDSI* edsi, string& gbs_dir);
   void load_gbs_meta_data(EDSI* edsi, string& gbs_dir, string& destUrl);
   char* filter(const char*);
   void usage(void);

/**********************************************************************
  MAIN PROGRAM
***********************************************************************/
int main(int argc, char** argv) {

  Ecce::initialize();

  // Set up authentication
  CmdLineAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);

  // Set up default parameters if none are entered:
  EDSI* edsi = 0;
  string sourceUrl = Ecce::ecceDataGBSPath();
  sourceUrl += "/testNewData";
  EDSIServerCentral central;
  string destUrl = central.getDefaultBasisSetLibrary().toString();

  bool doData = true;
  bool doMeta = true;
  
  // check command-line arguments:
  if (argc == 1) {
    usage();
  }
  for(int idx = 1; idx < argc; idx++) {
    if(!strcmp(argv[idx],"-src")) {
      sourceUrl = argv[++idx];
    } else if(!strcmp(argv[idx],"-dest")) {
      destUrl = argv[++idx];
    } else {
      usage();
    }
  }

  // Echo source and dest to user:
  cout << "Loading basis set data from " << sourceUrl << "\n to "
       << destUrl << endl;

  if(!SDirectory(sourceUrl.c_str()).exists()) {
    string msg = sourceUrl + " does not exist!";
    EE_RT_ASSERT(false, EE_FATAL, msg);
  }

  // Make connection to remote DAV server:
  edsi = EDSIFactory::getEDSI(destUrl.c_str());

  if (!edsi) {
    cout << "Cannot make connection to " << destUrl.c_str() << endl;
    
  } else {
    
    // Remove the old directory and create a fresh one:
    cout << "Removing existing data from dav server" << endl;

    if (!edsi->removeResource()) {
      EE_RT_ASSERT(false, EE_WARNING, "Unable to delete gbs directory.");
    }
   
    cout << "Making new basis set directory on dav server.\n";
      
    string full_path = edsi->getURL().toString();
    string path_to_dir = full_path.substr(0,full_path.rfind('/'));
    string dir  = full_path.substr(full_path.rfind('/'),full_path.size());
    edsi->setURL(path_to_dir.c_str());
    vector<ResourceMetaDataResult> results;
    if(edsi->listCollection(results)) {
      edsi->makeCollection(dir);
      edsi->setURL(full_path.c_str());
    } else {
      string msg = path_to_dir + " does not exist!";
      EE_RT_ASSERT(false, EE_FATAL, msg);
    }

    if(doMeta) {
      cout << "Loading meta data for " << destUrl << endl;
      load_gbs_meta_data(edsi, sourceUrl, destUrl);
    }
    if(doData) {
      cout << "Loading gbs files into " << destUrl << endl;
      load_gbs_data(edsi, sourceUrl);
    }
  }
}

/**********************************************************************
 Method:  load_gbs_meta_data

 Summary: This method loads the properties set on the
          GaussianBasisSets directory.  Specifically, there is one
          property for each basis set type.  Each property holds the
	  mapping of basis sets (within that type category) to the
	  associated files and supported atoms.  For example, STO-3G*
	  would fall under the "pople" property.  It would contain the
	  files STO-3G.BAS and STO-3GS.BAS, as well as a list of the
	  supported atoms for each of the previous files.
***********************************************************************/
void load_gbs_meta_data(EDSI* edsi, string& gbs_dir, string& destUrl)
{
  // Use the Ecce namespace for DAV properties
  string ns = VDoc::getEcceNamespace() + ":";

  // NOTE: I am writing 1 property at a time, because it helps to
  // troubleshoot where errors occur.  This doesn't effect the
  // performance much, and this is not important for this conversion utility
  vector<MetaDataResult> collection_meta_data(1);
  MetaDataResult* result=0;

  // the mapping data is stored in the gbsNameConv file, which was created
  // by the gbsDAVConverter perl script
  ifstream metaFile((gbs_dir + "/gbsNameConv").c_str());
  char line[5000];

  while(metaFile) {

    metaFile.getline(line,5000);

    // Dav properties are stored based on the gbs type:
    if(!strncmp(line,"type= ",6)) { // new type

      if(result) { // add the previous type property to the vector, if exists
	cout << "adding property " << result->name << endl;
	edsi->setURL( (destUrl + "/" + result->name).c_str());

	if (!edsi->putDataSet(result->value.c_str()))
	  EE_RT_ASSERT(false, EE_WARNING, "putDataSet unsuccessful");

	delete result;
	result = 0;
      }
      // make a new property
      result = new MetaDataResult;
      result->name = &line[6];

    } else { // concat to end of existing property
      result->value += line;
      result->value += '\n';
    }
  }
  metaFile.close();
  if (result) {
    // Push back the last property
    cout << "adding property " << result->name << endl;
    edsi->setURL( (destUrl + "/" + result->name).c_str());

    if (!edsi->putDataSet(result->value.c_str()))
      EE_RT_ASSERT(false, EE_WARNING, "putMetaData unsuccessful");

    delete result;
  }
  
  // Now do the revision information as a separate file:
  edsi->setURL( (destUrl + "/RevisionHistory").c_str() );

  // Revision history comes from the bsb.rev file, also generated by
  // gbsDAVConverter:
  ifstream rev_mdata((gbs_dir + "/bsb.rev").c_str());

  /*
  while (rev_mdata) {
    rev_mdata.getline(line,5000);
    if (line[0] != '#') {
      collection_meta_data[0].value += filter(line);
      collection_meta_data[0].value += '\n';
    }
  } 
  */
  
  if (!edsi->putDataSet(rev_mdata))
    EE_RT_ASSERT(false, EE_WARNING, "putDataSet unsuccessful");

  edsi->setURL(destUrl.c_str());

}

/**********************************************************************
 Method:  load_gbs_data

 Summary: This method loads the individual .BAS and .POT files and the
          metadata properties associated with each file.
***********************************************************************/
void load_gbs_data(EDSI* edsi, string& gbs_dir)
{
  SDirectory dir(gbs_dir.c_str());
  string base = edsi->getURL().toString();
  string ns = VDoc::getEcceNamespace() + ":";
  char* cleanStr;

  EcceSortedVector<string, less<string> > files;
  dir.getFileNames(files);  // Don't put namespace here because it 
                            // is not used in TRevLog::display!

  int cnt = files.size();
  for(int idx=0; idx<cnt; idx++) {
    const char* file = files[idx].c_str();

    if(&file[strlen(file)-4] == strstr(file,".BAS") ||
       &file[strlen(file)-4] == strstr(file,".POT") ) {

      cout << "file : " << file << endl;
      edsi->setURL( string(base + "/" + file).c_str() );
  
      // put gbs data onto dav server
      ifstream gbs_file( string(gbs_dir + "/" + file).c_str() );
      ostrstream buf;
      ostrstream infoFile;
      gbs_file  >> buf.rdbuf();
      buf << ends;
      if (!edsi->putDataSet( buf.str() )) {
        string msg = "Write failed!  " + edsi->m_msgStack.getMessage();
        EE_RT_ASSERT(false, EE_WARNING, msg);
      }

      // parse gbs meta data
      MetaDataResult result;
      vector<MetaDataResult> meta_data;
      ifstream meta_file(string(gbs_dir + "/" + file + ".meta").c_str());
      char line[500];
      meta_file.getline(line,500);
      while(!meta_file.eof()) {

        if(0 == strcmp(line,"name")       ||
           0 == strcmp(line,"type")       ||
           0 == strcmp(line,"category")       ||
           0 == strcmp(line,"spherical")       ||
           0 == strcmp(line,"contraction_type")       ||
           0 == strcmp(line,"references") ||
           0 == strcmp(line,"info")       ||
           0 == strcmp(line,"comments")
          ) {
    
          result.name = ns + line;
          result.value = "";
          bool addReturn = false;
          char data[500];
          do {
            meta_file.getline(data,500);
            if(0 != strcmp(line,data)) {
              if(addReturn) result.value += "\n";
              addReturn = true;
              result.value += data;

            } else {
              break;
            }
          } while(1);
          
          // Write these 3 pieces of data to a separate file because
          // they are too big for a DAV property
          if (strcmp(line,"references") == 0 || strcmp(line, "info") == 0
              || strcmp(line, "comments") == 0) {
            infoFile << line << endl << result.value.c_str() << endl
                     << line << endl;

          } else {
            // get rid of special characters
            cleanStr = filter(result.value.c_str());
            result.value = cleanStr;
            delete cleanStr;       
            // add to the metadata vector
            meta_data.push_back(result);
          }
          meta_file.getline(line,500);
    
        } else {
          cerr << " Unhandled(" << file << ") : " << line << endl;
          meta_file.getline(line,500);
        }

      }
      // put the propery metadata
      if (!edsi->putMetaData(meta_data)) {
        string msg = "Write failed!  " + edsi->m_msgStack.getMessage();
        EE_RT_ASSERT(false, EE_WARNING, msg);
      }

      // put the file metadata (info, comments, references)
      infoFile << ends;
      edsi->setURL( string(base + "/" + file + ".meta").c_str() );
      if (!edsi->putDataSet( infoFile.str() )) {
        string msg = "Write failed!  " + edsi->m_msgStack.getMessage();
        EE_RT_ASSERT(false, EE_WARNING, msg);
      }
    }
  }
}

/**********************************************************************
 Method:  usage

 Summary: 
***********************************************************************/
void usage(void) {
  cout << "load_tgbs -src  <source directory> " << endl
       << "          -dest <destination directory on dav server>" << endl;
  exit(-1);
}


/**********************************************************************
 Method:  filter

 Summary: This function gets rid of characters that are offensive to
          an xml parser (namely < and > ).
***********************************************************************/
char* filter(const char* data) {
  ostrstream os;
  int cnt = strlen(data);
  for(int idx=0; idx<cnt; idx++) {
    switch(data[idx]) {
      case '&' : 
        os << "&amp;";
        break;
      case '<' :
        os << "&lt;";
        break;
      case '>' :
        os << "&gt;";
        break;
      case '"' :
        os << "&quot;";
        break;
      default :
        os << data[idx];
        break;
    }
  }
  os << '\0';
  
  return os.str(); // caller must free memory!
}

