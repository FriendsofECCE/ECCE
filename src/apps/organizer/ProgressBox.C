/**
 * @file
 *
 * The Progress box of the calculation summary (#284).
 */
#include <algorithm>
#include <cmath>
#include <ctime>

#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/settings.h>

#include "wxgui/PlotStyle.H"
#include "wxgui/ewxConfig.H"
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
  Bind(wxEVT_LEFT_DOWN, &ProgressBox::OnClick, this);
  ewxConfig::getConfig(INIFILE)->Read(FOLDKEY, &p_collapsed);
  for (int i = 0; i < 3; i++) {
    p_buttons[i] = new wxButton(this, wxID_ANY, ACTIONS[i]);
    p_buttons[i]->Hide();
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
  const double from = p_received > 0 ? p_received : p_progress.at();
  return from > 0 ? std::max(0.0, now - from) : -1;
}


// The one line of the folded box, after its heading.
string ProgressBox::foldedText() const
{
  if (outOfContact())
    return JobProgress::noContactText((double)time(0) - p_lastHeard);
  if (p_finished) {
    vector<string> s = p_progress.summary(p_completed);
    return s.empty() ? string() : s[0];
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


vector<string> ProgressBox::describe() const
{
  vector<string> out;
  if (!wanted()) return out;
  out.push_back(string("heading ") + (p_collapsed ? "folded" : "open"));
  if (outOfContact()) {
    out.push_back("contact lost, last heard " +
                  JobProgress::lastHeardText(p_lastHeard, (double)time(0)));
    int shown = 0;
    for (int i = 0; i < 3; i++) shown += p_buttons[i]->IsShown() ? 1 : 0;
    out.push_back("buttons " + std::to_string(shown));
  }
  if (p_collapsed) {
    out.push_back("folded: " + foldedText());
    return out;
  }
  if (p_finished) {
    for (const string& s : p_progress.summary(p_completed))
      out.push_back(s);
    return out;
  }
  const string grey = outOfContact() ? "(grey) " : "";
  for (const JobProgress::Block& b : p_progress.running(std::max(0.0, age()))) {
    for (const string& l : b.lines) out.push_back(grey + b.title + ": " + l);
    for (const JobProgress::Criterion& c : b.criteria)
      out.push_back("  " + c.label + " " + c.value + " " +
                    (c.met ? CHECK : CROSS));
    if (b.bar) {
      char f[32];
      snprintf(f, sizeof f, "  bar %.2f", b.fraction);
      out.push_back(f);
    }
    if (!b.plot.empty())
      out.push_back("  plot " + b.plotLabel + " (" +
                    std::to_string(b.plot.size()) + " points)");
  }
  if (!outOfContact() && age() >= 0)
    out.push_back(JobProgress::updatedText(age()));
  for (const string& l : p_check) out.push_back("check: " + l);
  return out;
}


// A small line plot of one series, framed, with its label under it.
static void drawPlot(wxDC& dc, const wxRect& r, const vector<double>& v,
                     const string& label, const PlotPalette& pal,
                     const wxFont& small, const wxColour& series)
{
  dc.SetFont(small);
  const int lh = dc.GetCharHeight();
  wxRect area(r.x, r.y, r.width, std::max(8, r.height - lh - 2));
  dc.SetPen(wxPen(pal.axis));
  dc.SetBrush(wxBrush(pal.background));
  dc.DrawRectangle(area);
  double lo = *std::min_element(v.begin(), v.end());
  double hi = *std::max_element(v.begin(), v.end());
  if (hi - lo < 1e-12) { lo -= 0.5; hi += 0.5; }
  const int m = 3;
  vector<wxPoint> pts;
  for (size_t i = 0; i < v.size(); i++) {
    const double fx = v.size() > 1 ? double(i) / (v.size() - 1) : 0.5;
    const double fy = (v[i] - lo) / (hi - lo);
    pts.push_back(wxPoint(area.x + m + (int)std::lround(fx * (area.width - 2 * m)),
                          area.y + m + (int)std::lround((1 - fy) * (area.height - 2 * m))));
  }
  dc.SetPen(wxPen(series, 2));
  if (pts.size() > 1) dc.DrawLines((int)pts.size(), &pts[0]);
  dc.SetBrush(wxBrush(series));
  dc.SetPen(*wxTRANSPARENT_PEN);
  for (const wxPoint& p : pts) dc.DrawCircle(p, 2);
  dc.SetTextForeground(pal.axis);
  dc.DrawText(U(label), r.x, area.GetBottom() + 2);
}


wxSize ProgressBox::layout(wxDC *dc)
{
  const bool lost = outOfContact();
  bool buttonsShown = false;
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
      w = headW;
      y += headingHeight() + FromDIP(2);

      if (p_finished) {
        for (const string& s : p_progress.summary(p_completed)) {
          if (dc) {
            dc->SetTextForeground(text);
            dc->DrawText(U(s), 0, y);
          }
          w = std::max(w, m.GetTextExtent(U(s)).x);
          y += lh;
        }
        size = wxSize(w, y);
      } else {
        if (lost) {
          const string a = string(WARN) + " No contact: last heard from " +
              JobProgress::lastHeardText(p_lastHeard, (double)time(0));
          const string b = "The job may still be running; ECCE keeps trying "
                           "to reach its monitor.";
          if (dc) {
            dc->SetTextForeground(amber);
            dc->DrawText(U(a), 0, y);
            dc->SetTextForeground(text);
            dc->DrawText(U(b), 0, y + lh);
          }
          w = std::max(w, std::max(m.GetTextExtent(U(a)).x,
                                   m.GetTextExtent(U(b)).x));
          y += 2 * lh + lh / 3;
        }

        vector<JobProgress::Block> blocks =
            p_progress.running(std::max(0.0, age()));
        // Titles in one column; text after them; plots at the right.
        int titleW = 0, cellW = 0;
        m.SetFont(bold);
        for (const auto& b : blocks)
          titleW = std::max(titleW, m.GetTextExtent(U(b.title)).x);
        m.SetFont(normal);
        for (const auto& b : blocks)
          for (const auto& c : b.criteria)
            cellW = std::max(cellW, m.GetTextExtent(U(c.label + " " + c.value +
                                                       " " + CHECK)).x);
        const int x0 = titleW + gap;
        const int barW = FromDIP(240);
        // The text column is as wide as its widest line; plots go after it.
        int textW = 0;
        bool anyPlot = false;
        for (const auto& b : blocks) {
          for (const string& l : b.lines)
            textW = std::max(textW, m.GetTextExtent(U(l)).x);
          if (!b.criteria.empty())
            textW = std::max(textW, (b.criteria.size() > 1 ? 2 : 1) * cellW + gap);
          if (b.bar) textW = std::max(textW, barW);
          anyPlot = anyPlot || !b.plot.empty();
        }
        const int plotW = FromDIP(160), plotH = 3 * lh;
        const int plotX = x0 + textW + gap;
        if (!blocks.empty())
          w = std::max(w, anyPlot ? plotX + plotW : x0 + textW);
        const wxColour series = lost ? fg : pal.series(0);

        for (const auto& b : blocks) {
          const int y0 = y;
          if (dc) {
            dc->SetFont(bold);
            dc->SetTextForeground(fg);
            dc->DrawText(U(b.title), 0, y);
            dc->SetFont(normal);
          }
          for (const string& l : b.lines) {
            if (dc) dc->DrawText(U(l), x0, y);
            y += lh;
          }
          for (size_t i = 0; i < b.criteria.size(); i++) {
            const auto& c = b.criteria[i];
            const int x = x0 + (int)(i % 2) * (cellW + gap);
            if (dc) {
              const wxString t = U(c.label + " " + c.value + " ");
              dc->SetTextForeground(fg);
              dc->DrawText(t, x, y);
              dc->SetTextForeground(lost ? fg : c.met ? pal.raman : pal.imaginary);
              dc->DrawText(U(c.met ? CHECK : CROSS), x + dc->GetTextExtent(t).x, y);
            }
            if (i % 2 == 1 || i + 1 == b.criteria.size()) y += lh;
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
          if (!b.plot.empty()) {
            if (dc)
              drawPlot(*dc, wxRect(plotX, y0, plotW, plotH), b.plot,
                       b.plotLabel, pal, small, series);
            y = std::max(y, y0 + plotH);
            if (dc) dc->SetFont(normal);
          }
          y += lh / 3;
        }

        const double a = age();
        if (!lost && a >= 0) {
          if (dc) {
            dc->SetFont(small);
            dc->SetTextForeground(pal.axis);
            dc->DrawText(U(JobProgress::updatedText(a)), 0, y);
            dc->SetFont(normal);
          }
          y += lh;
        }

        if (lost) {
          // Real buttons, placed here when measuring (dc == 0).
          int x = 0, bh = 0;
          for (int i = 0; i < 3; i++) {
            const wxSize bs = p_buttons[i]->GetBestSize();
            if (!dc) {
              p_buttons[i]->SetSize(x, y, bs.x, bs.y);
              p_buttons[i]->Show();
            }
            x += bs.x + FromDIP(6);
            bh = std::max(bh, bs.y);
          }
          buttonsShown = true;
          w = std::max(w, x);
          y += bh + FromDIP(4);
          for (const string& l : p_check) {
            if (dc) {
              dc->SetTextForeground(text);
              dc->DrawText(U(l), 0, y);
            }
            w = std::max(w, m.GetTextExtent(U(l)).x);
            y += lh;
          }
        }
        size = wxSize(w, y);
      }
    }
  }
  if (!dc && !buttonsShown)
    for (int i = 0; i < 3; i++) p_buttons[i]->Hide();
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


void ProgressBox::OnTimer(wxTimerEvent&)
{
  if (IsShownOnScreen()) Refresh(false);
}
