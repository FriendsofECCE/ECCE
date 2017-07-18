#include <fstream>
  using std::endl;
  using std::ends;
  using std::ifstream;
  using std::ofstream;
#include <strstream>
  using std::ostrstream;
#include <unistd.h> // for getcwd()

#include "util/Ecce.H"
#include "util/EcceMap.H"
#include "util/SDirectory.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"
#include "util/STLUtil.H"

#include "tdat/ESPConstraintModel.H"
#include "tdat/Fragment.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"
#include "tdat/GUIValues.H"
#include "tdat/TTheory.H"

#include "dsm/TGBSConfig.H"
#include "dsm/ESPConstraintModelXMLizer.H"
#include "dsm/GeomConstraintModelXMLizer.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/VDoc.H"

#include "CalcEd.H"


string CalcEd::changeWD(const string& newDir)
{
  string oldPath;
  if (newDir != "") {
    // Change Working Directory
    char cwd[1025];
    getcwd(cwd, 1024);
    SDirectory workingDir(newDir);
    if (workingDir.exists() && !chdir(workingDir.path().c_str()))
      oldPath = cwd;
  }
  return oldPath;
}

bool CalcEd::write_cs(const string& output_file)
{
  bool status = false;

  Fragment* frag = p_iCalc->fragment();
  if (frag != (Fragment*)0) {
    ofstream os(output_file.c_str());
    if (os) {
      status = frag->dumpMVM(os);
      os.close();
    }
  }

  return status;
}

bool CalcEd::write_esp(const string& output_file, bool& file_flag)
{
  // we only write_esp if our runtype is ESP, so bail if it isn't
  if (p_iCalc->runtype().name() != "ESP") {
    // it's not an error to bail early if not the correct runtype
    return true;
  }

  bool status = false;
  file_flag = false;

  Fragment* frag = p_iCalc->fragment();
  if (frag != (Fragment*)0) {
    ESPConstraintModel esp(*frag);
    p_iCalc->getESPModel(esp);

    if (esp.size() > 0) {
      Serializer *ser = new ESPConstraintModelXMLizer();
      string data;
      EcceMap props;
      try {
        ser->serialize(esp, data, props);
        ofstream outFile(output_file.c_str());
        if (outFile) {
          outFile << data << endl;
          outFile.close();
          status = true;
          file_flag = true;
        }
      } catch (std::exception& ) {
        status = false;
      }
      delete ser;
    } else
      status = true;
  } else
    status = true;

  return status;
}

bool CalcEd::write_constraints(const string& output_file, bool& file_flag)
{
  // we only write_constraints if our runtype is geometry, so bail if it isn't
  if (p_iCalc->runtype().name() != "Geometry"
          && p_iCalc->runtype().name() != "GeoVib") {
    // it's not an error to bail early if not the correct runtype
    return true;
  }

  bool status = false;
  file_flag = false;

  Fragment* frag = p_iCalc->fragment();
  if (frag != (Fragment*)0) {
    GeomConstraintModel gcon(*frag, GeomConstraintRules());
    p_iCalc->getGeomConstraintModel(gcon);

    if (gcon.size() > 0) {
      Serializer *ser = new GeomConstraintModelXMLizer();
      string data;
      EcceMap props;
      try {
        ser->serialize(gcon, data, props);
        ofstream outFile(output_file.c_str());
        if (outFile) {
          outFile << data << endl;
          outFile.close();
          status = true;
          file_flag = true;
        }
      } catch (std::exception& ) {
        status = false;
      }
      delete ser;
    } else
      status = true;
  } else
    status = true;

  return status;
}

bool CalcEd::write_gbsconfig(const string& output_file,
        const bool& useExpCoeff, bool& file_flag, bool& ecp_flag,
        bool& spherical_flag)
{
  bool status = false;
  file_flag = false;
  ecp_flag = false;
  spherical_flag = false;

  const JCode *code = p_iCalc->application();
  if (code->theoryNeedsBasis(*(p_iCalc->theory())))
  {
    TGBSConfig* const gbsConfig = p_iCalc->gbsConfig();
    if (gbsConfig != (TGBSConfig*)0)
    {
      // Produce code formatted basis set file
      ostrstream stdStream;
      stdStream << gbsConfig->dump(code->name().c_str(), !useExpCoeff);
      stdStream << ends;
      char* charData = stdStream.str();

      ofstream outFile(output_file.c_str());
      if (outFile && charData!=NULL && charData[0]!='\0') {
        outFile << charData;
        outFile.close();
        status = true;
        file_flag = true;
      }

      ecp_flag = gbsConfig->hasECPs();
      spherical_flag = gbsConfig->coordsys()==TGaussianBasisSet::Spherical;
    }
  } else
    // OK not to create a basis file for semi-empirical
    status = true;

  return status;
}

/**
 *  Write out the guidm options and also some extra stuff that we
 *  had no other place to put.
 */
