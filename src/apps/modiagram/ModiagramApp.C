/**
 * @file
 *
 * A molecular orbital correlation diagram, on its own.
 *
 * WHY THIS EXISTS AS A PROGRAM.  The diagram is a teaching object
 * before it is a results viewer: the thing a lecturer draws on a board
 * and a student reproduces in an exam.  Everything it needs -- the
 * point group, the orbits, the projection operators, the symmetry
 * orbitals, the layout -- is already free of the rest of ECCE.  It
 * talks to no data server, publishes no messages and opens no
 * calculation, so requiring someone to install and start the whole
 * application to see one is a packaging decision, not a technical one.
 *
 * It reads the same spec file tools/modiagram uses, documented in
 * tools/modiagram/dump.C, and an ordinary XYZ file -- for which it
 * asks MOPAC for a quick spectrum, via the same helper the test suite
 * uses, so there is one implementation of that reading and not two.
 */
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/utils.h>

#include "tdat/MoSpec.H"
#include "MoDiagramCanvas.H"

using std::string;


/** Where the character tables live, which is all the data it needs. */
static string tablesPath()
{
  const char* home = getenv("ECCE_HOME");
  if (home != 0 && *home != '\0') {
    return string(home) + "/data/client/config/CharacterTables";
  }
  return "CharacterTables";
}


class ModiagramFrame : public wxFrame
{
  public:

    ModiagramFrame()
      : wxFrame(NULL, wxID_ANY, wxT("MO Diagram"),
                wxDefaultPosition, wxSize(1400, 950))
    {
      wxMenu *file = new wxMenu;
      file->Append(wxID_OPEN, wxT("&Open...\tCtrl+O"));
      file->Append(wxID_SAVEAS, wxT("&Save Image...\tCtrl+S"));
      file->AppendSeparator();
      file->Append(wxID_EXIT, wxT("&Quit\tCtrl+Q"));

      wxMenu *help = new wxMenu;
      help->Append(wxID_ABOUT, wxT("&About"));

      wxMenuBar *bar = new wxMenuBar;
      bar->Append(file, wxT("&File"));
      bar->Append(help, wxT("&Help"));
      SetMenuBar(bar);

      CreateStatusBar();
      SetStatusText(wxT("Open a molecule to draw its correlation diagram"));

      p_canvas = new MoDiagramCanvas(this);

      Bind(wxEVT_MENU, &ModiagramFrame::onOpen,  this, wxID_OPEN);
      Bind(wxEVT_MENU, &ModiagramFrame::onSave,  this, wxID_SAVEAS);
      Bind(wxEVT_MENU, &ModiagramFrame::onQuit,  this, wxID_EXIT);
      Bind(wxEVT_MENU, &ModiagramFrame::onAbout, this, wxID_ABOUT);
    }

