#include <fstream>
  using std::ifstream;

#include "util/Ecce.H"
#include "util/StringConverter.H"

#include "wxgui/ewxDialog.H"
#include "wxgui/ewxScrolledWindow.H"

#include "wxgui/ewxImageList.H"

ewxImageList::ewxImageList()
  : wxImageList()
{

}


ewxImageList::ewxImageList(int width, int height,
                           const bool mask, int initialCount)
  : wxImageList()
{
  Create(width, height, mask, initialCount);
}


bool ewxImageList::Create(int width, int height,
                          const bool mask, int initialCount)
{
  if (!wxImageList::Create(width, height, mask, initialCount)) {
    wxFAIL_MSG( wxT("ewxImageList creation failed") );
    return false;
  }

  return true;
}


ewxImageList::~ewxImageList()
{

}


bool ewxImageList::readPixmap(const string& file, string& decLine, 
                              int& width, int& height, int& colors,
                              vector<string>& pixDefLines,
                              vector<string>& pixDataLines)
{
  bool ret = false;

  pixDefLines.clear();
  pixDataLines.clear();

  string pixmapPath;

  if (file[0] == '/') {
    pixmapPath = file;
  } else {
    pixmapPath = Ecce::ecceDataPath();
    pixmapPath.append("/client/pixmaps/");
    pixmapPath.append(file);
  }

  ifstream ifs(pixmapPath.c_str()); 

  if (ifs) {
    char buf[256];
    int it;

    ifs.getline(buf, 255);
    while (!ifs.eof() && strncmp(buf, "static char", 11)!=0)
      ifs.getline(buf, 255);

    if (!ifs.eof()) {
      decLine = buf; 
      ifs.getline(buf, 255);

      // possible comment line 
      if (!ifs.eof() && strncmp(buf, "/*", 2)==0)
        ifs.getline(buf, 255);

      if (!ifs.eof()) {
        if (sscanf(&buf[1], "%d %d %d", &width, &height, &colors) == 3) {
          for (it=0; it<colors && !ifs.eof(); it++) {
            ifs.getline(buf, 255);
            pixDefLines.push_back(buf);
          }

          ifs.getline(buf, 255);
          // possible comment line 
          if (!ifs.eof() && strncmp(buf, "/*", 2)==0)
            ifs.getline(buf, 255);
          pixDataLines.push_back(buf);

          for (it=1; it<height && !ifs.eof(); it++) {
            ifs.getline(buf, 255);
            pixDataLines.push_back(buf);
          }
          
          if (pixDataLines[height-1].length() == width+2)
            pixDataLines[height-1].append("};");

          ret = !ifs.eof();
        }
      }
    }
  }

  return ret;
}


ewxDialog* ewxImageList::debug(wxWindow *parent) const
{
  ewxDialog *dialog = new ewxDialog(parent, wxID_ANY,
          "Debug ewxImageList", wxDefaultPosition, wxDefaultSize,
          wxCAPTION|wxCLOSE_BOX|wxMAXIMIZE_BOX|wxRESIZE_BORDER);
  dialog->SetSizer(new wxBoxSizer(wxVERTICAL));

  ewxScrolledWindow *win = new ewxScrolledWindow(dialog);
  dialog->GetSizer()->Add(win, 1, wxGROW|wxALL, 5);

  wxFlexGridSizer *sizer = new wxFlexGridSizer(0,11,0,0);
  win->SetSizer(sizer);

  sizer->AddSpacer(1);
  for (int i = 0; i < 10; i++) {
    sizer->Add(new wxStaticText(win, wxID_ANY,
                                StringConverter::toString(i)));
  }
  for (int i = 0; i < GetImageCount(); i++) {
    if (i % 10 == 0)
      sizer->Add(new wxStaticText(win, wxID_ANY,
                                  StringConverter::toString(i)));
    sizer->Add(new wxStaticBitmap(win, wxID_ANY, GetBitmap(i)));
  }
  dialog->Fit();
  return dialog;
}

