// Exercise the real canvas with synthetic spectra, including interleaved 4s/3d.
#include <fstream>
#include <sstream>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/image.h>
#include "MoAufbauCanvas.H"

static int result = 0;

class LayoutApp : public wxApp {
public:
  bool OnInit() override {
    wxInitAllImageHandlers();
    wxFrame *frame = new wxFrame(0, wxID_ANY, "Aufbau layout");
    MoAufbauCanvas *canvas = new MoAufbauCanvas(frame);
    const string directory = argv[1].ToStdString();
    for (const string& name : {"O", "Ar", "Cu", "Xe", "reordered"}) {
      vector<AufbauShell> shells;
      std::istringstream labels("1s 2s 2p 3s 3p 4s 3d 4p 5s 4d 5p 6s 5d");
      string label;
      int orbital = 0;
      while (labels >> label) {
        AufbauShell s;
        s.label = label; s.n = label[0]-'0'; s.l = aufbau::lOfLabel(label);
        s.mean = (double)shells.size()*10-200; s.core = shells.size() < 3;
        s.electrons = 0;
        for (int k = 0; k < 2*s.l+1; ++k) {
          AufbauOrbital o; o.index = orbital++; o.eV = s.mean;
          o.up = o.down = label != "6s" && label != "5d";
          if ((name == "O" && label == "3s") ||
              (name == "Ar" && (label == "4s" || label == "3d")) ||
              (name == "Cu" && label == "4p")) o.up = o.down = 0;
          if (name == "O" && label == "2p") o.down = k == 0;
          if (name == "Cu" && label == "4s") o.down = 0;
          s.electrons += o.up+o.down; s.orbs.push_back(o);
        }
        shells.push_back(s);
        if ((name == "O" && label == "3s") ||
            (name == "Ar" && label == "3d") ||
            (name == "Cu" && label == "4p")) break;
      }
      if (name == "reordered") std::swap(shells[5], shells[6]);
      canvas->setModel(name, shells);
      for (const wxSize& size : {wxSize(900,600), wxSize(500,400), wxSize(320,240)}) {
        const string path = directory+"/"+name+"-"+std::to_string(size.x)+"x"+std::to_string(size.y);
        if (!canvas->savePng(path+".png", false, size.x, size.y)) result = 1;
        std::ofstream out(path+".txt");
        out << canvas->describe();
      }
    }
    delete frame;
    return false;
  }
};
wxIMPLEMENT_APP_NO_MAIN(LayoutApp);
int main(int argc, char **argv) {
  wxEntry(argc, argv);
  return result;
}