bool CalcEd::write_setup(const string& output_file)
{
  bool status = false;

  ofstream os(output_file.c_str());
  if (os) {
    // I grep'ed in the controller dir and all of these seem necessary.
    // Seems somewhat hokey.
    os << "title: " << p_iCalc->getName() << endl;

    // Retrieving the annotation is a bit of work--no direct accessor
    string annotation = p_iCalc->getName();
    Resource *resource = dynamic_cast<Resource*>(p_iCalc);
    if (resource != (Resource*)0) {
      string annoprop = resource->getProp(VDoc::getEcceNamespace() +
                                          ":annotation");

      // remove any whitespace from the annoation including newlines
      // this is necessary because the annotation editor in the Organizer
      // is rather casual about whitespace and we want to make sure we
      // get something valid for this field for the input file
      STLUtil::stripLeadingAndTrailingWhiteSpace(annoprop);

      // If you clear an annotation in the Organizer, it actually leaves
      // a newline character
      if (annoprop != "") {
        if (annoprop.size() > 80)
          annoprop.resize(80);
        annotation = annoprop;
      }
    }
    os << "annotation: " << annotation << endl;

    const JCode* codecap = p_iCalc->application();
    if (codecap != (JCode*)0) {
      TypedFile file = codecap->getCodeFile(JCode::PARSE_OUTPUT);
      os << "parseFile: " << file.name() << endl;
    }

    TTheory* theory = p_iCalc->theory();
    if (theory != (TTheory*)0) {
      os << "Category: " << theory->category() << endl;
      os << "Theory: " << theory->name() << endl;
    }

    os << "RunType: " << p_iCalc->runtype().name() << endl;

    Fragment* frag = p_iCalc->fragment();
    if (frag != (Fragment*)0) {
      os << "Charge: " << frag->charge() << endl;
      os << "Symmetry: " << frag->pointGroup() << endl;
      if (frag->useSymmetry()) {
        // Need to temporarily chdir back to bin directory to call genmol
        string binDir = Ecce::ecceHome();
        binDir += "/";
        binDir += getenv("ECCE_SYSDIR");
        binDir += "bin";
        string currDir = changeWD(binDir);
        frag->generateFullMolecule();
        changeWD(currDir);
      }
      os << "NumElectrons: " << frag->numElectrons() << endl;

      int numElectrons, spin, numFrozenOrbs;
      int numOccupiedOrbs, numVirtualOrbs, numNormalModes;
      getOrbitalParams(numElectrons, spin, numFrozenOrbs,
                       numOccupiedOrbs, numVirtualOrbs, numNormalModes);

      os << "NumFrozenOrbs: " << numFrozenOrbs << endl;
      os << "NumOccupiedOrbs: " << numOccupiedOrbs << endl;
      os << "NumVirtualOrbs: " << numVirtualOrbs << endl;
      os << "NumNormalModes: " << numNormalModes << endl;
    }

    os << "ChemSys.Multiplicity: " <<
               (int)p_iCalc->spinMultiplicity() << endl;

    if (codecap->theoryNeedsBasis(*theory)) {
      TGBSConfig* const gbsConfig = p_iCalc->gbsConfig();
      if (gbsConfig != (TGBSConfig*)0) {
        string coords = gbsConfig->coordsys()==TGaussianBasisSet::Cartesian?
                  "Cartesian": "Spherical";
        os << "BasisSet.Coordinates: " << coords << endl;
      }
    }

    GUIValues* guivalues = p_iCalc->guiparams();
    if (guivalues != (GUIValues*)0)
        guivalues->dumpKeyVals(os);

    os.close();

    status = true;
  }

  return status;
}

bool CalcEd::write_template(RCommand& localconn, const string& templateFile,
                            const string& outputFile)
{
    bool status = false;

    if (!templateFile.empty()) {
      string templatePathname(Ecce::ecceDataControllersPath());
      templatePathname += "/" + templateFile;
      if (localconn.exists(templatePathname)) {
        string command = "/bin/cp " + templatePathname + " " + outputFile;
        status = localconn.exec(command);
        if (status) {
          command = "chmod 666 " + outputFile;
          status = localconn.exec(command);
        }
      }
    }

  return status;
}

