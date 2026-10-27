/**
 * @file
 *
 * The Progress box of the calculation summary (#284).
 */
#include <algorithm>
#include <cmath>
#include <ctime>

#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/stattext.h>

#include "tdat/VibSpectrum.H"
#include "wxgui/PlotStyle.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxSelectableText.H"
#include "wxgui/ewxThemeColours.H"

#include "ProgressBox.H"

using std::string;
using std::vector;

static const char *CHECK = "\xe2\x9c\x93";   // U+2713
static const char *CROSS = "\xe2\x9c\x97";   // U+2717
static const char *OPEN = "\xe2\x96\xbe";    // U+25BE, folded down
static const char *SHUT = "\xe2\x96\xb8";    // U+25B8, folded
static const char *WARN = "\xe2\x9a\xa0";    // U+26A0
static const char *INIFILE = "wxorganizer.ini";
static const char *FOLDKEY = "Progress/Collapsed";
static const char *MIDDOT = " \xc2\xb7 ";

static wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }

static const char *ACTIONS[3] = {
  "Reconnect", "Check Job on Machine", "Show Output So Far" };


ProgressBox::ProgressBox(wxWindow *parent)
  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxFULL_REPAINT_ON_RESIZE, "progressBox"),
    p_received(0), p_finished(false), p_completed(false), p_lastHeard(0),
    p_collapsed(false), p_timer(this), p_paints(0)
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  Bind(wxEVT_PAINT, &ProgressBox::OnPaint, this);
  Bind(wxEVT_TIMER, &ProgressBox::OnTimer, this);
  Bind(wxEVT_SIZE, &ProgressBox::OnSize, this);
  Bind(wxEVT_LEFT_DOWN, &ProgressBox::OnClick, this);
  Bind(wxEVT_CONTEXT_MENU, &ProgressBox::OnContextMenu, this);
  ewxConfig::getConfig(INIFILE)->Read(FOLDKEY, &p_collapsed);
  p_energyLabel = new wxStaticText(this, wxID_ANY, "");
  ewxMakeSelectable(p_energyLabel);
  p_energyLabel->SetSize(0, 0, 0, 0);
  p_energyLabel->Show(true);
  p_energyLabel->Bind(wxEVT_CONTEXT_MENU, &ProgressBox::OnContextMenu, this);
  for (int i = 0; i < 3; i++) {
    p_buttons[i] = new wxButton(this, wxID_ANY, ACTIONS[i]);
    p_buttons[i]->SetSize(0, 0, 0, 0);
    p_buttons[i]->Show(true);
    p_buttons[i]->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent&) {
      if (p_action) p_action(i);
    });
  }
}


ProgressBox::~ProgressBox()
{
  p_timer.Stop();
}


bool ProgressBox::set(const string& text, double received, bool finished,
                      bool completed, double lastHeard,
                      const vector<string>& checkLines)
{
  if (text != p_text) {
    p_text = text;
    p_progress.parse(text);
  }
  p_received = received;
  p_finished = finished;
  p_completed = completed;
  p_lastHeard = lastHeard;
  p_check = checkLines;

  //  A running job's box ages by the second ("updated n s ago", the time
  //  left, how long out of contact); only the box is repainted for it.
  if (!finished && wanted()) {
    if (!p_timer.IsRunning()) p_timer.Start(1000);
  } else {
    p_timer.Stop();
  }

  const wxSize want = layout(0);
  const bool changed = want != GetMinSize();
  if (changed) SetMinSize(want);
  Refresh(false);
  //  A box that got shorter leaves its old lines in the area it vacates
  //  until the parent repaints, under whatever is laid out there next.
  if (changed && GetParent()) GetParent()->Refresh();
  return changed;
}


void ProgressBox::setCollapsed(bool collapsed)
{
  if (collapsed == p_collapsed) return;
  p_collapsed = collapsed;
  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  config->Write(FOLDKEY, p_collapsed);
  config->Flush();
  const wxSize want = layout(0);
  SetMinSize(want);
  Refresh(false);
  if (GetParent()) GetParent()->Refresh();
  if (p_relayout) p_relayout();
}


void ProgressBox::OnClick(wxMouseEvent& event)
{
  if (event.GetY() < headingHeight()) setCollapsed(!p_collapsed);
  else event.Skip();
}


