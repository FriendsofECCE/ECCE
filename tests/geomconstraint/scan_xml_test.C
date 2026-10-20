//  #393: a scanned constraint keeps its steps and step size through the
//  constraint XML; an unscanned one stays unscanned.
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/Fragment.H"
#include "tdat/GeomConstraint.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"
#include "dsm/GeomConstraintModelXMLizer.H"
#include "util/EcceMap.H"

static int failures = 0;

static void check(bool ok, const std::string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

int main()
{
  Fragment frag;
  const char* tags[] = {"O", "O", "H", "H"};
  const double xyz[] = {0, 0.73, 0,  0, -0.73, 0,  0.85, 0.9, 0.3,  -0.85, -0.9, 0.3};
  for (int i = 0; i < 4; i++) frag.addAtom(tags[i], xyz + 3 * i);
  frag.addBond(0, 1, 1.0);
  frag.addBond(0, 2, 1.0);
  frag.addBond(1, 3, 1.0);

  GeomConstraintRules rules;
  GeomConstraintModel model(frag, rules);
  GeomConstraint *tors = new GeomConstraint(frag.atomRef(2), frag.atomRef(0),
                                            frag.atomRef(1), frag.atomRef(3));
  tors->setScan(6, 30.0);
  model.addConstraint(tors);
  model.addConstraint(new GeomConstraint(frag.atomRef(0), frag.atomRef(1)));

  GeomConstraintModelXMLizer xml;
  EcceMap props;
  std::string text;
  xml.serialize(model, text, props);
  check(text.find("<Scan steps=\"6\" step=\"30\"/>") != std::string::npos,
        "the torsion is written with its <Scan>");

  GeomConstraintModel back(frag, rules);
  xml.deserialize(text, back, props);
  std::vector<GeomConstraint*> *t = back.getTorsionConstraints();
  std::vector<GeomConstraint*> *b = back.getBondConstraints();
  check(t->size() == 1 && (*t)[0]->getScanSteps() == 6 &&
        (*t)[0]->getScanStep() == 30.0, "torsion scan survives the round trip");
  check(b->size() == 1 && (*b)[0]->getScanSteps() == 0,
        "a plain bond constraint has no scan");
  delete t;
  delete b;
  return failures ? 1 : 0;
}
