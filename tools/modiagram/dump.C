//  Run the real diagram engine on a molecule and print the model.
//
//  WHY THIS EXISTS.  The diagram is drawn by a wxWidgets canvas inside
//  builder, so looking at a layout change meant building a package,
//  installing it, starting the services and opening a calculation --
//  minutes per iteration, on someone else's desktop.  This runs the
//  SAME MoFragments and MoDiagram code against a molecule file and
//  prints what they produced, which tools/modiagram/draw.py then
//  renders.
//
//  It is a development tool, not a second implementation: every number
//  it prints came out of the engine builder uses.  The drawing in
//  draw.py IS a second implementation of the layout, and is for
//  agreeing what the diagram should look like before that layout is
//  written into the canvas.
//
//  Input file:
//      group  <NAME>                 the point group, uppercase
//      atom   <symbol> <x> <y> <z>   Angstrom
//      orbital <energy> <occupancy> [label]    Hartree; repeatable
//      basis  <n> <n> ...            basis functions per atom
//      shells <l> <l> ...            the angular momentum of each basis
//                                    function, in order
//      coef   <c> <c> ...            one line per orbital, in order
//
//  Output: plain text, one record per line, read by draw.py.
#include "tdat/MoSpec.H"

static void printColumn(const char* side, const MoColumn& col)
{
  printf("column\t%s\t%s\t%d\t%g\t%d", side, col.title.c_str(),
         col.hiddenCount, col.hiddenMaxEnergy, col.hiddenAboveCount);
  printf("\t%d", col.sketchNormal);
  for (size_t k = 0; k < col.sketchX.size(); k++) {
    printf("\t%.4f,%.4f", col.sketchX[k], col.sketchY[k]);
  }
  printf("\n");
  for (size_t i = 0; i < col.levels.size(); i++) {
    const MoLevel& l = col.levels[i];
    printf("level\t%s\t%zu\t%.10g\t%g\t%d\t%s\t%s\t%s\t%d\t%d",
           side, i, l.energy, l.occupancy, l.degeneracy,
           l.label.c_str(), l.irrep.c_str(),
           l.annotation.empty() ? "-" : l.annotation.c_str(),
           (int)l.character, l.pairing);
    printf("\t%.4f\t%.4f\t%d", l.shareLeft, l.shareRight, l.shell);
    printf("\tphase");
    for (size_t k = 0; k < l.phases.size(); k++) printf(",%.4f", l.phases[k]);
    for (size_t k = 0; k < l.energies.size(); k++) {
      printf("\t%.10g", l.energies[k]);
    }
    printf("\n");
  }
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    fprintf(stderr, "usage: dump <CharacterTables> <molecule>\n");
    return 2;
  }

  MoModel model;
  buildModel(argv[1], argv[2], model);

  const MoColumn& left   = model.left;
  const MoColumn& centre = model.centre;
  const MoColumn& right  = model.right;
  const vector<MoConnection>& links = model.links;
  const vector<double>& coords    = model.coords;
  const vector<string>& elements  = model.elements;
  const vector<string>& labels    = model.labels;
  const vector<int>& perAtom      = model.perAtom;
  const vector<int>& shellOf      = model.shellOf;
  const string& group = model.group;
  const string& note  = model.note;

  //  THE CHECK THAT DOES NOT KNOW THE ANSWER: what the basis spans
  //  against what the code says its orbitals span.  Two programs,
  //  different data, and it works on a molecule nobody has written an
  //  expected answer for.
  if (!perAtom.empty() && !shellOf.empty() && !labels.empty()) {
    vector< vector<int> > shellsPerAtom;
    int at = 0;
    for (size_t a = 0; a < perAtom.size(); a++) {
      vector<int> mine;
      //  One entry per SHELL, not per function: consecutive functions
      //  of the same l on one atom are one shell.
      for (int f = 0; f < perAtom[a]; ) {
        const int l = shellOf[at + f];
        mine.push_back(l);
        const int width = 2*l + 1;
        f += width;
      }
      shellsPerAtom.push_back(mine);
      at += perAtom[a];
    }

    string detail;
    const bool agree = MoFragments::basisSpansReported(
        coords, elements, group, shellsPerAtom, labels, false, detail);
    printf("oracle\t%s\t%s\n", agree ? "agree" : "DISAGREE", detail.c_str());
  }

  printf("group\t%s\n", group.c_str());
  printf("note\t%s\n", note.empty() ? "-" : note.c_str());
  printColumn("left", left);
  printColumn("centre", centre);
  printColumn("right", right);
  for (size_t i = 0; i < links.size(); i++) {
    printf("link\t%d\t%d\t%d\n", links[i].leftLevel, links[i].centreLevel,
           links[i].rightLevel);
  }
  return 0;
}