double ProgressBox::age() const
{
  const double now = (double)time(0);
  if (p_queued && p_progress.empty())
    return p_checked > 0 ? std::max(0.0, now - p_checked) : -1;
  const double from = p_received > 0 ? p_received : p_progress.at();
  return from > 0 ? std::max(0.0, now - from) : -1;
}


vector<JobProgress::Block> ProgressBox::blocks() const
{
  if (p_queued && !p_finished && p_progress.empty())
    return vector<JobProgress::Block>(
        1, JobProgress::queued(p_submitted, p_jobId, (double)time(0)));
  return p_progress.running(std::max(0.0, age()), p_finished);
}


// What a finished job came to, ending with its final energy.
vector<string> ProgressBox::outcome() const
{
  vector<string> out = p_progress.summary(p_completed);
  if (!p_finalEnergy.empty()) out.push_back(p_finalEnergy);
  return out;
}


string ProgressBox::copyText(int what) const
{
  if (what == 0) return p_finalValue;
  if (what == 1)
    return p_finalValue.empty() || p_finalUnits.empty()
               ? p_finalValue : p_finalValue + " " + p_finalUnits;
  string all;
  for (const string& line : describe()) all += line + "\n";
  return all;
}


void ProgressBox::OnContextMenu(wxContextMenuEvent&)
{
  wxMenu menu;
  const int ids[3] = { wxNewId(), wxNewId(), wxNewId() };
  if (!p_finalValue.empty()) {
    menu.Append(ids[0], "Copy energy");
    menu.Append(ids[1], "Copy energy with unit");
  }
  menu.Append(ids[2], "Copy all");
  menu.Bind(wxEVT_MENU, [this, ids](wxCommandEvent& e) {
    for (int i = 0; i < 3; i++)
      if (e.GetId() == ids[i] && wxTheClipboard->Open()) {
        wxTheClipboard->SetData(
            new wxTextDataObject(wxString::FromUTF8(copyText(i).c_str())));
        wxTheClipboard->Close();
      }
  });
  PopupMenu(&menu);
}


// The one line of the folded box, after its heading.
string ProgressBox::foldedText() const
{
  if (outOfContact())
    return JobProgress::noContactText((double)time(0) - p_lastHeard);
  if (p_finished) {
    vector<string> s = outcome();
    return s.empty() ? string() : s[0];
  }
  if (p_queued && p_progress.empty()) {
    const vector<JobProgress::Block> q = blocks();
    string line = q.empty() || q[0].lines.empty() ? string() : q[0].lines[0];
    const double a = age();
    if (a >= 0) line += (line.empty() ? "" : MIDDOT) + JobProgress::updatedText(a);
    return line;
  }
  string line = p_progress.shortLine();
  const double a = age();
  if (a >= 0) line += (line.empty() ? "" : MIDDOT) + JobProgress::updatedText(a);
  return line;
}


int ProgressBox::headingHeight() const
{
  wxClientDC dc(const_cast<ProgressBox*>(this));
  wxFont bold = GetFont();
  bold.MakeBold();
  dc.SetFont(bold);
  return dc.GetCharHeight() + FromDIP(2);
}


// The section and the quantity a plot shows.
static string plotCaption(const JobProgress::Block& b)
{
  return b.title + ": " + b.plotLabel +
         (b.plotUnit.empty() ? "" : " (" + b.plotUnit + ")") +
         (b.title == "Optimisation" ? ", all points" : "");
}


