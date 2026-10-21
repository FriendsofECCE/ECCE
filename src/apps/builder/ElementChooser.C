#include <algorithm>
#include <set>

#include <wx/bitmap.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/tooltip.h>

#include "util/Event.H"
#include "util/EventDispatcher.H"
#include "util/Preferences.H"
#include "tdat/TPerTab.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxConfig.H"

#include "ElementChooser.H"
#include "ElementLayout.H"

using namespace ElementLayout;
using ecce::EventDispatcher;
using ecce::Event;

namespace {
const int MARGIN = 8;
const int COLS = 19;        // 18 table columns; the strips carry a label too
const double ROWS = 14.6;   // strips, table, f-block and legend, in tiles
const double GRID_TOP = 2.6;
const char *RECENT_KEY = "/ElementChooser/Recent";

wxColour mix(const wxColour& a, const wxColour& b, double t)
{
  return wxColour(a.Red() + (b.Red() - a.Red()) * t,
                  a.Green() + (b.Green() - a.Green()) * t,
                  a.Blue() + (b.Blue() - a.Blue()) * t);
}

wxColour rgbColour(unsigned rgb)
{
  return wxColour(rgb >> 16, (rgb >> 8) & 255, rgb & 255);
}
}


ElementChooser::ElementChooser(wxWindow *parent, const std::string& current)
  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxTAB_TRAVERSAL | wxFULL_REPAINT_ON_RESIZE),
    p_current(current), p_hover(0), p_searching(false)
{
  SetName("Elements");
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  p_search = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                            wxDefaultSize, wxTE_PROCESS_ENTER);
  p_search->SetHint(_("Search by symbol or name, e.g. Fe or iron"));
  p_search->SetName("ElementSearch");
  loadRecent();

  Bind(wxEVT_PAINT, &ElementChooser::OnPaint, this);
  Bind(wxEVT_SIZE, &ElementChooser::OnSize, this);
  Bind(wxEVT_MOTION, &ElementChooser::OnMotion, this);
  Bind(wxEVT_LEAVE_WINDOW, &ElementChooser::OnLeave, this);
  Bind(wxEVT_LEFT_UP, &ElementChooser::OnLeftUp, this);
  p_search->Bind(wxEVT_TEXT, &ElementChooser::OnText, this);
  p_search->Bind(wxEVT_TEXT_ENTER, &ElementChooser::OnEnter, this);
  SetMinSize(wxSize(COLS * 26 + 2 * MARGIN, 400));
  p_search->CallAfter([this] { p_search->SetFocus(); });
}


int ElementChooser::searchHeight() const
{
  return p_search->GetBestSize().y + 2 * MARGIN;
}


wxSize ElementChooser::DoGetBestSize() const
{
  const int p = FromDIP(34);
  return wxSize(COLS * p + 2 * MARGIN,
                searchHeight() + int(ROWS * p) + 2 * MARGIN);
}


void ElementChooser::OnSize(wxSizeEvent& e)
{
  p_search->SetSize(MARGIN, MARGIN, GetClientSize().x - 2 * MARGIN,
                    p_search->GetBestSize().y);
  Refresh();
  e.Skip();
}


void ElementChooser::loadRecent()
{
  ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
  std::vector<std::string> v;
  config->Read(RECENT_KEY, &v);
  TPerTab tpt;
  for (const std::string& s : v)
    if (tpt.atomicNumber(s) > 0 && p_recent.size() < 8) p_recent.push_back(s);
}


void ElementChooser::saveRecent() const
{
  ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
  config->Write(RECENT_KEY, p_recent);
  config->Flush();
}


