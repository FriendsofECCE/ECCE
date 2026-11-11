#ifndef SIEXPORT_HH
#define SIEXPORT_HH

#include <functional>
#include <string>
#include <vector>

#include "util/EcceURL.H"

/// Supporting Information export (#625): writes a scratch folder per stored
/// calculation with what ecce-si-export reads, runs it, and returns false
/// with `why` set when that fails.  `table` is "", "csv" or "latex".
bool siExport(const std::vector<EcceURL>& calcs, const std::string& xyzFile,
              const std::string& table, std::string& why,
              bool includeExcluded = false);

/// The Checks table (#615) of `calcs` as `format` ("csv" or "latex"), made by
/// ecce-si-export --checks from the same scratch folders.
bool checksTable(const std::vector<EcceURL>& calcs, const std::string& format,
                 std::string& text, std::string& why);

/// Called before each calculation is gathered (index, count, name); false
/// cancels, and `why` then reads "Cancelled.".
typedef std::function<bool(size_t, size_t, const std::string&)> GatherProgress;

/// The Rank table (#703), shown as Compare Energies: ecce-si-export --rank
/// `format` for `calcs`.
bool rankTable(const std::vector<EcceURL>& calcs, const std::string& format,
               std::string& text, std::string& why,
               const GatherProgress& progress = GatherProgress(),
               const std::string& ref = std::string());

/// The QSAR descriptor table (#961) of `calcs`: ecce-qsar table with the
/// comma list of descriptor `groups` (counts, topology, models, charges, vsa,
/// qm), as CSV text.
bool qsarTable(const std::vector<EcceURL>& calcs, const std::string& groups,
               std::string& text, std::string& why,
               const GatherProgress& progress = GatherProgress());

#endif
