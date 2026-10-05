#ifndef PROCESSMACHINE_HH
#define PROCESSMACHINE_HH

#include <string>

// The form Machine Registration posts to $ECCE_HOME/scripts/processmachine.
// processmachine percent-decodes every name and value exactly once, so each
// one is encoded here and nothing in it can split or corrupt the form.
class ProcessMachine
{
  public:
    // Every byte but [A-Za-z0-9-_.~] as %XX.
    static std::string encode(const std::string& text);

    // "&name=value", both encoded.
    static std::string field(const std::string& name, const std::string& value);

    // Runs processmachine with the form on stdin.  No shell is involved.
    // Returns its exit status, or -1 if it could not be run.
    static int run(const std::string& form);
};

#endif
