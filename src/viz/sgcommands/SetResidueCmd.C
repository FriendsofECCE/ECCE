#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "tdat/Residue.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/SetResidueCmd.H"

#include <cstdlib>

SetResidueCmd::SetResidueCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
  p_sender = sender;
  p_resnum = -1;
  init();
}

SetResidueCmd::~SetResidueCmd()
{
}

void SetResidueCmd::init()
{
  addParameter(new CommandParameter("resnum", 0));
  addParameter(new CommandParameter("property", ""));
  addParameter(new CommandParameter("value", ""));
  setHelpMessage("Set a residue property");
}

// prop is "name", "pdbnum" or "chain"; false for anything else.
bool SetResidueCmd::apply(int resnum, const string& prop, const string& value,
                          string *old)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg ? sg->getFragment() : 0;
  if (frag == 0 || resnum < 0 || resnum >= frag->numResidues()) return false;
  Residue *res = frag->residue(resnum);
  if (res == 0) return false;

  if (prop == "name") {
    if (old) *old = res->name();
    res->name(value.c_str());
  } else if (prop == "pdbnum") {
    if (old) *old = std::to_string(res->number());
    res->number(atoi(value.c_str()));
  } else if (prop == "chain") {
    if (old) *old = string(1, res->chain());
    res->chain(value.empty() ? ' ' : value[0]);
  } else {
    return false;
  }
  return true;
}

bool SetResidueCmd::execute()
{
  p_resnum = getParameter("resnum")->getInteger();
  p_prop = getParameter("property")->getString();
  if (!apply(p_resnum, p_prop, getParameter("value")->getString(), &p_old))
    return false;
  Event event("GeomChange", "", p_sender);
  EventDispatcher::getDispatcher().publish(event);
  return true;
}

void SetResidueCmd::undo()
{
  if (apply(p_resnum, p_prop, p_old, 0)) {
    Event event("GeomChange");
    EventDispatcher::getDispatcher().publish(event);
  }
}
