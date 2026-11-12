#ifndef SMILESBATCH_HH
#define SMILESBATCH_HH

#include <functional>
#include <string>
#include <utility>
#include <vector>

class EcceURL;
class Fragment;

/**
 * Batch Add from SMILES (#961): a table of names, SMILES and measured values
 * becomes one calculation per row in a folder, each with the setup of one
 * template calculation (Series::make).  Used by ecce-batch-smiles and the
 * Organizer's Tools > Batch Add from SMILES.  No wx.
 */
namespace SmilesBatch {

  /// Properties on each calculation, for the descriptor table to read.
  extern const char *kSmilesProp;     // ecce:qsar.smiles, the SMILES as given
  extern const char *kMeasuredProp;   // ecce:qsar.measured, JSON {header: value}

  struct Table {
    std::vector<std::string> header;  // "column 1", ... when the file has none
    std::vector<std::vector<std::string> > rows;
    std::vector<int> lines;           // 1-based line of each row in the text
    char delimiter = ',';
  };

  /// CSV, TSV or semicolon-separated text, "quoted" cells allowed.  The
  /// first line is a header unless one of its cells is a number (header
  /// -1: guess, 0: no, 1: yes).
  bool parseTable(const std::string& text, Table& table, std::string& why,
                  int header = -1);

  struct Columns {
    int name = -1;                    // -1: rows are named mol-001, ...
    int smiles = -1;
    std::vector<int> responses;       // measured values, kept with their headers
  };

  /// SMILES: a header containing "smiles", else the first column whose
  /// first cells all parse.  Name: a header such as name/id/compound, else
  /// the first other column that is not numeric.  Responses: the other
  /// columns that are mostly numbers.
  Columns guessColumns(const Table& table);

  /// Column index of a header (case-insensitive) or a 1-based number; -1.
  int findColumn(const Table& table, const std::string& key);

  struct Row {
    int line = 0;
    std::string name, smiles;
    std::vector<std::pair<std::string, std::string> > measured;
    int charge = 0;                   // sum of the formal charges
    int atoms = 0;                    // with hydrogens
    std::string problem;              // why it will not be made, or ""
  };

  /// The rows as they would be made, each SMILES read (no 3-D yet), and a
  /// name used twice or a SMILES that does not parse given as the problem.
  std::vector<Row> preview(const Table& table, const Columns& columns);

  /// 3-D structure of a SMILES with formal charges, then the force-field
  /// Clean that Tools > Add from SMILES in the Builder applies.
  Fragment *structure(const std::string& smiles, int& charge, std::string& why);

  /// A calculation "QSAR template" in the folder (or the existing one):
  /// xTB, GFN2-xTB, Geometry, this computer with 4 threads.  "" and why
  /// when it could not be made.
  std::string defaultTemplate(const EcceURL& folder, std::string& why);

  struct Outcome {
    std::string name, url, smiles, note;
    int line = 0;
    int charge = 0;
    bool ok = false;
  };

  /// One calculation per row without a problem (rows with one come back as
  /// not made, with the problem as the note).  An empty template URL makes
  /// or reuses defaultTemplate; templateUrl gets the one used.  progress
  /// (rows done, rows, name) is told before each row.  replace: a
  /// calculation of the row's name that was never launched is made again
  /// (an edited template applied to the batch); a launched one is kept.
  std::vector<Outcome> make(const EcceURL& folder, const std::string& templ,
                            const std::vector<Row>& rows,
                            std::string& templateUrl, std::string& why,
                            bool replace,
                            std::function<void(int, int, const std::string&)>
                                progress = nullptr);

  /// The outcomes as one JSON object (folder, template, made, failed, rows).
  std::string json(const std::string& folder, const std::string& templ,
                   const std::vector<Outcome>& outcomes);
}

#endif
