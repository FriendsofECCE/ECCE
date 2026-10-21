// The dialog's state and rules without any widget: values, enabled/shown
// flags, the CheckDependency pass, restore, Reset, and the lines sent to the
// Calculation Editor.  The wx panel only mirrors this model (View) and
// forwards edits to it, so every rule can be unit-tested headless.
#ifndef ECCE_DETAILS_MODEL_H
#define ECCE_DETAILS_MODEL_H

#include "DetailsRules.h"

namespace details {

// Where the lines go (UDP to the editor in the dialog, a recorder in tests).
class Transport {
public:
  virtual ~Transport() {}
  virtual void send(const std::string& datagram) = 0;
};

enum Severity { NORMAL = 0, WARNING = 1, ERROR = 2 };

class View {
public:
  virtual ~View() {}
  virtual void itemUpdated(int index) = 0;      // value / enabled / shown
  virtual void status(const std::string& text, Severity s) = 0;
  virtual void resetEnabled(bool on) = 0;
  virtual void layoutChanged() = 0;
  virtual void closeRequested() = 0;
};

struct Params {
  std::map<std::string, std::string> vars;   // the argv fields by name
  std::string restoreFile, restoreFlag, readOnly;  // GUIValues, Writable
  std::string candidatesFile;                // ECCE_QST_CANDIDATES
  bool debug = false;
};

struct Item {
  std::string kind, name, label, unit, def, enableIf, showIf, exportIf;
  std::string first, second;                 // radiopair labels
  std::string soft, hard;                    // ranges, as templates.py
  Strings choices, urls;                     // urls: calcpick only
  int exportFlag = 0;
  int box = -1;
  // run time
  std::string value, saved;
  double savedNum = 0;
  bool enabled = true, shown = true;
  bool isSetting() const { return kind != "note"; }
};

struct Box {
  std::string label;
  int cols = 1;
};

// file order of what the panel lays out
struct Entry {
  char type;     // 'B' box, 'I' item
  int index;
};

class Model {
public:
  Model(const Description& d, const Params& p) : desc_(d), par_(p) {}
  void attach(Transport *t, View *v) { transport_ = t; view_ = v; }

  bool build(std::string& error);
  const std::vector<Item>& items() const { return items_; }
  const std::vector<Box>& boxes() const { return boxes_; }
  const std::vector<Entry>& entries() const { return entries_; }
  const Strings& warnings() const { return warnings_; }
  std::string title, helpURL;
  int minWidth = 0;
  bool hideMessage = false;

  int find(const std::string& name) const;
  // Applies the restore file, reports every setting, runs the dependencies.
  void initialize();
  // A user edit of an item; text is the widget's value ("1"/"0" for toggles,
  // the string for combos, the URL for calcpick).
  void userEdit(int index, const std::string& text);
  void reset();
  void close();
  bool anyNotDefault() const;
  std::string exportLine(int index) const;
  // JSON of what the dialog would show for this argv, defaults only.
  std::string inventoryJson();
  void checkDependency();
  // Sets a value the way wx SetValue does: no event, no line.
  void setValue(int index, const std::string& text);

private:
  const Description& desc_;
  Params par_;
  Transport *transport_ = 0;
  View *view_ = 0;
  std::vector<Item> items_;
  std::vector<Box> boxes_;
  std::vector<Entry> entries_;
  Strings warnings_;
  Strings batch_;
  bool loopStarted_ = false;
  int depth_ = 0;

  struct Step {
    enum Type { ItemRule, Force, StatusMsg } type;
    int item = -1;
    std::string target, value, when, text;
    bool silent = false, on = false;
  };
  std::vector<Step> steps_;

  bool writable() const { return par_.readOnly == "Writable"; }
  bool lookupVar(const std::string& n, std::string& v) const;
  bool lookupAny(const std::string& n, std::string& v) const;
  bool lookupItem(const Item& it, const std::string& n, std::string& v) const;
  bool eval(const std::string& e, bool live, const Item *self = 0);
  std::string normalize(const Item& it, const std::string& v) const;
  bool notDefault(const Item& it) const;
  bool sendsOnEnable(const Item& it) const;
  void save(Item& it);
  void emit(int index);
  void changed(int index);
  void enable(int index, bool on);
  void touch(int index);
  void say(const std::string& t, Severity s);
  std::string typeName(const Item& it) const;
  std::string unitOf(const Item& it) const;
  void dbg(const std::string& m) const;
};

}  // namespace details
#endif
