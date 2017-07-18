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

#include "tdat/Fragment.H"
#include "tdat/MetaPotentialModel.H"
#include "tdat/GUIValues.H"
#include "tdat/TTheory.H"

#include "dsm/MetaPotentialModelXMLizer.H"
#include "dsm/QMMMModelXMLizer.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/VDoc.H"

#include "MetaEd.H"


string MetaEd::changeWD(const string& newDir)
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

bool MetaEd::write_cs(const string& output_file)
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


bool MetaEd::write_potentials(const string& output_file, bool& file_flag)
{
  // we only write_potentials if our runtype is Car-Parrinello , so bail if it isn't
  if (p_iCalc->runtype().name() != "Car-Parrinello") {
    // it's not an error to bail early if not the correct runtype
    return true;
  }

  bool status = false;
  file_flag = false;

  Fragment* frag = p_iCalc->fragment();
  if (frag != (Fragment*)0) {
    MetaPotentialModel gcon(*frag);
    p_iCalc->getMetaPotentialModel(gcon);

    if (gcon.size() > 0) {
      Serializer *ser = new MetaPotentialModelXMLizer();
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

bool MetaEd::write_qmmmterms(const string& output_file, bool& file_flag)
{
  // we only write_potentials if our runtype is Car-Parrinello , so bail if it isn't
  if (p_iCalc->runtype().name() != "Car-Parrinello") {
    // it's not an error to bail early if not the correct runtype
    return true;
  }

  bool status = false;
  file_flag = false;

  Fragment* frag = p_iCalc->fragment();
  if (frag != (Fragment*)0) {
    QMMMModel gcon(*frag);
    p_iCalc->getQMMMModel(gcon);

    if (gcon.size() > 0) {
      Serializer *ser = new QMMMModelXMLizer();
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


/**
 *  Write out the guidm options and also some extra stuff that we
 *  had no other place to put.
 */
bool MetaEd::write_setup(const string& output_file)
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

    GUIValues* guivalues = p_iCalc->guiparams();
    if (guivalues != (GUIValues*)0)
        guivalues->dumpKeyVals(os);

    os.close();

    status = true;
  }

  return status;
}

bool MetaEd::write_template(RCommand& localconn, const string& templateFile,
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

bool MetaEd::input_controller(const bool& saveParamFlag,
                              const bool& useExpCoeff, string& message)
{
  bool status = false;
  string realExpName;

  message = "";

  // Create the files needed by the perl scripts
  // We assume that both files (param, fragment) are needed

  SDirectory* dir = TempStorage::getTempJobDirectory(
                                 p_iCalc->getURL().getFilePathTail().c_str());

  string tempFileName = p_iCalc->getName();

  // Files that will be produced
  string frag_file = tempFileName + ".frag";
  string meta_file = tempFileName + ".meta";
  string qmmm_file = tempFileName + ".qmmm";
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

      bool qmmm_file_flag, meta_file_flag;

      if (!write_cs(frag_file))
        message = "Input files could not be generated--failed writing "
                  "chemical system";
      else if (!write_potentials(meta_file, meta_file_flag))
        message ="Input files could not be generated--failed writing "
                 "chemical system meta-potentials";
      else if (!write_qmmmterms(qmmm_file, qmmm_file_flag))
        message ="Input files could not be generated--failed writing "
                 "chemical system QMMM terms";
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
          // determine if this is a chained metadynamics task since we don't
          // include the geometry and qmmm blocks for those
          bool chained = false;
          ChemistryTask *inputTask = dynamic_cast<ChemistryTask*>(p_iCalc->getInputProvider(p_iCalc->getSession()));
          if (inputTask)
            chained = inputTask->getApplicationType() == ResourceDescriptor::AT_METADYNAMICS;

          parser += " -n " + tempFileName + " -p -f";
          if (meta_file_flag)
            parser += " -m";
          if (qmmm_file_flag)
            parser += " -q";
          if (chained)
            parser += " -c";
          parser += " > " + input_file;

          string warning; 
          if (!localconn.execout(parser, message))
            message = "Input files could not be generated--input parsing "
                      "command " + parser + " failed";
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

