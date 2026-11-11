#include <memory>
#include <unistd.h>

#include <sys/stat.h>

#include <algorithm>
#include <cstdio>
#include <fstream>

#include "comm/RCommand.H"
#include "dsm/DeckWriter.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ESPConstraintModelXMLizer.H"
#include "dsm/GBSRules.H"
#include "dsm/GeomConstraintModelXMLizer.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/QstStructures.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"
#include "tdat/ESPConstraintModel.H"
#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"
#include "tdat/TTheory.H"
#include "util/Ecce.H"
#include "util/EcceMap.H"
#include "util/Filesystem.H"
#include "util/SDirectory.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"

using std::string;
using std::ofstream;
using std::endl;

namespace {

// genmol and the generators are found relative to the working directory.
string changeWD(const string& newDir)
{
  string oldPath;
  if (newDir != "") {
    char cwd[1025];
    if (!getcwd(cwd, 1024)) cwd[0] = '\0';
    SDirectory workingDir(newDir);
    if (workingDir.exists() && !chdir(workingDir.path().c_str()))
      oldPath = cwd;
  }
  return oldPath;
}

// File work is done here rather than through shell commands: every process
// costs a noticeable fraction of a second on Windows (#940).
bool copyTemplate(const string& from, const string& to)
{
  std::ifstream in(from.c_str(), std::ios::binary);
  std::ofstream out(to.c_str(), std::ios::binary | std::ios::trunc);
  if (!in || !out) return false;
  out << in.rdbuf();
  out.close();
  if (!out) return false;
#ifndef _WIN32
  chmod(to.c_str(), 0666);   // the generator rewrites it in place
#endif
  return true;
}

// What scripts/parsers/prettyInput does: a run of blank lines becomes the
// first of them.  Text mode on both sides, as Perl's own I/O on Windows.
bool prettyCopy(const string& from, const string& to)
{
  std::ifstream in(from.c_str());
  std::ofstream out(to.c_str(), std::ios::trunc);
  if (!in || !out) return false;
  string line;
  bool lastBlank = false;
  while (std::getline(in, line)) {
    const bool blank =
        line.find_first_not_of(" \t\r\n\f\v") == string::npos;
    if (!blank || !lastBlank) {
      out << line;
      if (!in.eof()) out << '\n';
    }
    lastBlank = blank;
  }
  out.close();
  return !in.bad() && !out.fail();
}

// The non-hidden files in the working directory whose names end in suffix,
// as `ls -1 *suffix` lists them.
std::vector<string> filesEndingIn(const string& suffix)
{
  std::vector<string> names;
  std::error_code ec;
  for (const auto& e : ecce::fs::directory_iterator(".", ec)) {
    const string n = e.path().filename().u8string();   // UTF-8 on MSW too
    if (n.size() >= suffix.size() && n[0] != '.' &&
        n.compare(n.size() - suffix.size(), suffix.size(), suffix) == 0 &&
        e.is_regular_file(ec))
      names.push_back(n);
  }
  std::sort(names.begin(), names.end());
  return names;
}

unsigned long coreElectrons(unsigned long atomicNumber)
{
  static unsigned long electrons[] = {0, 2, 10, 18, 36, 54, 86, 999};
  int ie;
  for (ie = 1; atomicNumber > electrons[ie]; ie++);
  return electrons[ie-1];
}

// The same Fragment the editor expands: a copy, so the stored one is kept.
Fragment *fullFragment(const Fragment *frag, string& warning)
{
  Fragment *full = new Fragment(*frag);
  if (frag->useSymmetry()) {
    string binDir = string(Ecce::ecceHome()) + "/";
    const char *sysdir = getenv("ECCE_SYSDIR");
    if (sysdir) binDir += sysdir;
    binDir += "bin";
    string currDir = changeWD(binDir);
    if (!full->generateFullMolecule())
      warning = "The full molecule could not be generated from its "
                "symmetry-unique atoms (point group " + full->pointGroup() +
                "); see the terminal for why.";
    changeWD(currDir);
  }
  return full;
}

bool writeCS(ICalculation *calc, const string& file)
{
  bool status = false;
  Fragment *frag = calc->fragment();
  if (frag) {
    ofstream os(file.c_str());
    if (os) status = frag->dumpMVM(os);
    delete frag;
  }
  return status;
}

template <class Model>
bool writeXml(Serializer *ser, Model& model, const string& file, bool& flag)
{
  bool status = false;
  string data;
  EcceMap props;
  try {
    ser->serialize(model, data, props);
    ofstream out(file.c_str());
    if (out) {
      out << data << endl;
      status = true;
      flag = true;
    }
  } catch (std::exception&) {
    status = false;
  }
  delete ser;
  return status;
}

bool writeESP(ICalculation *calc, const string& file, bool& flag)
{
  if (calc->runtype().name() != "ESP") return true;
  flag = false;
  Fragment *frag = calc->fragment();
  bool status = true;
  if (frag) {
    ESPConstraintModel esp(*frag);
    calc->getESPModel(esp);
    if (esp.size() > 0)
      status = writeXml(new ESPConstraintModelXMLizer(), esp, file, flag);
    delete frag;
  }
  return status;
}

bool writeConstraints(ICalculation *calc, const string& code,
                      const string& file, bool& flag)
{
  if (GeomConstraintModel::useBy(code, calc->runtype().name()) ==
      GeomConstraintModel::IGNORED)
    return true;
  flag = false;
  Fragment *frag = calc->fragment();
  bool status = true;
  if (frag) {
    GeomConstraintModel gcon(*frag, GeomConstraintRules());
    calc->getGeomConstraintModel(gcon);
    if (gcon.size() > 0)
      status = writeXml(new GeomConstraintModelXMLizer(), gcon, file, flag);
    delete frag;
  }
  return status;
}

bool writeGbs(ICalculation *calc, const string& file, bool useExpCoeff,
              bool& fileFlag, bool& ecp, bool& spherical)
{
  bool status = false;
  fileFlag = ecp = spherical = false;
  const JCode *code = calc->application();
  // A step nobody has opened in the editor may store no theory (#950).
  std::unique_ptr<TTheory> theory(calc->theory());
  if (!code || !theory) return false;
  if (code->theoryNeedsBasis(*theory)) {
    TGBSConfig *const cfg = calc->gbsConfig();
    if (cfg) {
      // dump() returns null when the translator fails.
      const char *data = cfg->dump(code->name().c_str(), !useExpCoeff);
      ofstream out(file.c_str());
      if (out && data != NULL && data[0] != '\0') {
        out << data;
        status = true;
        fileFlag = true;
      }
      ecp = cfg->hasECPs();
      spherical = cfg->coordsys() == TGaussianBasisSet::Spherical;
    }
  } else
    status = true;   // semi-empirical: no basis file
  return status;
}

bool writeParams(ICalculation *calc, const string& file,
                 const DeckWriter::Options& opt, string& fullWarning)
{
  ofstream os(file.c_str());
  if (!os) return false;
  os << "title: " << calc->getName() << endl;

  // No whitespace or newlines, and at most 80 characters, in the annotation.
  string annotation = calc->getName();
  Resource *resource = dynamic_cast<Resource*>(calc);
  if (resource) {
    string annoprop = resource->getProp(VDoc::getEcceNamespace() + ":annotation");
    STLUtil::stripLeadingAndTrailingWhiteSpace(annoprop);
    if (annoprop != "") {
      if (annoprop.size() > 80) annoprop.resize(80);
      annotation = annoprop;
    }
  }
  os << "annotation: " << annotation << endl;

  const JCode *codecap = calc->application();
  if (codecap) {
    TypedFile f = codecap->getCodeFile(JCode::PARSE_OUTPUT);
    os << "parseFile: " << f.name() << endl;
  }
  TTheory *theory = calc->theory();
  if (theory) {
    os << "Category: " << theory->category() << endl;
    os << "Theory: " << theory->name() << endl;
  }
  os << "RunType: " << calc->runtype().name() << endl;

  Fragment *frag = calc->fragment();
  if (frag) {
    os << "Charge: " << frag->charge() << endl;
    os << "Symmetry: " << frag->pointGroup() << endl;
    Fragment *full = frag;
    if (frag->useSymmetry()) {
      // The editor expands this one in place before NumElectrons.
      string w;
      full = fullFragment(frag, w);
      if (!w.empty()) fullWarning = w;
    }
    os << "NumElectrons: " << full->numElectrons() << endl;

    // Orbital counts, as the editor's getOrbitalParams: the full molecule's
    // electrons, the basis from the stored configuration.
    const int spin = (int)calc->spinMultiplicity();
    const int electrons = full->nuclearCharge() - full->charge();
    long core = 0;
    for (size_t i = 0; i < frag->numAtoms(); i++)
      core += coreElectrons(frag->atomRef(i)->atomicNumber());
    int openShells = 0;
    const string tn = theory ? theory->name() : string();
    if (tn.find("RO") == 0 || tn.find('U') == 0)
      openShells = opt.openShells >= 0 ? opt.openShells
                                       : (int)calc->openShells();
    const int occupied = (electrons - openShells) / 2;
    int frozen = -1, virt = -1;
    TGBSConfig *basis = calc->gbsConfig();
    if (basis) {
      TagCountMap *tcmap = full->tagCountsSTL();
      frozen = (core - basis->num_ecpCoreElectrons(*tcmap)) / 2;
      virt = basis->num_functions(*tcmap) - occupied - openShells;
    }
    (void)spin;
    os << "NumFrozenOrbs: " << frozen << endl;
    os << "NumOccupiedOrbs: " << occupied << endl;
    os << "NumVirtualOrbs: " << virt << endl;
    os << "NumNormalModes: " << 3 * frag->numAtoms() << endl;
    if (full != frag) delete full;
    delete frag;
  }

  os << "ChemSys.Multiplicity: " << (int)calc->spinMultiplicity() << endl;

  if (codecap && theory && codecap->theoryNeedsBasis(*theory)) {
    TGBSConfig *const cfg = calc->gbsConfig();
    if (cfg) {
      string coords = cfg->coordsys() == TGaussianBasisSet::Cartesian
                          ? "Cartesian" : "Spherical";
      os << "BasisSet.Coordinates: " << coords << endl;
    }
  }

  GUIValues *gv = calc->guiparams();
  if (gv) gv->dumpKeyVals(os);

  TaskJob *job = dynamic_cast<TaskJob*>(calc);
  const string guess = job ? job->attachedOrbitalGuessFile() : string();
  if (!guess.empty())
    os << "ES.Theory.SCF.OrbitalGuessFile: " << guess << endl;
  if (job && job->hasPointChargeInput())
    os << "ES.Theory.SCF.PointChargeFile: " << job->pointChargeInputFile() << endl;
  if (job && job->hasHessianInput()) {
    os << "ES.Runtype.GeomOpt.HessianFile: " << job->hessianInputFile() << endl;
    // A workflow step made with "Start from the source's Hessian" reads it
    // unless the Initial Hessian was set to something else; the last line
    // of a key wins.
    GUIValue *init = gv ? gv->get("ES.Runtype.GeomOpt.InitialHessian") : 0;
    const string chosen = init ? init->getValueAsString() : "";
    if (!job->getProp(TaskJob::hessianFromSourceProp()).empty() &&
        (chosen.empty() || chosen == "Default"))
      os << "ES.Runtype.GeomOpt.InitialHessian: "
            "Read from previous calculation" << endl;
  }
  return true;
}

}  // namespace


