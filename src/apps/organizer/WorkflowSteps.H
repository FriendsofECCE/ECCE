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

#include <map>

#include "dsm/ResourceDescriptor.H"

#include "FragOrbBox.H"
#include "KeyData.H"
#include "Recipe.H"

class Fragment;
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
    /// From a CREST ensemble: make this many steps, one per conformer from
    /// the lowest relative energy up (0 = one ordinary step).
    int best;
    /// From a CREST ensemble: one step per coordination pattern, from the
    /// pattern's lowest-energy conformer (ScanPoint::ensemblePatterns).
    bool perPattern;
    bool hessian;   ///< the source's Hessian as the initial Hessian (ORCA)
    Carry() : geometry(true), chargeMult(true), best(0), perPattern(false),
              hessian(false) {}
  };

  /// The link name a step carries its geometry under.
  extern const char *const kGeometryLink;

  /// True if `to` can start from the Hessian of `source`: both are ORCA
  /// calculations and the source kept a .hess.
  bool canCarryHessian(Resource *source, ResourceType *toType);

  /// Link names are "<role>" or "<role>.<point>" ("@" is not allowed in a link
  /// name): geometry, reactant,
  /// product, tsguess.
  std::string linkName(const std::string& role, const std::string& point);
  void splitLinkName(const std::string& name, std::string& role,
                     std::string& point);
  /// What the arrow says: "geometry", "geometry (maximum)", "TS guess (last
  /// point)".
  std::string linkLabel(const std::string& name);

  /// The step's kind as a short tag for its box: "Opt", "Opt + freq",
  /// "TS (QST3) + freq", "IRC", "Scan", "Single point", "Freq", "Conformer
  /// search", ... ("" when the run type is not known).
  std::string stepTag(Resource *step);

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

  /// True if the source is a CREST ensemble (a structure and a relative
  /// energy per conformer).
  bool isEnsemble(Resource *step);

  /// Makes n steps from the n lowest-energy conformers of an ensemble
  /// source, named "<name>-conf<k>", each linked as "geometry.c<k>" and
  /// annotated with its rank and relative energy.  Empty with `why` set if
  /// n is not 1..ensemble size or a step fails; nothing is left behind.
  std::vector<Resource*> createBest(Session *wf, ResourceType *rt,
                                    const std::string& name, Resource *source,
                                    int n, const Carry& carry, std::string& why);

  /// The same for the given ranks (1 = lowest relative energy), in order.
  std::vector<Resource*> createRanks(Session *wf, ResourceType *rt,
                                     const std::string& name, Resource *source,
                                     const std::vector<int>& ranks,
                                     const Carry& carry, std::string& why);

  /// Like createBest, but one step per coordination pattern of the ensemble,
  /// named "<name>-pat<k>" (k = 1.. by the pattern's lowest energy) and
  /// annotated with the pattern, its count and its representative.
  std::vector<Resource*> createPatterns(Session *wf, ResourceType *rt,
                                        const std::string& name,
                                        Resource *source, const Carry& carry,
                                        std::string& why);

  /// True when step is a geometry optimisation by run type (Geometry, GeoVib).
  bool isOptimisation(Resource *step);

  /// True when step is a completed xTB geometry optimisation: the only
  /// structure CREST accepts without an xTB optimisation before it (#904).
  bool isXtbOptimisation(Resource *step);

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

  // ---- recipes (#710) ----

  /// The level of theory chosen for each recipe role: role id -> method, basis.
  typedef std::map<std::string, std::pair<std::string, std::string> > Levels;

  /// Makes the variant's steps in wf, linked as the recipe says, and returns
  /// them in recipe order (empty with `why` set on failure; nothing is left
  /// behind).  With `after`, the steps without a source start from it (its
  /// geometry, charge and multiplicity); otherwise those that need a
  /// structure are placeholders.  Settings, run types, the TS-search picks
  /// and the roles are set; the structures are not.
  std::vector<Resource*> applyRecipe(Session *wf, const Recipe& recipe,
                                     const RecipeVariant& variant,
                                     const Levels& levels, Resource *after,
                                     std::string& why);

  /// A recipe step with no source (hasSource false) and no structure yet.
  bool needsStructure(Resource *step, bool hasSource);

  /// The recipe a workflow was made from ("" for any other workflow).
  std::string recipeId(Session *wf);

  /// Attaches a finished field step's orbitals as the guess of `step`, once.
  /// The deck only carries MORead if this ran before the editor saved it.
  bool attachFieldOrbitals(Resource *step, Resource *source);

  /// Takes the geometry, charge and multiplicity of a recipe step's sources
  /// just before it is launched (they did not exist when it was made).
  /// A hand-made step takes its source's result only when the source had not
  /// run when the step was made.  Empty, or why not.
  std::string refreshFromSources(Session *wf, Resource *step, bool prepare = true);

  /// A Created step with sources (any recipe step) whose sources are all
  /// Complete, whose source results have been taken and which has a
  /// structure gets its input written (DeckWriter, as the editor's Save) and
  /// becomes Ready; a Ready one still waiting goes back to Created; with no launch settings it takes a source's, else those
  /// of another step of the workflow.  Empty, or why it is not launchable,
  /// which is also stored on the step for its box (kNotReadyProp).
  std::string prepareStep(Session *wf, Resource *step);
  extern const char *const kNotReadyProp;

  /// A recipe step's structure was stored (by the Builder, or carried in
  /// from its source): the recipe's rules are applied again with what is now
  /// known of the molecule, and each recipe step that has a structure gets
  /// its role's method and basis set.  A level chosen by hand is kept.
  /// Returns what was stored, for the test hook.
  std::string structureSet(Session *wf, Resource *step);

  /// The code's own value for a recipe method, "" when it has none.
  std::string methodValue(const std::string& codeType, const std::string& method);

  /// The small line under a box's name (#810): a finished step's energy and
  /// imaginary modes, or its ensemble size; a running step's progress; why a
  /// failed one stopped.  `progress` is the live or stored snapshot text.
  /// "" when there is nothing to say.
  std::string resultLine(Resource *step, const std::string& progress);

  /// The steps of a recipe workflow with their energies, for keyData().
  std::vector<KeyStep> keySteps(Session *wf);

  /// What proposeFragments (Recipe.H) suggests for the step's structure,
  /// with its atomic numbers and charge for checkFragments; empty without
  /// a structure.
  std::vector<RecipeFragment> proposedFragments(Resource *step,
                                                std::vector<int> *z = 0,
                                                int *charge = 0);

  /// The steps of a fragment analysis (#201): the source of the first
  /// fragment step (the complex), then that complex's fragment steps.
  /// Empty when the workflow has none.
  std::vector<FragStep> fragmentSteps(Session *wf);

  /// A workflow as a recipe: settings and links kept, structures and
  /// results dropped.  False with `why` set when it has no steps.
  bool toRecipe(Session *wf, const std::string& id, const std::string& title,
                Recipe& out, std::string& why);

  /// Where Save Workflow as Recipe writes: the user's Recipes directory.
  std::string userRecipeDir();

  std::vector<Node> tree(Session *wf);

  /// Steps not yet launched whose source is complete; `waiting` gets the
  /// names of the other unlaunched ones, `excluded` those left out because
  /// they are marked excluded.
  std::vector<Resource*> readySteps(Session *wf,
                                    std::vector<std::string> *waiting = 0,
                                    std::vector<std::string> *excluded = 0);

  /// Why each step cannot be launched now ("already run", "excluded",
  /// "waiting for A, B"); a ready step has no entry.
  std::map<Resource*, std::string> stepBlockers(Session *wf);

  /// The workflow's "run the remaining steps automatically" setting (#736).
  bool autoRun(Session *wf);
  void setAutoRun(Session *wf, bool on);

  /// "name code state source charge mult atoms" per step (a join's source is
  /// "a+b+c"), joined by "|",
  /// then " links:" and "src>dst:name" for each link.
  std::string info(Session *wf);

  /// A link proposed between calculations moved into a workflow (#874).
  struct ProposedLink {
    Resource *source;
    Resource *target;
    std::string name;   ///< the link's name (kGeometryLink)
    std::string why;    ///< "orbitals" or "Hessian" (recorded), "geometry" (matched)
  };

  /// At most one link into each target, from a source: the one the target
  /// records as where its orbitals or Hessian came from, else one whose
  /// final geometry is the target's starting geometry.  No cycles.
  std::vector<ProposedLink> proposeLinks(const std::vector<Resource*>& sources,
                                         const std::vector<Resource*>& targets);

  /// Same elements in the same order and every interatomic distance within
  /// tol (Angstrom): a structure is the same in any orientation.
  bool sameStructure(const Fragment *a, const Fragment *b, double tol = 0.01);
}

#endif
