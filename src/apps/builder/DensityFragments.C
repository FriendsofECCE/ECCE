#include <memory>

#include <wx/arrstr.h>
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>

#include "DensityFragments.H"
#include "dsm/EDSIFactory.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "util/EcceURL.H"

using std::string;
using std::vector;

namespace DensityFragments {

static void collect(Resource *folder, const string& own, const string& prefix,
                    int depth, vector<IPropCalculation*>& out, vector<string>& labels)
{
  vector<Resource*> *kids = folder ? folder->getChildren() : 0;
  if (!kids) return;
  for (size_t i = 0; i < kids->size(); i++) {
    Resource *r = (*kids)[i];
    IPropCalculation *other = dynamic_cast<IPropCalculation*>(r);
    if (other) {
      if (other->getURL().toString() == own || other->getProperty("MO") == 0) continue;
      out.push_back(other);
      labels.push_back(prefix + other->getURL().getFilePathTail());
    } else if (depth > 0 && (r->getContentType() == ResourceDescriptor::CT_PROJECT ||
                             r->getContentType() == ResourceDescriptor::CT_SESSION)) {
      collect(r, own, prefix + r->getName() + "/", depth - 1, out, labels);
    }
  }
}

vector<IPropCalculation*> candidates(IPropCalculation *calc, vector<string> *labels)
{
  vector<IPropCalculation*> out;
  vector<string> names;
  if (calc) {
    Resource *folder = EDSIFactory::getResource(calc->getURL().getParent());
    collect(folder, calc->getURL().toString(), "", 1, out, names);
  }
  if (labels) *labels = names;
  return out;
}

bool resolve(IPropCalculation *calc, const vector<string>& names,
             vector<string>& urls, string& complaint)
{
  vector<string> labels;
  const vector<IPropCalculation*> found = candidates(calc, &labels);
  urls.clear();
  for (const string& n : names) {
    size_t i = 0;
    while (i < labels.size() && labels[i] != n) i++;
    if (i == labels.size()) { complaint = "No calculation named " + n + " beside this one."; return false; }
    urls.push_back(found[i]->getURL().toString());
  }
  return true;
}

bool attach(DensityPlane::Evaluator& whole, const vector<string>& urls, string& complaint)
{
  whole.clearFragments();
  complaint.clear();
  vector< std::shared_ptr<const DensityPlane::Evaluator> > built;
  vector<char> used;
  for (const string& url : urls) {
    IPropCalculation *calc = dynamic_cast<IPropCalculation*>(
        EDSIFactory::getResource(EcceURL(url)));
    const string name = EcceURL(url).getFilePathTail();
    if (!calc) { complaint = name + " is not a calculation."; return false; }
    std::shared_ptr<DensityPlane::Evaluator> f(new DensityPlane::Evaluator);
    if (!f->build(calc)) { complaint = name + ": " + f->complaint; return false; }
    string why;
    if (!DensityPlane::matchFragment(whole, *f, used, why)) {
      complaint = name + ": " + why;
      return false;
    }
    built.push_back(f);
  }
  for (size_t i = 0; i < built.size(); i++)
    whole.addFragment(built[i], EcceURL(urls[i]).getFilePathTail());
  return true;
}

bool choose(wxWindow *parent, IPropCalculation *calc, const vector<string>& current,
            vector<string>& chosen)
{
  vector<string> labels;
  const vector<IPropCalculation*> found = candidates(calc, &labels);
  if (found.empty()) {
    wxMessageBox("No other calculation with orbitals was found in this folder. "
                 "The fragments are calculations of the same atoms, at the same "
                 "positions, computed with the same basis set.",
                 "Density difference", wxOK | wxICON_INFORMATION, parent);
    return false;
  }
  wxArrayString names;
  wxArrayInt on;
  for (size_t i = 0; i < found.size(); i++) {
    names.Add(wxString::FromUTF8(labels[i].c_str()));
    for (const string& c : current)
      if (c == found[i]->getURL().toString()) on.Add((int)i);
  }
  // Line breaks: the generic dialog sizes to its longest line.
  wxMultiChoiceDialog dlg(parent,
      "Tick the fragments to subtract from this calculation.\n"
      "Each is a calculation of some of the same atoms at the same\n"
      "positions, with the same basis set. The difference is this\n"
      "density minus theirs.",
      "Density difference", names);
  dlg.SetSelections(on);
  if (dlg.ShowModal() != wxID_OK) return false;
  chosen.clear();
  const wxArrayInt sel = dlg.GetSelections();
  for (size_t i = 0; i < sel.GetCount(); i++)
    chosen.push_back(found[sel[i]]->getURL().toString());
  return true;
}

}  // namespace DensityFragments
