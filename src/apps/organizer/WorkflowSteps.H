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
    Carry() : geometry(true), chargeMult(true) {}
  };

  /// The link name a step carries its geometry under.
  extern const char *const kGeometryLink;

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

  /// The members, parents before children, with the source of each (or 0).
  struct Node {
    Resource *step;
    Resource *source;
    int depth;
  };
  std::vector<Node> tree(Session *wf);

  /// Steps not yet launched whose source is complete; `waiting` gets the
  /// names of the other unlaunched ones.
  std::vector<Resource*> readySteps(Session *wf,
                                    std::vector<std::string> *waiting = 0);

  /// "name code state source charge mult atoms" per step, joined by "|",
  /// then " links:" and "src>dst:name" for each link.
  std::string info(Session *wf);
}

#endif