bool CalcEd::input_controller(const bool& saveParamFlag,
                              const bool& useExpCoeff, string& message)
{
  bool status = false;
  string realExpName;

  message = "";

  // Create the files needed by the perl scripts
  // We assume that all three files (param, basisset, fragment) are needed

  SDirectory* dir = TempStorage::getTempJobDirectory(
                                 p_iCalc->getURL().getFilePathTail().c_str());

  string tempFileName = p_iCalc->getName();

  // Files that will be produced
  string frag_file = tempFileName + ".frag";
  string esp_file = tempFileName + ".esp";
  string con_file = tempFileName + ".con";
  string basis_file = tempFileName + ".basis";
  string param_file = tempFileName + ".param";

  string lastDir = "";     // used to cache current working directory

  const JCode* codecap = p_iCalc->application();

  // create local RCommand connection to avoid overhead of system calls
  RCommand localconn("system");

  // We no longer assume the directory already exists.  Also go ahead
  // and check permissions for better msgs while we are at it.
  if (codecap == (JCode*)0)
    message="Input files could not be generated--code does not exist";
  else if (!localconn.isOpen())
    message = "Unable to create local command shell.";
  else if (!localconn.directory(dir->path()))
    message = "Unable to create temporary local calculation directory " 
              + dir->path() + ". Out of disk space or permissions failure?";
  else if (!localconn.writable(dir->path()))
    message = "Unable to write to the temporary local calculation directory " 
              + dir->path() + ". Out of disk space?";
  // change Directory to Experiment Location
  else if (!localconn.cd(dir->path()))
    message = "Input files could not be generated--temporary local directory "
              + dir->path() + " does not exist";
  else if ((lastDir = changeWD(dir->path())) == "")
    message = "Input files could not be generated--temporary local directory "
              + dir->path() + " does not exist";
  else {
    // Remove all input files from DAV to insure consistency

    if (!p_iCalc->removeInputFiles())
      message = "Input files could not be deleted "
                "from DAV before regeneration";
    else {
      TypedFile typedInFile = codecap->getCodeFile(JCode::PRIMARY_INPUT);
      string input_file = typedInFile.name();
      string orig_input_file = input_file + ".orig";
      string tmpstr;
      codecap->get_string("InputGenerator", tmpstr);
      string parser = tmpstr;
      codecap->get_string("Template", tmpstr);
      string templateFile = tmpstr;

      bool ecp_flag, spherical_flag;
      bool esp_file_flag, con_file_flag, basis_file_flag;

      if (!write_cs(frag_file))
        message = "Input files could not be generated--failed writing "
                  "chemical system";
      else if (!write_esp(esp_file, esp_file_flag))
        message ="Input files could not be generated--failed writing "
                 "ESP file";
      else if (!write_constraints(con_file, con_file_flag))
        message ="Input files could not be generated--failed writing "
                 "chemical system constraints";
      else if (!write_gbsconfig(basis_file, useExpCoeff,
                                basis_file_flag, ecp_flag, spherical_flag))
        message ="Input files could not be generated--failed writing "
                 "basis set";
      else if (!write_setup(param_file))
        message ="Input files could not be generated--failed writing "
                 "output file " + param_file;
      else if (!write_template(localconn, templateFile, orig_input_file)) {
        message ="Input files could not be generated--failed writing "
                 "code template (no template for ";
        message += codecap->name();
        message += "?)";
      } else if (parser == "") 
        message = "Input files could not be generated"
                  "--input parser not found";
      else {
        string parser_path(Ecce::ecceDataControllersPath());
        parser_path += "/" + parser;
        if (!localconn.exists(parser_path))
          message = "Input files could not be generated--parser file " +
                    parser_path + " not found";
        else {
          if (ecp_flag)
            parser += " -e";
          if (spherical_flag)
            parser += " -s";

          parser += " -n " + tempFileName + " -p -f";
          if (esp_file_flag)
            parser += " -q";
          if (con_file_flag)
            parser += " -c";
          if (basis_file_flag)
            parser += " -b";
          parser += " -t " + orig_input_file;

          string warning; 
          if (!localconn.execout(parser, message))
            message = "Input files could not be generated--input parsing "
                      "command " + parser + " failed";
          else {
            string pretty_cmd = "prettyInput <" + orig_input_file +
                                " >" + input_file;

            if (!localconn.exec(pretty_cmd))
              message = "Input files could not be generated--pretty parsing "
                        "command " + pretty_cmd + " failed";
            else {
              // DAV put (copy from local disk to DAV)

              // primary input file
              ifstream ifs(input_file.c_str());
              if (p_iCalc->putInputFile(input_file, &ifs))
                status = true;
              else
                message = "Input files could not be generated--input file "
                          "copy to DAV failed";
        
              ifs.close();

              // auxiliary basis set files (Amica)
              TypedFile auxInputFile =
                        codecap->getCodeFile(JCode::AUXILIARY_INPUT);
              if (auxInputFile.name() != "") {
                string lsbas;
                string lscmd = "ls -1 *" + auxInputFile.name();
                if (localconn.execout(lscmd, lsbas)) {
                  char* tok;
                  for (tok = strtok((char*)lsbas.c_str(), "\n");
                       tok != NULL;
                       tok = strtok(NULL, "\n")) {
                    // last character is a <cr> so delete it
                    tok[strlen(tok)-1] = '\0';
                    ifs.open(tok);
                    if (!p_iCalc->putInputFile(tok, &ifs)) {
                      status = false;
                      message = "Input files could not be generated--basis "
                                "file copy to DAV failed";
                      ifs.close();
                      break;
                    } 
                    ifs.close();
                  }
                }
              }
            } // pretty command succeeded
          } // parser command succeeded
        } // parser file exists
      } // parser exists
    } // Input file could be deleted from DAV
  } // UNIX directory exists, writable, and able to cd to it

  // Revert working directory before deleting temporary one because
  // you can't delete the current directory
  if (lastDir != "")
    changeWD(lastDir);

  if (!saveParamFlag)
    dir->remove();

  return status;
}

