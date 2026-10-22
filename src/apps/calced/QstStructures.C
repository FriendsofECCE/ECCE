#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/ResourceDescriptor.H"
#include "tdat/AtomMap.H"
#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "tdat/TAtm.H"
#include "util/EcceURL.H"
#include "util/TempStorage.H"
#include "util/SFile.H"

#include "QstStructures.H"

bool qstFinalStructure(PropertyTask* calc, std::vector<std::string>& el,
                       std::vector<double>& xyz, bool& optimised)
{
   el.clear();
   xyz.clear();
   optimised = false;
   if (!calc) return false;
   Fragment* frag = calc->fragment();
   if (!frag) return false;
   optimised = calc->getFragmentStep(frag, -1);
   for (size_t i = 0; i < frag->numAtoms(); i++) {
      const TAtm* a = frag->atomRef((int)i);
      el.push_back(a->atomicSymbol());
      const double* c = a->coordinates();
      xyz.push_back(c[0]); xyz.push_back(c[1]); xyz.push_back(c[2]);
   }
   delete frag;
   return !el.empty();
}

Fragment* qstReactantFragment(const std::string& url)
{
   PropertyTask* t = EDSIFactory::getPropertyTask(EcceURL(url.c_str()));
   Fragment* frag = t ? t->fragment() : 0;
   if (!frag) return 0;
   t->getFragmentStep(frag, -1);
   return frag;
}

std::vector<QstEntry> qstCandidates(ICalculation* self)
{
   std::vector<QstEntry> out;
   Resource* parent = EDSIFactory::getResource(self->getURL().getParent());
   if (!parent) return out;
   std::vector<Resource*>* kids = parent->getChildren();
   if (!kids) return out;
   for (size_t i = 0; i < kids->size(); i++) {
      ICalculation* c = dynamic_cast<ICalculation*>((*kids)[i]);
      if (!c || c->getURL().toString() == self->getURL().toString()) continue;
      std::vector<std::string> el;
      std::vector<double> xyz;
      bool opt;
      if (!qstFinalStructure(c, el, xyz, opt)) continue;
      QstEntry e;
      e.name = c->getName();
      e.url = c->getURL().toString();
      e.kind = opt ? "optimised" : "input";
      out.push_back(e);
   }
   return out;
}

std::string qstWriteCandidateFile(const std::vector<QstEntry>& entries)
{
   SFile* f = TempStorage::getTempFile();
   if (!f) return "";
   const std::string path = f->path();
   delete f;
   std::ofstream os(path.c_str());
   if (!os) return "";
   for (size_t i = 0; i < entries.size(); i++)
      os << entries[i].name << '\t' << entries[i].url << '\t'
         << entries[i].kind << '\n';
   return path;
}

/** One block of the "Show mapping" text: reactant atom -> the number the
 *  atom has in this structure's own order, far movers flagged. */
static void addTable(std::string& out, const char* role,
                     const std::vector<std::string>& refEl,
                     const AtomMapResult& r)
{
   out += std::string(role) + ": reactant atom -> atom number in the " + role +
          "\n";
   for (size_t j = 0; j < refEl.size(); j++) {
      int from = -1;
      for (size_t i = 0; i < r.map.size(); i++)
         if (r.map[i] == (int)j) from = (int)i;
      char buf[96];
      sprintf(buf, "  %s%zu -> %d", refEl[j].c_str(), j + 1, from + 1);
      out += buf;
      for (size_t i = 0; i < r.far.size(); i++)
         if (r.far[i] == from) out += "   (moves more than 1 A)";
      out += '\n';
   }
}

static bool writeFrag(const std::string& file, const std::string& title,
                      const std::vector<std::string>& el,
                      const std::vector<double>& xyz)
{
   std::ofstream os(file.c_str());
   if (!os) return false;
   os.setf(std::ios::fixed);
   os.precision(8);
   os << "ecce\nnum_atoms: " << el.size() << "\ntitle: " << title
      << "\natom_info: symbol x y z\natom_list:\n";
   for (size_t i = 0; i < el.size(); i++)
      os << el[i] << ' ' << xyz[3*i] << ' ' << xyz[3*i+1] << ' '
         << xyz[3*i+2] << '\n';
   return (bool)os;
}