DeckWriter::Result DeckWriter::generate(ICalculation *calc, const Options& opt)
{
  // The basis translators (std2<code>) are run by name; only the ecce-<app>
  // wrappers of the GUI put them on PATH.
  {
    const string dir = Ecce::ecceDataControllersPath();
    const char *old = getenv("PATH");
    const string path = old ? old : "";
#ifdef _WIN32
    const string sep = ";";   // a ':' would glue dir onto the first entry
#else
    const string sep = ":";
#endif
    if (!dir.empty() &&
        (sep + path + sep).find(sep + dir + sep) == string::npos)
      setenv("PATH", (dir + (path.empty() ? "" : sep) + path).c_str(), 1);
  }
  Result r;
  string &message = r.message;

  SDirectory *dir = TempStorage::getTempJobDirectory(
                        calc->getURL().getFilePathTail().c_str());
  const string base = calc->getName();
  const string fragFile = base + ".frag", espFile = base + ".esp",
               conFile = base + ".con", basisFile = base + ".basis",
               paramFile = base + ".param";
  string lastDir;
  const JCode *codecap = calc->application();
  RCommand localconn("system");

  std::unique_ptr<TTheory> theory(calc->theory());
  if (codecap == (JCode*)0)
    message = "Input files could not be generated--code does not exist";
  else if (!theory)
    message = "Input files could not be generated--no theory yet";
  else if (!localconn.isOpen())
    message = "Unable to create local command shell.";
  else if (!SFile::is_dir(dir->path()))
    message = "Unable to create temporary local calculation directory " +
              dir->path() + ". Out of disk space or permissions failure?";
  else if (!SFile(dir->path()).is_writable())
    message = "Unable to write to the temporary local calculation directory " +
              dir->path() + ". Out of disk space?";
  // The generator inherits this working directory: localconn has no cd.
  else if ((lastDir = changeWD(dir->path())) == "")
    message = "Input files could not be generated--temporary local directory " +
              dir->path() + " does not exist";
  else {
    TypedFile typedInFile = codecap->getCodeFile(JCode::PRIMARY_INPUT);
    const string inputFile = typedInFile.name();
    const string origInput = inputFile + ".orig";
    string parser, templateFile;
    codecap->get_string("InputGenerator", parser);
    codecap->get_string("Template", templateFile);

    bool ecp = false, spherical = false, espFlag = false, conFlag = false,
         basisFlag = false;

    // QST/NEB structure files are written first, as the editor always did.
    for (char k = '1'; k <= '3'; k++)
      remove((base + ".qst" + k + ".frag").c_str());
    remove("neb_product.xyz");
    remove("neb_guess.xyz");
    bool qstOk = true;
    {
      GUIValues *g = calc->guiparams();
      const string rt = calc->runtype().name();
      const bool neb = rt.compare(0, 25, "Transition State (NEB-TS)") == 0;
      if (g && (neb || rt.compare(0, 22, "Transition State (QST)") == 0)) {
        string status, warning;
        qstOk = neb ? nebWriteStructureFiles(calc, status, warning,
                                             &r.qstDetail, &r.qstTable)
                    : qstWriteStructureFiles(calc, base, status, warning,
                                             &r.qstDetail, &r.qstTable);
        r.qstStatus = status;
        if (!warning.empty())
          r.warnings.push_back(std::make_pair(string(neb ? "NEB-TS" : "Opt"),
                                              warning));
      }
    }

    if (opt.stage) opt.stage("Writing the structure and basis set");
    if (!writeCS(calc, fragFile))
      message = "Input files could not be generated--failed writing "
                "chemical system";
    else if (!qstOk)
      message = "Input files could not be generated--failed writing "
                "the QST structures";
    else if (!writeESP(calc, espFile, espFlag))
      message = "Input files could not be generated--failed writing ESP file";
    else if (!writeConstraints(calc, codecap->name(), conFile, conFlag))
      message = "Input files could not be generated--failed writing "
                "chemical system constraints";
    else if (!writeGbs(calc, basisFile, opt.useExpCoeff, basisFlag, ecp,
                       spherical))
      message = "Input files could not be generated--failed writing "
                "basis set";
    else if (!writeParams(calc, paramFile, opt, r.fullMoleculeWarning))
      message = "Input files could not be generated--failed writing "
                "output file " + paramFile;
    else {
      bool haveTemplate = false;
      if (!templateFile.empty()) {
        string tp = string(Ecce::ecceDataControllersPath()) + "/" + templateFile;
        haveTemplate = SFile(tp).exists() && copyTemplate(tp, origInput);
      }
      if (!haveTemplate) {
        message = "Input files could not be generated--failed writing "
                  "code template (no template for ";
        message += codecap->name();
        message += "?)";
      } else if (parser == "")
        message = "Input files could not be generated--input parser not found";
      else {
        string parserPath = string(Ecce::ecceDataControllersPath()) + "/" + parser;
        if (!SFile(parserPath).exists())
          message = "Input files could not be generated--parser file " +
                    parserPath + " not found";
        else {
          // By full path: only the Linux wrappers put scripts/parsers on PATH.
          parserPath = RCommand::quotePath(parserPath);
          if (ecp) parserPath += " -e";
          if (spherical) parserPath += " -s";
          parserPath += " -n " + RCommand::quotePath(base) + " -p -f";
          if (espFlag) parserPath += " -q";
          if (conFlag) parserPath += " -c";
          if (basisFlag) parserPath += " -b";
          parserPath += " -t " + RCommand::quotePath(origInput);

          // The generator's own output says what was wrong, so it leads.
          if (opt.stage) opt.stage("Running the " + codecap->name() +
                                   " input generator");
          string out;
          if (!localconn.execout(parserPath, out)) {
            message = "Input files could not be generated.";
            if (!out.empty()) message += "\n\n" + out;
            message += "\n(command: " + parserPath + ")";
          } else {
            // Form: ai.<code> warning [<anchor>]: <text>
            size_t pos = 0;
            while (pos < out.size()) {
              size_t eol = out.find('\n', pos);
              if (eol == string::npos) eol = out.size();
              string line = out.substr(pos, eol - pos);
              pos = eol + 1;
              size_t open = line.find(" warning [");
              size_t close = line.find("]: ", open == string::npos ? 0 : open);
              if (line.compare(0, 3, "ai.") == 0 && open != string::npos &&
                  close != string::npos) {
                size_t a = open + 10;
                size_t end = line.find_last_not_of(" \t\r");
                r.warnings.push_back(std::make_pair(
                    line.substr(a, close - a),
                    line.substr(close + 3, end + 1 - (close + 3))));
              }
            }
            if (opt.stage) opt.stage("Storing the input file");
            if (!prettyCopy(origInput, inputFile))
              message = "Input files could not be generated--could not "
                        "write " + inputFile;
            // Only now that there is a new deck: a failed generation leaves
            // the last good one for Final Edit.
            else if (!calc->removeInputFiles())
              message = "Input files could not be deleted from DAV before "
                        "regeneration";
            else {
              std::ifstream ifs(inputFile.c_str());
              if (calc->putInputFile(inputFile, &ifs)) r.ok = true;
              else message = "Input files could not be generated--input file "
                             "copy to DAV failed";
              ifs.close();

              // NEB-TS product / guess: inputs of the calculation.
              const char *neb[2] = {"neb_product.xyz", "neb_guess.xyz"};
              for (int k = 0; k < 2 && r.ok; k++) {
                std::ifstream in(neb[k]);
                if (in && !calc->putInputFile(neb[k], &in)) {
                  r.ok = false;
                  message = string("Input files could not be generated--") +
                            neb[k] + " copy to DAV failed";
                }
              }

              // auxiliary basis set files (Amica)
              TypedFile aux = codecap->getCodeFile(JCode::AUXILIARY_INPUT);
              if (aux.name() != "") {
                for (const string& name : filesEndingIn(aux.name())) {
                  std::ifstream in(name.c_str());
                  if (!calc->putInputFile(name, &in)) {
                    r.ok = false;
                    message = "Input files could not be generated--basis "
                              "file copy to DAV failed";
                    break;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Back out before deleting: the current directory cannot be removed.
  if (lastDir != "") changeWD(lastDir);
  if (!opt.keepFiles) dir->remove();
  else r.keptDir = dir->path();
  return r;
}


bool DeckWriter::rebuildBasis(ICalculation *calc, const string& name,
                              string& why)
{
  Fragment *frag = calc->fragment();
  const JCode *code = calc->application();
  if (!frag || !code) { why = "no structure or code"; delete frag; return false; }
  EDSIServerCentral central;
  EDSIGaussianBasisSetLibrary lib(central.getDefaultBasisSetLibrary());
  TGBSConfig *cfg = lib.simpleLookup(name.c_str(), frag->uniqueTagStr().c_str());
  delete frag;
  if (!cfg) { why = "basis set " + name + " has no functions for this structure"; return false; }
  TTheory *theory = calc->theory();
  GBSRules::autoOptimize(cfg, code, theory);
  if (!calc->gbsConfig(cfg)) { why = "the basis set could not be stored"; delete cfg; return false; }
  return true;
}