vector<string> ProgressBox::describe() const
{
  vector<string> out;
  if (!wanted()) return out;
  out.push_back(string("heading ") + (p_collapsed ? "folded" : "open"));
  if (outOfContact()) {
    out.push_back("contact lost, last heard " +
                  JobProgress::lastHeardText(p_lastHeard, (double)time(0)));
    int shown = 0;
    // Unused buttons stay shown at zero size (GTK), so size counts.
    for (int i = 0; i < 3; i++)
      shown += p_buttons[i]->IsShown() && p_buttons[i]->GetSize().x > 0;
    out.push_back("buttons " + std::to_string(shown));
  }
  if (p_collapsed) {
    out.push_back("folded: " + foldedText());
    return out;
  }
  const string grey = outOfContact() ? "(grey) " : "";
  for (const JobProgress::Block& b : blocks()) {
    for (const string& l : b.lines) out.push_back(grey + b.title + ": " + l);
    for (const JobProgress::Criterion& c : b.criteria)
      out.push_back("  " + c.label + " " + c.value + " " +
                    (c.met ? CHECK : CROSS));
    if (!b.alert.empty())
      out.push_back("  " + b.alert + (b.alertBad ? " (unexpected)" : ""));
    if (b.bar) {
      char f[32];
      snprintf(f, sizeof f, "  bar %.2f", b.fraction);
      out.push_back(f);
    }
    if (!b.plot.empty())
      out.push_back("  plot " + plotCaption(b) + " (" +
                    std::to_string(b.plot.size()) + " points)");
  }
  for (const string& g : p_geom) out.push_back("geom " + g);
  if (p_finished)
    for (const string& s : outcome())
      out.push_back("summary: " + s);
  else if (!outOfContact() && age() >= 0)
    out.push_back(JobProgress::updatedText(age()));
  for (const string& l : p_check) out.push_back("check: " + l);
  return out;
}


// 10 to the power n, in superscript digits: the y tick of a log plot.
static wxString powerOfTen(int n)
{
  static const char *digit[] = { "\xe2\x81\xb0", "\xc2\xb9", "\xc2\xb2", "\xc2\xb3",
      "\xe2\x81\xb4", "\xe2\x81\xb5", "\xe2\x81\xb6", "\xe2\x81\xb7",
      "\xe2\x81\xb8", "\xe2\x81\xb9" };
  string s = "10";
  if (n < 0) s += "\xe2\x81\xbb";   // superscript minus
  for (char c : std::to_string(std::abs(n))) s += digit[c - '0'];
  return U(s);
}