std::vector<ElementChooser::Tile>
ElementChooser::layout(const wxSize& size, int *tileSize) const
{
  const int top = searchHeight();
  int p = std::min((size.x - 2 * MARGIN) / COLS,
                   int((size.y - top - MARGIN) / ROWS));
  if (p < 8) p = 8;
  if (tileSize) *tileSize = p;
  TPerTab tpt;
  std::vector<Tile> t;
  auto at = [&](double col, double row) {
    return wxRect(MARGIN + int(col * p), top + int(row * p), p, p);
  };
  const std::vector<std::string>& common = commonElements();
  for (size_t i = 0; i < p_recent.size(); i++)
    t.push_back({tpt.atomicNumber(p_recent[i]), 1, at(2 + i, 0)});
  for (size_t i = 0; i < common.size(); i++)
    t.push_back({tpt.atomicNumber(common[i]), 2, at(2 + i, 1)});
  for (int z = 1; z <= 118; z++) {
    Cell c = cellOf(z);
    //  The f-block follows the table after a gap of half a tile.
    const double row = GRID_TOP + (c.row >= F_ROW_LA ? c.row - 0.5 : c.row);
    t.push_back({z, 0, at(c.col, row)});
  }
  for (int w = 0; w < 2; w++) {
    Cell c = placeholderCell(w);
    t.push_back({-1 - w, 0, at(c.col, GRID_TOP + c.row)});
  }
  return t;
}


const ElementChooser::Tile *
ElementChooser::tileAt(const wxPoint& pt, const std::vector<Tile>& tiles) const
{
  for (const Tile& t : tiles)
    if (t.z > 0 && t.rect.Contains(pt)) return &t;
  return 0;
}


static void drawTile(wxDC& dc, const wxRect& r, const wxColour& fill,
                     const wxColour& border, const wxColour& text,
                     const wxString& sym, const wxString& num, int p,
                     bool placeholder)
{
  wxRect in = r;
  in.Deflate(1);
  dc.SetBrush(wxBrush(fill));
  dc.SetPen(wxPen(border, placeholder ? 1 : 2));
  dc.DrawRoundedRectangle(in, 3);
  dc.SetTextForeground(text);
  wxFont f = dc.GetFont();
  f.SetPixelSize(wxSize(0, std::max(8, int(p * (placeholder ? 0.24 : 0.38)))));
  f.SetWeight(placeholder ? wxFONTWEIGHT_NORMAL : wxFONTWEIGHT_BOLD);
  dc.SetFont(f);
  wxSize ts = dc.GetTextExtent(sym);
  dc.DrawText(sym, in.x + (in.width - ts.x) / 2,
              in.y + (in.height - ts.y) / 2 + (placeholder ? 0 : p / 8));
  if (!num.empty()) {
    f.SetPixelSize(wxSize(0, std::max(7, int(p * 0.2))));
    f.SetWeight(wxFONTWEIGHT_NORMAL);
    dc.SetFont(f);
    dc.DrawText(num, in.x + 3, in.y + 1);
  }
}


wxBitmap ElementChooser::render(const wxSize& size)
{
  wxBitmap bmp(size.x, size.y);
  wxMemoryDC dc(bmp);
  paintTo(dc);
  dc.SelectObject(wxNullBitmap);
  return bmp;
}


void ElementChooser::OnPaint(wxPaintEvent&)
{
  wxAutoBufferedPaintDC dc(this);
  paintTo(dc);
}


