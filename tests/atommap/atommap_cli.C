// atommap reactant.xyz product.xyz [guess.xyz] [--write out.xyz]
//
// Maps the atoms of product (and guess) onto the reactant's numbering and
// superposes them (#419).  --write gives the renumbered, aligned product
// (a guess goes to <out>.guess.xyz).  Exit 0 when mapped, 1 on error.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "tdat/AtomMap.H"
#include "xyzio.H"

static bool writeXyz(const string& path, const vector<string>& el,
                     const vector<double>& c, const string& comment)
{
   FILE* f = fopen(path.c_str(), "w");
   if (!f) return false;
   fprintf(f, "%d\n%s\n", (int)el.size(), comment.c_str());
   for (size_t i = 0; i < el.size(); i++)
      fprintf(f, "%-3s %14.8f %14.8f %14.8f\n", el[i].c_str(),
              c[3*i], c[3*i+1], c[3*i+2]);
   fclose(f);
   return true;
}

static int report(const char* name, const Xyz& ref, const Xyz& mov,
                  const string& writePath)
{
   const AtomMapResult r = AtomMap::map(ref.el, ref.c, mov.el, mov.c);
   printf("== %s\n", name);
   if (!r.ok) {
      printf("ERROR: %s\n", r.error.c_str());
      return 1;
   }
   printf("%s; %d of %d atoms changed place\n",
          r.identity ? "order kept" : "renumbered", r.nChanged,
          (int)ref.el.size());
   printf("RMSD %.3f A before alignment, %.3f A after\n",
          r.alignment.rmsdBefore, r.alignment.rmsdAfter);
   printf("%-4s %-3s -> %-4s %9s\n", "ref", "el", "mov", "disp(A)");
   vector<int> inv(r.map.size());
   for (size_t i = 0; i < r.map.size(); i++) inv[r.map[i]] = (int)i;
   for (size_t a = 0; a < inv.size(); a++) {
      const int i = inv[a];
      printf("%-4d %-3s -> %-4d %9.3f%s\n", (int)a + 1, ref.el[a].c_str(),
             i + 1, r.displacement[i], r.displacement[i] > 1.0 ? "  FAR" : "");
   }
   printf("%d pair(s) flagged far (> 1.0 A)\n", (int)r.far.size());
   if (!writePath.empty()) {
      vector<double> c = mov.c;
      AtomMap::apply(r.alignment, c);
      if (!writeXyz(writePath, ref.el, AtomMap::renumber(c, r.map),
                    "renumbered and aligned onto the reactant")) {
         printf("ERROR: cannot write %s\n", writePath.c_str());
         return 1;
      }
   }
   return 0;
}

int main(int argc, char** argv)
{
   vector<string> files;
   string out;
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--write") && i + 1 < argc) out = argv[++i];
      else files.push_back(argv[i]);
   }
   if (files.size() < 2 || files.size() > 3) {
      fprintf(stderr, "usage: atommap reactant.xyz product.xyz [guess.xyz] [--write out.xyz]\n");
      return 2;
   }
   vector<Xyz> s(files.size());
   for (size_t i = 0; i < files.size(); i++)
      if (!readXyz(files[i], s[i])) {
         fprintf(stderr, "cannot read %s\n", files[i].c_str());
         return 2;
      }
   int rc = report("product", s[0], s[1], out);
   if (files.size() == 3)
      rc |= report("guess", s[0], s[2], out.empty() ? out : out + ".guess.xyz");
   return rc;
}