static std::string value(GUIValues* g, const char* key)
{
   GUIValue* v = g ? g->get(key) : 0;
   return v ? v->getValueAsString() : "";
}

/** Renumbers and aligns (els, xyz) onto the reference structure.  false,
 *  with warning set, when the atoms do not match. */
static bool mapOnto(const std::vector<std::string>& refEl,
                    const std::vector<double>& ref,
                    std::vector<std::string>& els, std::vector<double>& xyz,
                    const char* role, const char* runtype,
                    AtomMapResult& r, std::string& warning)
{
   r = AtomMap::map(refEl, ref, els, xyz);
   if (!r.ok) {
      warning = std::string(runtype) + ": " + role + ": " + r.error +
                " The input is not complete.";
      return false;
   }
   AtomMap::apply(r.alignment, xyz);
   xyz = AtomMap::renumber(xyz, r.map);
   els = AtomMap::renumber(els, r.map);
   return true;
}

bool qstWriteStructureFiles(ICalculation* self, const std::string& base,
                            std::string& status, std::string& warning,
                            std::string* detail, std::string* table)
{
   status.clear();
   if (table) table->clear();
   warning.clear();
   GUIValues* g = self->guiparams();
   const std::string urls[3] = {
      value(g, "ES.Runtype.GeomOpt.QSTReactant"),
      value(g, "ES.Runtype.GeomOpt.QSTProduct"),
      value(g, "ES.Runtype.GeomOpt.QSTGuess") };
   const bool guess = !urls[2].empty();
   if (urls[0].empty() || urls[1].empty()) {
      warning = "Transition State (QST): pick the reactant and product "
                "calculations in the Calculation Editor.";
      return true;
   }

   // The calculation's own structure (a copy of the reactant) fixes the
   // atom order.
   std::vector<std::string> refEl;
   std::vector<double> ref;
   bool refOpt;
   if (!qstFinalStructure(self, refEl, ref, refOpt)) return false;

   std::vector<std::string> els[3];
   std::vector<double> xyz[3];
   const char* role[3] = {"Reactant", "Product", "TS guess"};
   int n = 2 + (guess ? 1 : 0);
   std::ostringstream st, rm, wr, dt;
   for (int k = 0; k < n; k++) {
      std::string title = role[k];
      PropertyTask* t = EDSIFactory::getPropertyTask(EcceURL(urls[k].c_str()));
      bool opt;
      if (!qstFinalStructure(t, els[k], xyz[k], opt)) {
         warning = std::string("Transition State (QST): the ") + role[k] +
                   " calculation has no structure.";
         return true;
      }
      title = t->getName();
      AtomMapResult r;
      if (!mapOnto(refEl, ref, els[k], xyz[k], role[k], "Transition State (QST)",
                   r, warning))
         return true;
      if (table) addTable(*table, role[k], refEl, r);
      if (r.nChanged > 0) {
         std::string lc = role[k];
         for (char& c : lc) c = (char)tolower((unsigned char)c);
         st << (st.tellp() ? ", " : "") << lc << " renumbered ("
            << r.nChanged << " moved)";
      }
      dt << role[k] << " renumbered: " << r.nChanged
         << " atoms changed place. ";
      char buf[32];
      sprintf(buf, "%.2f", r.alignment.rmsdAfter);
      if (k > 0) rm << (rm.tellp() ? "/" : "") << buf;   // reactant is the reference
      if (!r.far.empty())
         dt << role[k] << ": " << r.far.size()
            << " atom(s) move more than 1 A after alignment (expected for "
               "an isomerisation). ";
      if (!writeFrag(base + ".qst" + char('1'+k) + ".frag", title, els[k],
                     xyz[k]))
         return false;
   }
   char cnt[16];
   sprintf(cnt, "%d", (int)refEl.size());
   status = std::string("Matched ") + cnt + " atoms" +
            (st.tellp() ? " \xC2\xB7 " + st.str() : std::string()) +
            " \xC2\xB7 RMSD " + rm.str() + " \xC3\x85";
   warning = wr.str();
   if (detail)
      *detail = std::string("Atoms matched (") + cnt + "). " + dt.str() +
                "Aligned onto this calculation, RMSD " + rm.str() +
                " \xC3\x85.";
   return true;
}

