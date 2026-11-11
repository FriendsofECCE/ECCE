#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "dsm/DeckWriter.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/Series.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TaskJob.H"
#include "tdat/Fragment.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"
#include "tdat/LatticeDef.H"
#include "tdat/SpinMult.H"
#include "tdat/TAtm.H"
#include "tdat/TTheory.H"
#include "util/EcceURL.H"

using std::string;
using std::vector;

namespace {

struct Structure {
  string name;
  Fragment *frag;        // charge set
  int mult;              // 0: lowest
  Structure() : frag(0), mult(0) {}
};

string baseName(const string& path)
{
  size_t s = path.find_last_of('/');
  string n = s == string::npos ? path : path.substr(s + 1);
  size_t dot = n.rfind('.');
  if (dot != string::npos && dot > 0) n = n.substr(0, dot);
  return n;
}

// Every key=value word of the comment line, case-insensitive.
bool word(const string& line, const string& key, int& value)
{
  std::istringstream in(line);
  string w;
  while (in >> w) {
    size_t eq = w.find('=');
    if (eq == string::npos) continue;
    string k = w.substr(0, eq);
    for (size_t i = 0; i < k.size(); i++) k[i] = tolower(k[i]);
    if (k == key) { value = atoi(w.c_str() + eq + 1); return true; }
  }
  return false;
}

Fragment *readXyz(const string& path, int defCharge, int& charge, int& mult,
                  string& why)
{
  std::ifstream in(path.c_str());
  if (!in) { why = "cannot read " + path; return 0; }
  string line;
  int n = 0;
  if (!std::getline(in, line) || (n = atoi(line.c_str())) < 1) {
    why = "not an xyz file"; return 0;
  }
  std::getline(in, line);
  charge = defCharge;
  word(line, "charge", charge);
  word(line, "mult", mult);
  Fragment *frag = new Fragment;
  //  Extended xyz: a Lattice="..." entry (three row vectors) makes the
  //  structure periodic, as ecce-sites writes it for a site screen (#728).
  string::size_type lat = line.find("Lattice=\"");
  if (lat != string::npos) {
    std::istringstream vs(line.substr(lat + 9, line.find('"', lat + 9) - lat - 9));
    double v[9];
    int got = 0;
    while (got < 9 && vs >> v[got]) got++;
    if (got != 9) { why = "Lattice= needs 9 numbers"; delete frag; return 0; }
    LatticeDef *cell = new LatticeDef(1, 1, 1, 90, 90, 90);
    cell->assign(MPoint(v[0], v[1], v[2]), MPoint(v[3], v[4], v[5]), MPoint(v[6], v[7], v[8]));
    frag->setLattice(cell);
  }
  for (int i = 0; i < n; i++) {
    string sym;
    double xyz[3];
    if (!std::getline(in, line)) { why = "xyz file ends early"; delete frag; return 0; }
    std::istringstream ls(line);
    if (!(ls >> sym >> xyz[0] >> xyz[1] >> xyz[2])) {
      why = "bad atom line: " + line; delete frag; return 0;
    }
    if (isdigit((unsigned char)sym[0])) {
      why = "atomic numbers are not supported: use element symbols"; delete frag; return 0;
    }
    frag->addAtom(sym, xyz);
  }
  frag->addCovalentBonds();
  return frag;
}

}  // namespace