// A small line plot, framed, with its label under it. The x axis runs
// 1..xEnd (cycles, steps) or 0..xEnd (ps) with a few ticks.  A linear
// plot's y axis shows the extremes; a
// log plot's (blk.curves, log10 of |value|) shows powers of ten and draws
// each curve's convergence limit as a dashed line, so the curve can be read
// as well as seen.
static void drawPlot(wxDC& dc, const wxRect& r, const JobProgress::Block& blk,
                     const string& label, const PlotPalette& pal,
                     const wxFont& small, const wxColour& series)
{
  const bool logAxis = blk.plotLog && !blk.curves.empty();
  vector<const vector<double>*> lines;
  vector<wxColour> colours;
  const wxColour curveColour[] = { series, pal.imaginary };
  if (logAxis)
    for (size_t i = 0; i < blk.curves.size(); i++) {
      lines.push_back(&blk.curves[i].values);
      colours.push_back(curveColour[std::min<size_t>(i, 1)]);
    }
  else {
    lines.push_back(&blk.plot);
    colours.push_back(series);
  }
  dc.SetFont(small);
  const int lh = dc.GetCharHeight();
  double lo = 1e300, hi = -1e300;
  //  A log curve's exact zero (a first SCF cycle has no energy change) is
  //  stored as 1e-16 and has no place on the axis.
  auto drawn = [&](double v) { return !logAxis || v > -15.5; };
  for (const vector<double>* v : lines)
    for (double d : *v)
      if (drawn(d)) {
        lo = std::min(lo, d);
        hi = std::max(hi, d);
      }
  if (lo > hi) lo = hi = 0;
  if (logAxis) {
    for (const auto& c : blk.curves)
      if (c.hasThreshold) {
        lo = std::min(lo, c.threshold);
        hi = std::max(hi, c.threshold);
      }
    lo = std::floor(lo - 1e-9);
    hi = std::ceil(hi + 1e-9);
    if (hi - lo < 1) hi = lo + 1;
  } else if (hi - lo < 1e-12) {
    lo -= 0.5;
    hi += 0.5;
  }
  vector<std::pair<double, wxString> > yTicks;
  if (logAxis) {
    const int most = std::max(2, (r.height - 2 * lh) / (lh + 3));
    const int step = std::max(1, (int)std::ceil((hi - lo) / most));
    for (int e = (int)hi; e >= (int)lo; e -= step)
      yTicks.push_back(std::make_pair((double)e, powerOfTen(e)));
  } else {
    yTicks.push_back(std::make_pair(hi, wxString::Format("%.4g", hi)));
    yTicks.push_back(std::make_pair(lo, wxString::Format("%.4g", lo)));
  }
  int left = 0;
  for (const auto& t : yTicks) left = std::max(left, dc.GetTextExtent(t.second).x);
  left += 4;
  const int axisH = lh + 2;
  wxRect area(r.x + left, r.y, r.width - left,
              std::max(8, r.height - lh - 2 - axisH));
  dc.SetPen(wxPen(pal.axis));
  dc.SetBrush(wxBrush(pal.background));
  dc.DrawRectangle(area);

  const double xEnd = blk.plotXEnd > 0 ? blk.plotXEnd : (double)blk.plot.size();
  const double xLast = blk.plotXLast > 0 ? blk.plotXLast : (double)blk.plot.size();
  //  Cycles and steps count from 1, so the frame starts there; time from 0.
  const bool whole = blk.plotXUnit != "ps";
  const double xStart = whole && xEnd > 1 ? 1.0 : 0.0;
  auto xFrac = [&](double x) { return (x - xStart) / (xEnd - xStart); };
  const int m = 3;
  auto yOf = [&](double v) {
    return area.y + m + (int)std::lround((1 - (v - lo) / (hi - lo)) * (area.height - 2 * m));
  };
  if (logAxis)
    for (const auto& t : yTicks) {
      dc.SetPen(wxPen(pal.grid));
      dc.DrawLine(area.x + 1, yOf(t.first), area.GetRight(), yOf(t.first));
    }
  for (size_t c = 0; c < lines.size(); c++) {
    const vector<double>& v = *lines[c];
    if (logAxis && blk.curves[c].hasThreshold) {
      dc.SetPen(wxPen(colours[c], 1, wxPENSTYLE_SHORT_DASH));
      dc.DrawLine(area.x + 1, yOf(blk.curves[c].threshold),
                  area.GetRight(), yOf(blk.curves[c].threshold));
    }
    vector<wxPoint> pts;
    for (size_t i = 0; i < v.size(); i++) {
      const double x = xLast * double(i + 1) / v.size();
      const double fx = xFrac(x);
      if (!drawn(v[i])) continue;
      pts.push_back(wxPoint(area.x + m + (int)std::lround(fx * (area.width - 2 * m)),
                            yOf(v[i])));
    }
    dc.SetPen(wxPen(colours[c], 2));
    if (pts.size() > 1) dc.DrawLines((int)pts.size(), &pts[0]);
    dc.SetBrush(wxBrush(colours[c]));
    dc.SetPen(*wxTRANSPARENT_PEN);
    for (const wxPoint& p : pts) dc.DrawCircle(p, 2);
  }

  dc.SetTextForeground(pal.axis);
  for (const auto& t : yTicks) {
    const int ty = std::min(std::max(yOf(t.first) - lh / 2, area.y),
                            area.GetBottom() - lh);
    dc.DrawText(t.second, r.x + left - 4 - dc.GetTextExtent(t.second).x, ty);
  }
  if (logAxis && blk.curves.size() > 1) {
    int lx = area.GetRight() - 4;
    for (size_t c = blk.curves.size(); c-- > 0;) {
      const wxString name = U(blk.curves[c].label);
      lx -= dc.GetTextExtent(name).x;
      dc.SetTextForeground(colours[c]);
      dc.DrawText(name, lx, area.y + 1);
      lx -= 8;
    }
    dc.SetTextForeground(pal.axis);
  }
  SpectrumTicks xt = niceTicks(0, xEnd, 3);
  if (whole) {
    xt.decimals = 0;
    xt.values.clear();
    const double step = std::max(1.0, std::ceil(xEnd / 3));
    if (xStart > 0) xt.values.push_back(1);
    for (double t = step; t <= xEnd + 1e-9; t += step)
      if (t > 1) xt.values.push_back(t);
  }
  for (double t : xt.values) {
    const int px = area.x + m + (int)std::lround(xFrac(t) * (area.width - 2 * m));
    const wxString s = wxString::Format("%.*f", xt.decimals, t);
    const int w = dc.GetTextExtent(s).x;
    if (px + w / 2 > r.GetRight()) continue;
    dc.SetPen(wxPen(pal.axis));
    dc.DrawLine(px, area.GetBottom(), px, area.GetBottom() + 3);
    dc.DrawText(s, std::max(area.x, px - w / 2), area.GetBottom() + 2);
  }
  dc.DrawText(U(label + (blk.plotXUnit.empty() ? "" : " (x: " + blk.plotXUnit + ")")),
              r.x, area.GetBottom() + axisH);
}


