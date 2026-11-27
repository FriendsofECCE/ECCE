// ecce-pockets: binding-pocket detection on a PDB file (#1320), headless.
//
//   ecce-pockets FILE.pdb [--spacing A] [--margin A] [--probe A] [--max-ray A]
//        [--min-score N] [--min-volume A3] [--lining A] [--top N]
//        [--keep-water] [--keep-ligands] [--threads N]
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "dock/PocketFinder.H"
#include "tdat/Fragment.H"

using namespace std;

int main(int argc, char **argv)
{
  string file;
  PocketFinder::Options o;
  int top = 10;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    bool more = i + 1 < argc;
    if (a == "--spacing" && more) o.spacing = atof(argv[++i]);
    else if (a == "--margin" && more) o.margin = atof(argv[++i]);
    else if (a == "--probe" && more) o.probe = atof(argv[++i]);
    else if (a == "--max-ray" && more) o.maxRay = atof(argv[++i]);
    else if (a == "--min-score" && more) o.minScore = atoi(argv[++i]);
    else if (a == "--min-volume" && more) o.minVolume = atof(argv[++i]);
    else if (a == "--lining" && more) o.liningDistance = atof(argv[++i]);
    else if (a == "--top" && more) top = atoi(argv[++i]);
    else if (a == "--threads" && more) o.threads = atoi(argv[++i]);
    else if (a == "--keep-water") o.ignoreWater = false;
    else if (a == "--keep-ligands") o.ignoreLigands = false;
    else if (a[0] != '-' && file.empty()) file = a;
    else { cerr << "unknown or incomplete option " << a << "\n"; return 2; }
  }
  if (file.empty()) {
    cerr << "usage: ecce-pockets FILE.pdb [--spacing A] [--margin A] [--probe A] [--max-ray A]\n"
            "         [--min-score N] [--min-volume A3] [--lining A] [--top N]\n"
            "         [--keep-water] [--keep-ligands] [--threads N]\n";
    return 2;
  }
  ifstream in(file);
  Fragment frag;
  if (!in || !frag.restorePDB(in, 1.0, false, 1, " ", true, false, " ", false)) {
    cerr << "cannot read " << file << " as PDB\n";
    return 1;
  }
  PocketFinder::Result r = PocketFinder::findPockets(frag, o);
  printf("atoms used %d, ignored: %d water, %d ligand\n", r.atomsUsed,
         r.atomsIgnoredWater, r.atomsIgnoredLigand);
  if (r.radiusFromCovalent || r.radiusFromDefault) {
    printf("WARNING: no vdW radius for %d atoms (covalent+0.8 used) and no radius at all "
           "for %d (1.8 used); atomic numbers:", r.radiusFromCovalent, r.radiusFromDefault);
    for (int z : r.fallbackElements) printf(" %d", z);
    printf("\n");
  }
  printf("%d pockets\n", (int)r.pockets.size());
  for (int i = 0; i < (int)r.pockets.size() && i < top; i++) {
    const PocketFinder::Pocket& p = r.pockets[i];
    printf("#%d score %.1f volume %.0f A3 enclosure %.2f centre %.2f %.2f %.2f "
           "extent %.1f x %.1f x %.1f\n  lining:", i + 1, p.score, p.volume, p.meanEnclosure,
           p.centre[0], p.centre[1], p.centre[2],
           p.hi[0]-p.lo[0], p.hi[1]-p.lo[1], p.hi[2]-p.lo[2]);
    for (const auto& l : p.lining) printf(" %s%d%c", l.name.c_str(), l.number, l.chain);
    printf("\n");
  }
  return 0;
}
