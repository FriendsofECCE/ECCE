#ifndef ORCARIFITTING_HH
#define ORCARIFITTING_HH

#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "util/Ecce.H"
#include "util/TypedFile.H"

namespace OrcaRIFitting {

struct Settings {
  std::string mode;
  std::string auxiliary;
};

inline bool active(const std::string& mode)
{
  return mode == "RIJCOSX" || mode == "RIJONX";
}

inline Settings defaults()
{
  Settings result;
  std::ifstream file(std::string(Ecce::ecceHome()) +
                     "/scripts/parsers/orca-ri.defaults");
  std::string line, key, value;
  while (std::getline(file, line)) {
    std::istringstream fields(line);
    if (!(fields >> key >> value)) continue;
    if (key == "Mode") result.mode = value;
    if (key == "Auxiliary") result.auxiliary = value;
  }
  return result;
}

inline std::string upper(std::string text)
{
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return text;
}

inline Settings readFile(std::istream& file, bool output)
{
  Settings result;
  std::string line;
  // Only the initial input echo and SCF settings describe this calculation;
  // later optimisation steps and compound jobs can carry other settings.
  for (size_t count = 0; count < 4096 && std::getline(file, line); ++count) {
    if (output && line.find("SCF CONVERGED AFTER") != std::string::npos) break;
    const std::string marker = "Your calculation utilizes the auxiliary basis:";
    const size_t basis = line.find(marker);
    if (output && basis != std::string::npos) {
      std::istringstream fields(line.substr(basis + marker.size()));
      fields >> result.auxiliary;
    }
    if (output && line.find("RI-approximation to the Coulomb term is turned on")
                      != std::string::npos)
      result.mode = "RIJONX";
    if (output && line.find("RIJ-COSX (HFX calculated with COS-X)")
                      != std::string::npos &&
        line.find("on", line.find("....")) != std::string::npos)
      result.mode = "RIJCOSX";

    if (output) {
      const size_t echo = line.find('>');
      if (line.empty() || line[0] != '|' || echo == std::string::npos) continue;
      line.erase(0, echo + 1);
    }
    const size_t begin = line.find_first_not_of(" \t\r");
    if (begin == std::string::npos || line[begin] != '!') continue;
    line.erase(0, begin + 1);
    const size_t comment = line.find('#');
    if (comment != std::string::npos) line.erase(comment);
    std::istringstream tokens(line);
    std::string token;
    while (tokens >> token) {
      const std::string name = upper(token);
      if (name == "RIJCOSX" || name == "RIJONX") result.mode = name;
      else if (name == "NORI") result.mode = "None";
      else if (name == "DEF2/J") result.auxiliary = "def2/J";
      else if (name == "DEF2/JK") result.auxiliary = "def2/JK";
      else if (name == "SARC/J") result.auxiliary = "SARC/J";
    }
  }
  return result;
}

inline Settings storedFile(const ICalculation *calc, JCode::CodeFileType type)
{
  TypedFile file;
  calc->getDataFile(type, file);
  if (file.name().empty()) return Settings();
  EcceURL url = calc->getDataFileUrl(type, file.name());
  for (const EcceURL& found : calc->getDataFiles(type)) {
    if (found.getFilePathTail() == file.name()) {
      url = found;
      break;
    }
  }
  std::unique_ptr<EDSI> edsi(EDSIFactory::getEDSI(url));
  if (!edsi) return Settings();
  // Summaries need the header, never the potentially very large result body.
  std::stringstream prefix;
  return edsi->getDataSubSet(prefix, 0, 256 * 1024) ?
      readFile(prefix, type == JCode::PRIMARY_OUTPUT) : Settings();
}

inline Settings resolve(const ICalculation *calc, GUIValues *params)
{
  Settings result;
  const JCode *code = calc ? calc->application() : 0;
  if (!code || code->name() != "ORCA") return result;
  static const Settings generatorDefaults = defaults();
  GUIValue *mode = params ? params->get("ES.Theory.SCF.RIMode") : 0;
  GUIValue *legacy = params ? params->get("ES.Theory.SCF.UseRIJCOSX") : 0;
  GUIValue *aux = params ? params->get("ES.Theory.SCF.RIJCOSXAuxBasis") : 0;
  if (mode || legacy) {
    result.mode = mode ? mode->getValueAsString() :
                  legacy->getValueAsString() == "1" ? "RIJCOSX" : "None";
    if (active(result.mode)) {
      result.auxiliary = aux ? aux->getValueAsString() : std::string();
      if (result.auxiliary.empty()) result.auxiliary = generatorDefaults.auxiliary;
    }
  } else {
    // Imported results may predate preservation of RI in SetupParams.
    // Prefer what actually ran, including ORCA's implicit RI defaults.
    result = storedFile(calc, JCode::PRIMARY_OUTPUT);
    if (result.mode.empty()) result = storedFile(calc, JCode::PRIMARY_INPUT);
    if (result.mode.empty()) result.mode = generatorDefaults.mode;
  }
  if (!active(result.mode)) {
    result.mode.clear();
    result.auxiliary.clear();
  }
  return result;
}

}

#endif