vector<Series::Outcome> Series::make(const EcceURL& templ, const EcceURL& folder,
                                     const vector<string>& sources,
                                     int defaultCharge, int defaultMult)
{
  vector<Outcome> results;
  Resource *tres = EDSIFactory::getResource(templ);
  ICalculation *tcalc = dynamic_cast<ICalculation*>(tres);
  if (!tcalc) {
    Outcome o; o.note = "the template is not a calculation";
    results.push_back(o);
    return results;
  }
  string basisName;
  if (TGBSConfig *cfg = tcalc->gbsConfig()) basisName = cfg->name();
  Fragment *tfrag = tcalc->fragment();
  const string templateTags = tfrag ? tfrag->uniqueTagStr() : string();
  // Constraints name atom indices of the template's structure.
  int templateConstraints = 0;
  if (tfrag) {
    GeomConstraintModel gcon(*tfrag, GeomConstraintRules());
    if (ChemistryTask *tt = dynamic_cast<ChemistryTask*>(tres)) {
      tt->getGeomConstraintModel(gcon);
      templateConstraints = gcon.size();
    }
  }
  delete tfrag;

  for (size_t i = 0; i < sources.size(); i++) {
    Outcome o;
    o.name = baseName(sources[i]);
    string why;
    int charge = defaultCharge, mult = defaultMult;
    Fragment *frag = 0;
    const bool isXyz = sources[i].size() > 4 &&
        sources[i].compare(sources[i].size() - 4, 4, ".xyz") == 0;
    if (isXyz) {
      frag = readXyz(sources[i], defaultCharge, charge, mult, why);
    } else {
      Resource *sres = EDSIFactory::getResource(EcceURL(sources[i]));
      PropertyTask *stask = dynamic_cast<PropertyTask*>(sres);
      ICalculation *scalc = dynamic_cast<ICalculation*>(sres);
      if (!stask || !scalc) why = "not a calculation";
      else if ((frag = stask->fragment()) == 0) why = "the calculation has no structure";
      else {
        // The final geometry of a run; a calculation not run keeps its own.
        stask->getFragmentStep(frag, -1);
        charge = frag->charge();
        mult = (int)scalc->spinMultiplicity();
        o.name = sres->getName();
      }
    }
    if (!frag) { o.note = why; results.push_back(o); continue; }
    frag->charge(charge);

    const int electrons = frag->nuclearCharge() - charge;
    if (mult == 0) mult = (electrons % 2 == 0) ? 1 : 2;
    if (electrons < 0 || mult < 1 || mult > 9 || (electrons - (mult - 1)) % 2 != 0 ||
        mult - 1 > electrons) {
      char buf[160];
      snprintf(buf, sizeof buf, "multiplicity %d does not fit %d electrons "
               "(charge %d)", mult, electrons, charge);
      o.note = buf;
      delete frag;
      results.push_back(o);
      continue;
    }

    EcceURL target = folder.getChild(o.name);
    if (EDSIFactory::getResource(target)) {
      o.note = "a calculation of that name exists in the folder";
      delete frag;
      results.push_back(o);
      continue;
    }
    if (!tres->copy(target)) {
      o.note = "copy failed: " + tres->messages();
      delete frag;
      results.push_back(o);
      continue;
    }
    Resource *cres = EDSIFactory::getResource(target);
    TaskJob *job = dynamic_cast<TaskJob*>(cres);
    ICalculation *calc = dynamic_cast<ICalculation*>(cres);
    ChemistryTask *ctask = dynamic_cast<ChemistryTask*>(cres);
    bool good = job && calc && ctask;
    if (good) {
      job->resetForRerun(ResourceDescriptor::STATE_CREATED);
      // Remove the copied constraints; one stored without a configuration
      // entry is not found by name, so delete its file if it is still read.
      ctask->setGeomConstraintModel(0);
      if (Fragment *f0 = ctask->fragment()) {
        GeomConstraintModel left(*f0, GeomConstraintRules());
        ctask->getGeomConstraintModel(left);
        delete f0;
        if (left.size() > 0)
          if (Resource *file = EDSIFactory::getResource(
                  target.getChild("Parameters").getChild("GeomConstraintModel.gcst")))
            file->remove();
      }
      good = ctask->fragment(frag) &&
             calc->spinMultiplicity((SpinMult::SpinMultEnum)mult) &&
             calc->openShells(mult - 1);
      if (!good) o.note = "the structure could not be stored";
    } else o.note = "the copy is not a calculation";
    if (good && !basisName.empty() && frag->uniqueTagStr() != templateTags) {
      if (!DeckWriter::rebuildBasis(calc, basisName, why)) {
        o.note = why;
        good = false;
      }
    }
    if (good) {
      DeckWriter::Result r = DeckWriter::generate(calc);
      if (r.ok) job->setState(ResourceDescriptor::STATE_READY);
      else o.note = r.message;
      good = r.ok;
    }
    // The template's restricted theory is kept as it is; say so, since the
    // code will refuse or misread an open-shell structure under it.
    if (good && mult > 1) {
      TTheory *th = calc->theory();
      const string tn = th ? th->name() : string();
      if (tn.compare(0, 1, "R") == 0 && tn.compare(0, 2, "RO") != 0)
        o.note = "warning: the template's theory " + tn +
                 " is restricted and the structure has multiplicity " +
                 std::to_string(mult) + "; use an unrestricted template";
    }
    if (good && templateConstraints > 0) {
      if (!o.note.empty()) o.note += "; ";
      o.note += "the template's " + std::to_string(templateConstraints) +
                " geometry constraint(s) were not carried";
    }
    delete frag;
    o.ok = good;
    if (good) o.url = target.toString();
    else if (cres) cres->remove();
    results.push_back(o);
  }
  return results;
}
