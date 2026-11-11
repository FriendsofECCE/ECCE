#include <cstdio>
#include <fstream>
#include <iostream>
  using std::cerr;
  using std::endl;
  using std::ends;
  using std::ifstream;
  using std::ofstream;
#include <strstream>
  using std::ostrstream;
#include <unistd.h> // for getcwd()

#include "util/Ecce.H"
#include "util/EcceMap.H"
#include "util/SDirectory.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"
#include "util/STLUtil.H"

#include "tdat/ESPConstraintModel.H"
#include "tdat/Fragment.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"
#include "tdat/GUIValues.H"
#include "tdat/TTheory.H"

#include "dsm/TGBSConfig.H"
#include "dsm/ESPConstraintModelXMLizer.H"
#include "dsm/GeomConstraintModelXMLizer.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"

#include "CalcEd.H"
#include "dsm/QstStructures.H"
#include "dsm/DeckWriter.H"


void CalcEd::setMatchTooltip()
{
  const wxString tip = wxString::FromUTF8(p_qstDetail.c_str());
  p_feedback->SetToolTip(tip);
  for (wxWindowList::iterator it = p_feedback->GetChildren().begin();
       it != p_feedback->GetChildren().end(); ++it)
    (*it)->SetToolTip(tip);
}


// The deck itself is made by DeckWriter, which the series command shares.
bool CalcEd::input_controller(const bool& saveParamFlag,
                              const bool& useExpCoeff, string& message)
{
  DeckWriter::Options opt;
  opt.useExpCoeff = useExpCoeff;
  opt.keepFiles = saveParamFlag;
  opt.openShells = getOpenShells();
  opt.stage = p_saveStage;
  DeckWriter::Result r = DeckWriter::generate(p_iCalc, opt);

  p_qstStatus = r.qstStatus;
  p_qstDetail = r.qstDetail;
  p_qstTable = r.qstTable;
  p_inputGenWarnings = r.warnings;
  if (!r.fullMoleculeWarning.empty())
    p_feedback->setMessage(r.fullMoleculeWarning, WxFeedback::WARNING);
  message = r.message;
  if (saveParamFlag) {
    p_keptGeneratorDir = r.keptDir;
    cerr << "CalcEd: generator files kept in " << p_keptGeneratorDir << endl;
  }
  return r.ok;
}