static bool writeXyz(const std::string& file, const std::string& title,
                     const std::vector<std::string>& el,
                     const std::vector<double>& xyz)
{
   std::ofstream os(file.c_str());
   if (!os) return false;
   os.setf(std::ios::fixed);
   os.precision(8);
   os << el.size() << '\n' << title << '\n';
   for (size_t i = 0; i < el.size(); i++)
      os << el[i] << ' ' << xyz[3*i] << ' ' << xyz[3*i+1] << ' '
         << xyz[3*i+2] << '\n';
   return (bool)os;
}

bool nebWriteStructureFiles(ICalculation* self, std::string& status,
                            std::string& warning, std::string* detail, std::string* table)
{
   status.clear();
   if (table) table->clear();
   warning.clear();
   remove("neb_product.xyz");
   remove("neb_guess.xyz");
   GUIValues* g = self->guiparams();
   const std::string urls[2] = { value(g, "ES.Runtype.NEB.Product"),
                                 value(g, "ES.Runtype.NEB.Guess") };
   if (urls[0].empty()) {
      warning = "Transition State (NEB-TS): pick the product calculation "
                "in the Calculation Editor.";
      return true;
   }

   // This calculation's own structure (a copy of the reactant) fixes the order.
   std::vector<std::string> refEl;
   std::vector<double> ref;
   bool refOpt;
   if (!qstFinalStructure(self, refEl, ref, refOpt)) return false;

   const char* role[2] = {"Product", "TS guess"};
   const char* file[2] = {"neb_product.xyz", "neb_guess.xyz"};
   std::ostringstream st, rm, wr, dt;
   int done = 0;
   for (int k = 0; k < 2; k++) {
      if (urls[k].empty()) continue;
      std::vector<std::string> el;
      std::vector<double> xyz;
      bool opt;
      PropertyTask* t = EDSIFactory::getPropertyTask(EcceURL(urls[k].c_str()));
      if (!qstFinalStructure(t, el, xyz, opt)) {
         warning = std::string("Transition State (NEB-TS): the ") + role[k] +
                   " calculation has no structure.";
         return true;
      }
      AtomMapResult r;
      if (!mapOnto(refEl, ref, el, xyz, role[k], "Transition State (NEB-TS)",
                   r, warning))
         return true;
      if (table) addTable(*table, role[k], refEl, r);
      if (r.nChanged > 0) {
         std::string lc = role[k];
         for (char& c : lc) c = (char)tolower((unsigned char)c);
         st << (st.tellp() ? ", " : "") << lc << " renumbered ("
            << r.nChanged << " moved)";
      }
      dt << role[k] << " renumbered: " << r.nChanged
         << " atoms changed place. ";
      char buf[32];
      sprintf(buf, "%.2f", r.alignment.rmsdAfter);
      rm << (done++ ? "/" : "") << buf;
      if (!r.far.empty())
         dt << role[k] << ": " << r.far.size()
            << " atom(s) move more than 1 A after alignment (expected for "
               "an isomerisation). ";
      if (!writeXyz(file[k], t->getName(), el, xyz)) return false;
   }
   char cnt[16];
   sprintf(cnt, "%d", (int)refEl.size());
   status = std::string("Matched ") + cnt + " atoms" +
            (st.tellp() ? " \xC2\xB7 " + st.str() : std::string()) +
            " \xC2\xB7 RMSD " + rm.str() + " \xC3\x85";
   warning = wr.str();
   if (detail)
      *detail = std::string("Atoms matched (") + cnt + "). " + dt.str() +
                "Aligned onto this calculation, RMSD " + rm.str() +
                " \xC3\x85.";
   return true;
}
