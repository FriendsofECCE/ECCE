// The wx-free half of the native Details dialog engine: the .dlg line parser
// and the rule evaluator (expressions, ranges, number formatting).  Grammar:
// docs/claude/codereg/dlg-grammar.md.
#ifndef ECCE_DETAILS_RULES_H
#define ECCE_DETAILS_RULES_H

#include <functional>
#include <istream>
#include <map>
#include <string>
#include <vector>

namespace details {

typedef std::vector<std::string> Strings;
typedef std::map<std::string, Strings> Lists;

std::string trim(const std::string& s);
Strings split(const std::string& s, char sep);
bool toNumber(const std::string& s, double& v);
// Python's str(float), so a reformatted value is byte-identical to the one
// the wxPython dialog sends.
std::string pyFloatStr(double v);

// One .dlg line: a command word, key=value pairs (values may be "quoted").
struct Line {
  std::string cmd;
  std::map<std::string, std::string> kv;
  std::string get(const std::string& k, const std::string& d = "") const
  {
    std::map<std::string, std::string>::const_iterator it = kv.find(k);
    return it == kv.end() ? d : it->second;
  }
  bool has(const std::string& k) const { return kv.count(k) != 0; }
};

bool parseLine(const std::string& raw, Line& out);

struct Description {
  std::vector<Line> lines;   // file order, "list" lines removed
  Lists lists;
  std::string error;
  bool load(const std::string& path);
  bool parse(std::istream& in);
};

// Resolves a variable (argv field or control value); false if unknown.
typedef std::function<bool(const std::string&, std::string&)> Lookup;

// "a==b && (c>1 || d in @L) && !e".  The left side of a comparison is looked
// up (unknown -> ""), the right side is a literal unless it starts with $ or
// is @N (a choice index).  Errors go to *err and the result is false.
bool evalExpr(const std::string& expr, const Lookup& lookup,
              const Lists& lists, std::string* err = 0);

// Replaces $name by its value where the lookup knows it.
std::string substitute(const std::string& s, const Lookup& lookup);

// templates.py GetRangeBounds / InRange.
bool rangeBounds(const std::string& range, bool& hasLo, double& lo,
                 bool& hasHi, double& hi);
// false if value is outside; message then says how ("greater than 3.").
bool inRange(const std::string& range, double value, std::string& message);

}  // namespace details
#endif