// `s` broken at spaces into rows no wider than `width` (a longer word keeps
// its own row).
static vector<wxString> wrapLine(wxDC& dc, const string& s, int width)
{
  vector<wxString> rows;
  wxString cur;
  for (const wxString& word : wxSplit(U(s), ' ')) {
    const wxString t = cur.empty() ? word : cur + " " + word;
    if (!cur.empty() && dc.GetTextExtent(t).x > width) {
      rows.push_back(cur);
      cur = word;
    } else {
      cur = t;
    }
  }
  rows.push_back(cur);
  return rows;
}


wxSize ProgressBox::layout(wxDC *dc)
{
  const bool lost = outOfContact();
  bool buttonsShown = false;
  bool energyShown = false;
  wxSize size(0, 0);
  if (wanted()) {
    wxFont normal = GetFont();
    wxFont bold = normal;
    bold.MakeBold();
    wxFont small = normal;
    small.SetFractionalPointSize(normal.GetFractionalPointSize() * 0.85);

    wxClientDC measure(this);
    wxDC& m = dc ? *dc : static_cast<wxDC&>(measure);
    m.SetFont(normal);
    const int lh = m.GetCharHeight() + FromDIP(2);
    const int gap = FromDIP(12);
    const PlotPalette pal = PlotPalette::system();
    const wxColour amber = ewxThemeColours::statusText(ewxThemeColours::UNSURE);
    const wxColour text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    //  Out of contact the last progress is history: greyed, not current.
    const wxColour fg = lost ? wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT)
                             : text;
    int y = 0, w = 0;

    // The heading, which folds the box.
    const string head = string(p_collapsed ? SHUT : OPEN) + " Progress";
    m.SetFont(bold);
    if (dc) {
      dc->SetTextForeground(text);
      dc->DrawText(U(head), 0, y);
    }
    const int headW = m.GetTextExtent(U(head)).x;
    m.SetFont(normal);
    if (p_collapsed) {
      const string line = foldedText();
      if (dc) {
        dc->SetTextForeground(lost ? amber : pal.axis);
        dc->DrawText(U(line), headW + gap, y);
      }
      w = headW + gap + m.GetTextExtent(U(line)).x;
      size = wxSize(w, headingHeight());
    } else {
      y += headingHeight() + FromDIP(2);

      vector<JobProgress::Block> blocks = this->blocks();
      int titleW = 0;
      m.SetFont(bold);
      for (const auto& b : blocks)
        titleW = std::max(titleW, m.GetTextExtent(U(b.title)).x);
      m.SetFont(normal);
      // The narrowest the box is ever laid out: one plot, or a title and a
      // little text.  Wider than this it wraps instead of growing.
      const int minPlotW = FromDIP(160), minPlotH = FromDIP(70);
      w = std::max(std::max(headW, minPlotW), titleW + gap + FromDIP(120));
      const int clientW = GetClientSize().x;
      const int availW = std::max(clientW > 0 ? clientW : FromDIP(600), w);

      {
        if (lost) {
          const string a = string(WARN) + " No contact: last heard from " +
              JobProgress::lastHeardText(p_lastHeard, (double)time(0));
          const string b = string("The job may still be ") +
                           (p_queued && p_progress.empty() ? "queued" : "running") +
                           "; ECCE keeps trying to reach its monitor.";
          if (dc) dc->SetTextForeground(amber);
          for (const wxString& r : wrapLine(m, a, availW)) {
            if (dc) dc->DrawText(r, 0, y);
            y += lh;
          }
          if (dc) dc->SetTextForeground(text);
          for (const wxString& r : wrapLine(m, b, availW)) {
            if (dc) dc->DrawText(r, 0, y);
            y += lh;
          }
          y += lh / 3;
        }

        // Titles in one column; text after them, wrapped to what is left;
        // the plots go under the text, side by side while they fit.
        int cellW = 0;
        m.SetFont(normal);
        for (const auto& b : blocks)
          for (const auto& c : b.criteria)
            cellW = std::max(cellW, m.GetTextExtent(U(c.label + " " + c.value +
                                                       " " + CHECK)).x);
        const int x0 = titleW + gap;
        const wxColour series = lost ? fg : pal.series(0);
        p_geom.clear();

        //  The plots share one column just right of the widest section text,
        //  so each stays next to its own lines instead of at the box's edge.
        int textNat = 0, plotNat = 0;
        for (const auto& b : blocks) {
          for (const string& l : b.lines)
            textNat = std::max(textNat, m.GetTextExtent(U(l)).x);
          if (!b.criteria.empty())
            textNat = std::max(textNat, b.criteria.size() > 1 ? 2 * cellW + gap
                                                              : cellW);
          if (b.bar) textNat = std::max(textNat, FromDIP(240));
          if (!b.plot.empty()) {
            m.SetFont(small);
            plotNat = std::max(plotNat, std::max(FromDIP(260),
                          m.GetTextExtent(U(plotCaption(b))).x + 2));
            m.SetFont(normal);
          }
        }
        const int plotX = std::min(availW - plotNat, x0 + textNat + 2 * gap);

        for (const auto& b : blocks) {
          const int y0 = y;
          //  A plot goes to the right of its section's text when at least
          //  ~150 px of text remain, else directly under it.
          int pw = 0, ph = 0;
          if (!b.plot.empty()) {
            m.SetFont(small);
            pw = std::min(availW, std::max(FromDIP(260),
                          m.GetTextExtent(U(plotCaption(b))).x + 2));
            ph = minPlotH + FromDIP(20) + m.GetCharHeight() + 2;
            m.SetFont(normal);
          }
          const bool beside = pw > 0 && plotX - gap - x0 >= FromDIP(150);
          const int textAvail = (beside ? plotX - gap : availW) - x0;
          const int barW = std::min(FromDIP(240), textAvail);
          const int cols = 2 * cellW + gap <= textAvail ? 2 : 1;
          if (dc) {
            dc->SetFont(bold);
            dc->SetTextForeground(fg);
            dc->DrawText(U(b.title), 0, y);
            dc->SetFont(normal);
          }
          for (size_t li = 0; li < b.lines.size(); li++) {
            const vector<wxString> rows = wrapLine(m, b.lines[li], textAvail);
            for (size_t ri = 0; ri < rows.size(); ri++) {
              if (dc) dc->DrawText(rows[ri], x0, y);
              //  The imaginary count follows the last row of the last line.
              if (dc && !b.alert.empty() && li + 1 == b.lines.size() &&
                  ri + 1 == rows.size()) {
                const int ax = x0 + m.GetTextExtent(rows[ri]).x;
                dc->SetTextForeground(b.alertBad && !lost ? pal.imaginary : fg);
                dc->DrawText(U(string(MIDDOT) + b.alert), ax, y);
                dc->SetTextForeground(fg);
              }
              y += lh;
            }
          }
          for (size_t i = 0; i < b.criteria.size(); i++) {
            const auto& c = b.criteria[i];
            const int x = x0 + (int)(i % cols) * (cellW + gap);
            if (dc) {
              const wxString t = U(c.label + " " + c.value + " ");
              dc->SetTextForeground(fg);
              dc->DrawText(t, x, y);
              dc->SetTextForeground(lost ? fg : c.met ? pal.raman : pal.imaginary);
              dc->DrawText(U(c.met ? CHECK : CROSS), x + dc->GetTextExtent(t).x, y);
            }
            if ((int)(i % cols) == cols - 1 || i + 1 == b.criteria.size())
              y += lh;
          }
          if (b.bar) {
            const wxRect bar(x0, y + lh / 5, barW, lh * 3 / 5);
            if (dc) {
              dc->SetPen(wxPen(pal.axis));
              dc->SetBrush(wxBrush(pal.box));
              dc->DrawRectangle(bar);
              dc->SetPen(*wxTRANSPARENT_PEN);
              dc->SetBrush(wxBrush(series));
              dc->DrawRectangle(bar.x + 1, bar.y + 1,
                                (int)std::lround((bar.width - 2) * b.fraction),
                                bar.height - 2);
            }
            y += lh;
          }
          // A stage with no line yet (a title only) still takes its own row,
          // or the next title is drawn over it.
          y = std::max(y, y0 + lh);
          y += lh / 3;
          const int textEnd = y;
          if (!b.plot.empty()) {
            const wxRect r = beside ? wxRect(plotX, y0, pw, ph)
                                    : wxRect(0, y, pw, ph);
            if (dc) {
              drawPlot(*dc, r, b, plotCaption(b), pal, small, series);
              dc->SetFont(normal);
            }
            p_geom.push_back(b.title + " text " + std::to_string(y0) + " " +
                             std::to_string(textEnd) + " plot " +
                             std::to_string(r.x) + " " + std::to_string(r.y) +
                             " " + std::to_string(r.width) + " " +
                             std::to_string(r.height));
            y = beside ? std::max(y, y0 + ph + lh / 3) : y + ph + FromDIP(6);
          }
        }

        // A finished job keeps its plots; the outcome goes under them.
        if (p_finished) {
          for (const string& s : outcome()) {
            //  The final energy is a real label, so it can be selected and
            //  copied (#408); placed when measuring, like the buttons.
            if (s == p_finalEnergy) {
              if (!dc) {
                p_energyLabel->SetLabel(U(s));
                p_energyLabel->SetForegroundColour(text);
                p_energyLabel->SetSize(0, y, m.GetTextExtent(U(s)).x + 2, lh);
                p_energyLabel->Show();
              }
              energyShown = true;
              y += lh;
            } else {
              if (dc) dc->SetTextForeground(text);
              for (const wxString& r : wrapLine(m, s, availW)) {
                if (dc) dc->DrawText(r, 0, y);
                y += lh;
              }
            }
          }
        }

        const double a = age();
        if (!lost && !p_finished && a >= 0) {
          if (dc) {
            dc->SetFont(small);
            dc->SetTextForeground(pal.axis);
            dc->DrawText(U(JobProgress::updatedText(a)), 0, y);
            dc->SetFont(normal);
          }
          y += lh;
        }

        if (lost) {
          // Real buttons, placed here when measuring (dc == 0); a row that
          // does not fit goes on to the next.
          int x = 0, bh = 0;
          for (int i = 0; i < 3; i++) {
            const wxSize bs = p_buttons[i]->GetBestSize();
            if (x > 0 && x + bs.x > availW) {
              x = 0;
              y += bh + FromDIP(4);
              bh = 0;
            }
            if (!dc) {
              p_buttons[i]->SetSize(x, y, bs.x, bs.y);
              p_buttons[i]->Show();
            }
            x += bs.x + FromDIP(6);
            bh = std::max(bh, bs.y);
          }
          buttonsShown = true;
          y += bh + FromDIP(4);
          if (dc) dc->SetTextForeground(text);
          for (const string& l : p_check)
            for (const wxString& r : wrapLine(m, l, availW)) {
              if (dc) dc->DrawText(r, 0, y);
              y += lh;
            }
        }
        size = wxSize(w, y);
      }
    }
  }
  if (!dc && !buttonsShown)
    for (int i = 0; i < 3; i++) {
      p_buttons[i]->SetSize(0, 0, 0, 0);
      p_buttons[i]->Show(true);
    }
  if (!dc && !energyShown) {
    p_energyLabel->SetSize(0, 0, 0, 0);
    p_energyLabel->Show(true);
  }
  return size;
}


void ProgressBox::OnPaint(wxPaintEvent&)
{
  wxAutoBufferedPaintDC dc(this);
  dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
  dc.Clear();
  dc.SetFont(GetFont());
  p_paints++;
  layout(&dc);
}


// The height depends on the width (text and plots wrap), so a new width
// can need a new height; the parent lays out again for it.
void ProgressBox::OnSize(wxSizeEvent& event)
{
  event.Skip();
  if (!wanted() || p_collapsed || GetClientSize().x == p_laidOutWidth) return;
  p_laidOutWidth = GetClientSize().x;
  const wxSize want = layout(0);
  if (want != GetMinSize()) {
    SetMinSize(want);
    CallAfter([this]() { if (p_relayout) p_relayout(); });
  }
}


void ProgressBox::OnTimer(wxTimerEvent&)
{
  if (IsShownOnScreen()) Refresh(false);
}


bool ProgressBox::energySelectable() const
{
  return p_energyLabel->IsShown() && ewxIsSelectable(p_energyLabel);
}