void ElementChooser::paintTo(wxDC& dc)
{
  const wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
  const wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
  const wxColour hi = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
  dc.SetBackground(wxBrush(bg));
  dc.Clear();
  dc.SetFont(GetFont());

  int p;
  std::vector<Tile> tiles = layout(GetClientSize(), &p);
  TPerTab tpt;
  Preferences prefs("PerTable");
  const std::set<int> matches(p_matches.begin(), p_matches.end());

  wxFont lf = GetFont();
  lf.SetPixelSize(wxSize(0, std::max(9, int(p * 0.3))));
  dc.SetFont(lf);
  dc.SetTextForeground(fg);
  const int top = searchHeight();
  dc.DrawText(_("Recent"), MARGIN, top + p / 4);
  dc.DrawText(_("Common"), MARGIN, top + p + p / 4);

  for (const Tile& t : tiles) {
    wxString sym, num;
    wxColour fill = bg, border, text = fg;
    const bool ph = t.z < 0;
    if (ph) {
      sym = t.z == -1 ? "57-71" : "89-103";
      border = rgbColour(categoryRGB(t.z == -1 ? Lanthanide : Actinide));
      fill = mix(bg, border, 0.25);
    } else {
      sym = tpt.atomicSymbol(t.z);
      if (t.kind == 0) num = wxString::Format("%d", t.z);
      std::string col;
      fill = prefs.getString(sym.ToStdString() + ".Color", col)
                 ? wxColour(ewxColor(col)) : wxColour(ewxColor(tpt.color(t.z)));
      border = rgbColour(categoryRGB(categoryOf(t.z)));
      text = darkTextOn(fill.Red(), fill.Green(), fill.Blue())
                 ? *wxBLACK : *wxWHITE;
      if (p_searching && !matches.count(t.z)) {
        fill = mix(fill, bg, 0.75);
        text = mix(text, bg, 0.6);
        border = mix(border, bg, 0.75);
      }
    }
    drawTile(dc, t.rect, fill, border, text, sym, num, p, ph);
    const bool mark = !ph && ((p_searching && matches.count(t.z)) ||
                              sym.ToStdString() == p_current);
    if (mark) {
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
      dc.SetPen(wxPen(hi, 3));
      dc.DrawRoundedRectangle(t.rect, 3);
    }
  }

  int x = MARGIN, y = top + int((GRID_TOP + 7 + 0.5 + 2 + 0.6) * p);
  dc.SetFont(lf);
  dc.SetTextForeground(fg);
  for (int c = 0; c < NumCategories; c++) {
    const wxString name = wxGetTranslation(categoryName((Category)c));
    const int w = dc.GetTextExtent(name).x + p / 3 + 5;
    if (x + w > GetClientSize().x - MARGIN) {
      x = MARGIN;
      y += int(p * 0.55);
    }
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(rgbColour(categoryRGB((Category)c)), 2));
    dc.DrawRoundedRectangle(x, y, p / 3, p / 3, 2);
    dc.DrawText(name, x + p / 3 + 5, y - 1);
    x += w + p / 4;
  }
}


void ElementChooser::OnMotion(wxMouseEvent& e)
{
  const Tile *t = tileAt(e.GetPosition(), layout(GetClientSize()));
  const int z = t ? t->z : 0;
  if (z == p_hover) return;
  p_hover = z;
  if (!z) { UnsetToolTip(); return; }
  TPerTab tpt;
  SetToolTip(wxString::Format("%s (%d), %s", tpt.name(z), z,
             wxGetTranslation(categoryName(categoryOf(z)))));
}


void ElementChooser::OnLeave(wxMouseEvent&) { p_hover = 0; }


void ElementChooser::OnLeftUp(wxMouseEvent& e)
{
  const Tile *t = tileAt(e.GetPosition(), layout(GetClientSize()));
  if (t) pick(TPerTab().atomicSymbol(t->z));
}


void ElementChooser::OnText(wxCommandEvent&)
{
  search(p_search->GetValue().ToStdString(), false);
}


void ElementChooser::OnEnter(wxCommandEvent&)
{
  search(p_search->GetValue().ToStdString(), true);
}


int ElementChooser::search(const std::string& text, bool pressEnter)
{
  if (p_search->GetValue().ToStdString() != text)
    p_search->ChangeValue(text);
  p_matches = ElementLayout::search(text);
  p_searching = !text.empty();
  Refresh();
  if (pressEnter && p_matches.size() == 1)
    pick(TPerTab().atomicSymbol(p_matches[0]));
  return (int)p_matches.size();
}


bool ElementChooser::pick(const std::string& symbol)
{
  if (TPerTab().atomicNumber(symbol) <= 0) return false;
  p_current = symbol;
  pushRecent(p_recent, symbol);
  saveRecent();
  Refresh();
  EventDispatcher::getDispatcher().publish(Event("ElementChanged", symbol));
  //  The popup holding this panel closes on a button event from its content.
  wxCommandEvent done(wxEVT_BUTTON, GetId());
  done.SetEventObject(this);
  GetEventHandler()->ProcessEvent(done);
  return true;
}
