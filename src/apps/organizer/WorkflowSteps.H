/**
 * @file
 *
 * What a Workflow does with its steps, apart from drawing them: making a step
 * from an earlier one, finding the steps that can be launched, and a
 * one-line description of every step and link for the test hooks.
 */

#ifndef WORKFLOWSTEPS_HH
#define WORKFLOWSTEPS_HH

#include <string>
#include <vector>

#include "dsm/ResourceDescriptor.H"

class Resource;
class ResourceType;
class Session;

namespace WorkflowSteps {

  /// A code the workflow offers: label on the box, resource type, colour.
  struct Code {
    const char *label;
    const char *typeName;
    unsigned char r, g, b;
  };

  const std::vector<Code>& codes();

  /// The code of a step, or 0.
  const Code *codeOf(const Resource *step);

  struct Carry {
    bool geometry;
    bool chargeMult;
    /// For a scan or IRC source: "last", "max" or a point number (ScanPoint.H);
    /// empty takes the final geometry.
    std::string point;
    Carry() : geometry(true), chargeMult(true) {}
  };

  /// The link name a step carries its geometry under.
  extern const char *const kGeometryLink;

  /// Link names are "<role>" or "<role>@<point>": geometry, reactant,
  /// product, tsguess.
  std::string linkName(const std::string& role, const std::string& point);
  void splitLinkName(const std::string& name, std::string& role,
                     std::string& point);
  /// What the arrow says: "geometry", "geometry (maximum)", "TS guess (last
  /// point)".
  std::string linkLabel(const std::string& name);

  /// True if the source is a scan or IRC with a structure for every point.
  bool isScan(Resource *step);

  /// True if the step has a geometry from a run (not only its input).
  bool hasResultGeometry(Resource *step);

  /// The state of a calculation; STATE_ILLEGAL for anything else.
  ResourceDescriptor::RUNSTATE stateOf(Resource *step);
  bool isComplete(Resource *step);
  bool isLaunched(Resource *step);

  /// A name for a new step of this code that no member has yet.
  std::string defaultName(Session *wf, const Code& code);

  /// Makes a step of type rt in wf, links it to source (null for the first
  /// step) and copies what carry says.  Null with `why` set on failure.
  Resource *create(Session *wf, ResourceType *rt, const std::string& name,
                   Resource *source, const Carry& carry, std::string& why);

  /// One source of a step and the name of its link.
  struct Incoming {
    Resource *source;
    std::string name;
  };

  /// The members, sources before the steps made from them.  `source` is the
  /// one source of an ordinary step (0 for a first step, and for a join);
  /// `in` lists every source, a join having several.  A step is one column
  /// right of its right-most source.
  struct Node {
    Resource *step;
    Resource *source;
    int depth;
    std::vector<Incoming> in;
  };

  /// A source of a join with the role it plays (reactant, product, tsguess)
  /// and the scan point it contributes ("" its final geometry).
  struct JoinSource {
    Resource *step;
    std::string role;
    std::string point;
  };

  /// Codes that can take several sources: G16 (QST2/QST3), ORCA (NEB-TS).
  bool canJoin(const Code& code);

  /// "Transition State (QST) + Frequencies" or the NEB-TS one.
  std::string joinRunType(const Code& code);

  /// Roles for the sources in the order given: a scan among three becomes
  /// the TS guess at its maximum, the others reactant then product.
  std::vector<JoinSource> defaultRoles(const std::vector<Resource*>& sources);

  /// Empty when the roles make a QST2/QST3/NEB-TS: one reactant, one
  /// product, at most one TS guess.
  std::string checkRoles(const std::vector<JoinSource>& sources);

  /// Makes a step of rt that takes its structures from the sources.  Its own
  /// structure, charge and multiplicity are the reactant's; its run type and
  /// picks are the ones the Calculation Editor's TS Search stores, so the
  /// input is written by the same path.  Null with `why` set on failure.
  Resource *createJoin(Session *wf, ResourceType *rt, const std::string& name,
                       const std::vector<JoinSource>& sources,
                       std::string& why);

  std::vector<Node> tree(Session *wf);

  /// Steps not yet launched whose source is complete; `waiting` gets the
  /// names of the other unlaunched ones.
  std::vector<Resource*> readySteps(Session *wf,
                                    std::vector<std::string> *waiting = 0);

  /// "name code state source charge mult atoms" per step (a join's source is
  /// "a+b+c"), joined by "|",
  /// then " links:" and "src>dst:name" for each link.
  std::string info(Session *wf);
}

#endif
