#ifndef CODECHOICE_HH
#define CODECHOICE_HH
/////////////////////////////////////////////////////////////////////////////
// Class: CodeChoice
//
// Which codes a user is offered when making a NEW calculation (#1049):
// File > New, right-click New, Continue in Another Code, the workflow view
// and the Calculation Editor's code bar all filter through this one class.
// Existing calculations of a hidden code are never affected.
//
// The choice is per user (EcceGlobal "Codes.Shown", a string list).  With no
// stored choice the default is ECCE-QM plus every code registered on a
// machine, or the site's list (siteconfig/Codes) when one exists.
/////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <string>
#include <vector>

class ResourceType;

class CodeChoice
{
  public:
    struct Info {
      std::string name;      // CodeFactory name, also the applicationType
      std::string display;   // what the user reads
      std::string text;      // one line on what the code does
      bool core;             // in the core set; the rest are experimental
    };

    // Every supported code, core set first.
    static const std::vector<Info>& known();
    static std::string displayName(const std::string& code);

    static bool shown(const std::string& code);
    static std::vector<std::string> shownCodes();
    static std::vector<std::string> hiddenCodes();
    static int hiddenCount();

    static std::vector<std::string> defaultShown();
    static bool registeredCode(const std::string& code);
    static bool hasStoredChoice();

    static void setShown(const std::vector<std::string>& codes);
    static void show(const std::string& code, bool on = true);

    // False for a type that makes a new calculation of a hidden code.
    static bool typeShown(ResourceType* type);
    // Same, for a resource type given by its name ("orca_es").
    static bool typeNameShown(const std::string& typeName);
    // The last list entry: "9 more codes (hidden)...", "+9 hidden".
    static std::string moreLabel(int count);
    static std::string barLabel(int count);
    static std::string moreTip(int count);

    // Called after the choice changes, so a menu built earlier can be rebuilt.
    static void setListener(std::function<void()> listener);

    // The code a ResourceType makes calculations of, "" for other types.
    static std::string codeOfType(ResourceType* type);
};

#endif