    /**
     * Draw a molecule.
     *
     * An XYZ file has no orbitals in it, so one is computed: the
     * helper runs MOPAC and writes the same spec file this reads.
     * Without MOPAC installed that is simply not available, and the
     * program says so rather than drawing something it made up.
     */
    bool show(const wxString& path)
    {
      wxString spec = path;
      wxString temporary;

      if (path.Lower().EndsWith(wxT(".xyz"))) {
        temporary = wxFileName::CreateTempFileName(wxT("modiagram"));
        const char* home = getenv("ECCE_HOME");
        wxString helper = (home != 0 && *home != '\0')
            ? wxString(home, wxConvUTF8) + wxT("/scripts/modiagram-mopac")
            : wxT("modiagram-mopac");

        SetStatusText(wxT("Running MOPAC..."));
        wxString command = helper + wxT(" \"") + path + wxT("\" \"") +
                           temporary + wxT("\"");
        if (wxExecute(command, wxEXEC_SYNC) != 0) {
          wxLogError("%s", "Could not get a spectrum for that molecule.\n\n"
                     "An XYZ file has no orbital energies in it, so MOPAC "
                     "is run to compute them.  Check that mopac is "
                     "installed and on the PATH.");
          ::wxRemoveFile(temporary);
          return false;
        }
        spec = temporary;
      }

      MoModel model;
      const bool fragments = buildModel(tablesPath().c_str(),
                                        (const char*)spec.mb_str(), model);

      if (!temporary.empty()) ::wxRemoveFile(temporary);

      if (model.centre.levels.empty()) {
        wxLogError("%s", "No orbitals were found in that file.");
        return false;
      }

      p_canvas->setGroup(model.group);
      p_canvas->setFormula(MoDiagram::formula(model.elements, model.charge));
      p_canvas->setDiagram(model.left, model.centre, model.right,
                           model.links, fragments, model.note);
      p_canvas->Refresh();

      SetTitle(wxFileName(path).GetName() + wxT(" - MO Diagram"));
      //  Spelled as a chemist writes it, not as the generator's
      //  tables do: Td, C2v, D-infinity-h.
      string pretty = model.group;
      for (string::size_type i = 0; i < pretty.size(); i++) {
        pretty[i] = (i == 0) ? toupper(pretty[i]) : tolower(pretty[i]);
      }
      if (pretty == "Dinfh") pretty = "D(inf)h";
      if (pretty == "Cinfv") pretty = "C(inf)v";

      SetStatusText(wxString::Format(
          wxT("%s, %d molecular orbital levels, %d correlations"),
          wxString(pretty.c_str(), wxConvUTF8).c_str(),
          (int)model.centre.levels.size(),
          (int)model.links.size()));
      p_path = path;
      return true;
    }

  private:

    void onOpen(wxCommandEvent&)
    {
      wxFileDialog dialog(this, wxT("Open a molecule"), wxT(""), wxT(""),
                          wxT("Molecules (*.xyz;*.mol)|*.xyz;*.mol|"
                              "XYZ files (*.xyz)|*.xyz|"
                              "Diagram specs (*.mol)|*.mol|"
                              "All files (*)|*"),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST);
      if (dialog.ShowModal() == wxID_OK) show(dialog.GetPath());
    }

    void onSave(wxCommandEvent&)
    {
      wxFileDialog dialog(this, wxT("Save the diagram"), wxT(""),
                          wxFileName(p_path).GetName() + wxT(".png"),
                          wxT("PNG images (*.png)|*.png"),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if (dialog.ShowModal() != wxID_OK) return;

      //  Painted onto a bitmap through the same paintOnto() the window
      //  uses, so what is saved is what is on the screen.
      const wxSize size = p_canvas->GetClientSize();
      wxBitmap bitmap(size.x, size.y, 24);
      wxMemoryDC dc(bitmap);
      dc.SetBackground(*wxWHITE_BRUSH);
      dc.Clear();
      p_canvas->paintOnto(dc, size);
      dc.SelectObject(wxNullBitmap);

      if (!bitmap.SaveFile(dialog.GetPath(), wxBITMAP_TYPE_PNG)) {
        wxLogError("%s", "Could not write that image.");
      } else {
        SetStatusText(wxT("Saved ") + dialog.GetPath());
      }
    }

    void onQuit(wxCommandEvent&) { Close(true); }

    void onAbout(wxCommandEvent&)
    {
      wxMessageBox(
          wxT("A qualitative molecular orbital correlation diagram.\n\n"
              "The fragment orbitals, the terminal-atom symmetry orbitals "
              "and the molecular orbitals between them, correlated by "
              "symmetry and placed by the calculation's own energies.\n\n"
              "Part of ECCE."),
          wxT("About MO Diagram"), wxOK | wxICON_INFORMATION, this);
    }

    MoDiagramCanvas *p_canvas;
    wxString         p_path;
};


class ModiagramApp : public wxApp
{
  public:
    virtual bool OnInit()
    {
      wxInitAllImageHandlers();

      ModiagramFrame *frame = new ModiagramFrame;
      frame->Show(true);

      if (argc > 1) frame->show(argv[1]);
      return true;
    }
};

IMPLEMENT_APP(ModiagramApp)
