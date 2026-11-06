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
    for (const string& name : {"O", "Ar", "Cr", "Cu", "Xe", "reordered"}) {
      vector<AufbauShell> shells;
      std::istringstream labels(name == "Cr"
          ? "1s 2s 2p 3s 3p 3d 4s 4p 5s"
          : name == "O" ? "1s 2s 2p 3s 3p"
          : name == "Ar" ? "1s 2s 2p 3s 3p 4s 4p"
          : "1s 2s 2p 3s 3p 4s 3d 4p 5s 4d 5p 6s 5d");
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
          if ((name == "O" && (label == "3s" || label == "3p")) ||
              (name == "Ar" && (label == "4s" || label == "4p")) ||
              ((name == "Cu" || name == "Cr") && label == "4p")) o.up = o.down = 0;
          if (name == "O" && label == "2p") o.down = k == 0;
          if ((name == "Cu" || name == "Cr") && label == "4s") o.down = 0;
          if (name == "Cr" && label == "3d") o.down = 0;
          if (name == "Cr" && (label == "4p" || label == "5s")) o.up = o.down = 0;
          s.electrons += o.up+o.down; s.orbs.push_back(o);
        }
        shells.push_back(s);
        if ((name == "O" && label == "3p") ||
            (name == "Ar" && label == "4p") ||
            (name == "Cu" && label == "4p")) break;
      }
      if (name == "reordered") std::swap(shells[5], shells[6]);
      canvas->setModel(name, shells);
      for (const wxSize& size : {wxSize(900,600), wxSize(500,400), wxSize(400,300), wxSize(320,240)}) {
        const string path = directory+"/"+name+"-"+std::to_string(size.x)+"x"+std::to_string(size.y);
        if (!canvas->savePng(path+".png", false, size.x, size.y)) result = 1;
        std::ofstream out(path+".txt");
        out << canvas->describe();
        if (size.x >= 400 && (name == "O" || name == "Ar" || name == "Cr" || name == "Cu")) {
          vector<AufbauShell> scaled = shells;
          const vector<double> energies = name == "Cr"
              ? vector<double>{-3200, -300, -250, -74, -46, -4.93, -4.65, -0.68, 0.25}
              : vector<double>{-3200, -300, -250, -30, -12, 2, 4, 8};
          for (size_t j = 0; j < scaled.size(); ++j) {
            AufbauShell& s = scaled[j];
            s.mean = energies[j];
            s.core = j < (name == "O" ? 1 : 3);
            for (size_t k = 0; k < s.orbs.size(); ++k)
              s.orbs[k].eV = s.mean + (name == "O" && s.label == "2p" && k > 0 ? 5 : 0);
            if (name == "O" && s.label == "2p") s.mean += 10.0/3;
          }
          canvas->setModel(name, scaled);
          if (!canvas->savePng(path+"-scale.png", true, size.x, size.y)) result = 1;
          std::ofstream(path+"-scale.txt") << canvas->describe();
          canvas->setModel(name, shells);
        }
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
