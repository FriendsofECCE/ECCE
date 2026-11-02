#ifndef SIEXPORT_HH
#define SIEXPORT_HH

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

#endif
