//  Element colour schemes (Preferences > Colours): prints "Sym #rrggbb" for
//  every element, for the saved scheme, then again after the scheme was
//  changed in the preference file and applied, as the dialog does.
//
//      elementcolours show            the saved scheme
//      elementcolours switch SCHEME   print, save SCHEME, apply, print
#include <cstdio>
#include <cstring>
#include <string>

#include "tdat/TPerTab.H"
#include "util/PreferenceLabels.H"
#include "util/Preferences.H"

static void show(const char *tag)
{
  TPerTab tpt;
  printf("scheme %s %s\n", tag, TPerTab::colourSchemePref().c_str());
  for (int z = 0; z < tpt.numAtoms(); z++)
    printf("%s %s %s\n", tag, tpt.atomicSymbol(z).c_str(),
           tpt.colorName(z).c_str());
}

int main(int argc, char **argv)
{
  if (argc >= 2 && !strcmp(argv[1], "show")) {
    show("saved");
    return 0;
  }
  if (argc >= 3 && !strcmp(argv[1], "switch")) {
    show("before");
    {
      Preferences prefs(PrefLabels::GLOBALPREFFILE);
      prefs.setString(PrefLabels::ELEMENTCOLOURS, argv[2]);
      prefs.saveFile();
    }
    TPerTab::applyColourScheme();
    show("after");
    return 0;
  }
  return 2;
}
