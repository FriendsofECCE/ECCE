// Ctrl+click adds a row to a WxGridView's selection and a second Ctrl+click
// takes it out; Shift+click extends to a range (#290).  Real wxMouseEvents are
// sent to the grid window; run under a display (xvfb-run).
#include <cstdio>
#include <cstdlib>
#include <set>

#include "wx/wx.h"
#include "wx/generic/grid.h"

#include "wxgui/GridElement.H"
#include "wxgui/GridModel.H"
#include "wxgui/WxGridView.H"
#include "wxgui/ewxGrid.H"

static int failures = 0;

static void expect(const char* what, WxGridView* v, std::set<int> want)
{
  std::set<int> got = v->getSelectedRows();
  bool ok = (got == want);
  std::printf("%s %s:", ok ? "ok  " : "FAIL", what);
  for (int r : got) std::printf(" %d", r);
  std::printf("\n");
  if (!ok) failures++;
}

static void click(WxGridView* v, int row, bool ctrl, bool shift)
{
  wxWindow* w = v->getGrid()->GetGridWindow();
  int y = v->getRowHeight() * row + v->getRowHeight() / 2;
  for (wxEventType t : {wxEVT_LEFT_DOWN, wxEVT_LEFT_UP}) {
    wxMouseEvent e(t);
    e.SetEventObject(w);
    e.SetPosition(wxPoint(20, y));
    e.SetControlDown(ctrl);
    e.SetShiftDown(shift);
    e.SetLeftDown(t == wxEVT_LEFT_DOWN);
    w->GetEventHandler()->ProcessEvent(e);
    wxYield();
  }
}

class App : public wxApp
{
 public:
  bool OnInit() override
  {
    wxFrame* f = new wxFrame(NULL, wxID_ANY, "t", wxDefaultPosition,
                             wxSize(300, 300));
    WxGridView* v = new WxGridView(f, wxID_ANY);
    GridModel* m = new GridModel();
    m->AppendCols(1);
    m->AppendRows(6);
    GridProperty prop(GridProperty::PT_CDATA);
    for (int r = 0; r < 6; r++)
      m->SetValueAsGridElement(r, 0,
                               new GridElement(wxString::Format("r%d", r), &prop));
    v->setModel(m);
    f->Show();
    p_view = v;
    p_frame = f;
    return true;
  }

  int OnRun() override
  {
    WxGridView* v = p_view;
    wxYield();
    click(v, 1, false, false);  expect("click 1", v, {1});
    click(v, 3, true, false);   expect("ctrl+click 3 adds", v, {1, 3});
    click(v, 4, true, false);   expect("ctrl+click 4 adds", v, {1, 3, 4});
    click(v, 3, true, false);   expect("ctrl+click 3 removes", v, {1, 4});
    click(v, 0, false, false);  expect("click 0 replaces", v, {0});
    click(v, 3, false, true);   expect("shift+click 3 ranges", v, {0, 1, 2, 3});
    p_frame->Destroy();
    wxYield();
    std::printf("failures=%d\n", failures);
    std::fflush(stdout);
    // libwx's exit-time teardown crashes with no real session; the result is out.
    std::_Exit(failures ? 1 : 0);
  }

 private:
  WxGridView* p_view = 0;
  wxFrame* p_frame = 0;
};

wxIMPLEMENT_APP(App);
