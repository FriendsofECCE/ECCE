/**
 * @file
 *
 * The Progress box of the calculation summary (#284).
 */
#include <algorithm>
#include <cmath>
#include <ctime>

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/settings.h>

#include "wxgui/PlotStyle.H"

#include "ProgressBox.H"

using std::string;
using std::vector;

static const char *CHECK = "\xe2\x9c\x93";   // U+2713
static const char *CROSS = "\xe2\x9c\x97";   // U+2717

static wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }


ProgressBox::ProgressBox(wxWindow *parent)
  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxFULL_REPAINT_ON_RESIZE, "progressBox"),
    p_received(0), p_finished(false), p_completed(false), p_timer(this),
    p_paints(0)
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  Bind(wxEVT_PAINT, &ProgressBox::OnPaint, this);
  Bind(wxEVT_TIMER, &ProgressBox::OnTimer, this);
}


ProgressBox::~ProgressBox()
{
  p_timer.Stop();
}


bool ProgressBox::set(const string& text, double received, bool finished,
                      bool completed)
{
  if (text != p_text) {
    p_text = text;
    p_progress.parse(text);
  }
  p_received = received;
  p_finished = finished;
  p_completed = completed;

  //  A running job's box ages by the second ("updated n s ago", the time
  //  left); only the box is repainted for it.
  if (!finished && hasProgress()) {
    if (!p_timer.IsRunning()) p_timer.Start(1000);
  } else {
    p_timer.Stop();
  }

  const wxSize want = layout(0);
  const bool changed = want != GetMinSize();
  if (changed) SetMinSize(want);
  Refresh(false);
  return changed;
}


double ProgressBox::age() const
{
  const double now = (double)time(0);
  const double from = p_received > 0 ? p_received : p_progress.at();
  return from > 0 ? std::max(0.0, now - from) : -1;
}


vector<string> ProgressBox::describe() const
{
  vector<string> out;
  if (!hasProgress()) return out;
  if (p_finished) {
    for (const string& s : p_progress.summary(p_completed))
      out.push_back(s);
    return out;
  }
  for (const JobProgress::Block& b : p_progress.running(std::max(0.0, age()))) {
    for (const string& l : b.lines) out.push_back(b.title + ": " + l);
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
  if (age() >= 0) out.push_back(JobProgress::updatedText(age()));
  return out;
}


// A small line plot of one series, framed, with its label under it.
static void drawPlot(wxDC& dc, const wxRect& r, const vector<double>& v,
                     const string& label, const PlotPalette& pal,
                     const wxFont& small)
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
  dc.SetPen(wxPen(pal.series(0), 2));
  if (pts.size() > 1) dc.DrawLines((int)pts.size(), &pts[0]);
  dc.SetBrush(wxBrush(pal.series(0)));
  dc.SetPen(*wxTRANSPARENT_PEN);
  for (const wxPoint& p : pts) dc.DrawCircle(p, 2);
  dc.SetTextForeground(pal.axis);
  dc.DrawText(U(label), r.x, area.GetBottom() + 2);
}


wxSize ProgressBox::layout(wxDC *dc) const
{
  if (!hasProgress()) return wxSize(0, 0);
  wxFont normal = GetFont();
  wxFont bold = normal;
  bold.MakeBold();
  wxFont small = normal;
  small.SetFractionalPointSize(normal.GetFractionalPointSize() * 0.85);

  wxClientDC measure(const_cast<ProgressBox*>(this));
  wxDC& m = dc ? *dc : static_cast<wxDC&>(measure);
  m.SetFont(normal);
  const int lh = m.GetCharHeight() + FromDIP(2);
  const int gap = FromDIP(12);
  const PlotPalette pal = PlotPalette::system();
  const wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
  int y = 0, w = 0;

  if (p_finished) {
    for (const string& s : p_progress.summary(p_completed)) {
      if (dc) {
        dc->SetTextForeground(fg);
        dc->DrawText(U(s), 0, y);
      }
      w = std::max(w, m.GetTextExtent(U(s)).x);
      y += lh;
    }
    return wxSize(w, y);
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
  w = anyPlot ? plotX + plotW : x0 + textW;

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
        dc->SetTextForeground(c.met ? pal.raman : pal.imaginary);
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
        dc->SetBrush(wxBrush(pal.series(0)));
        dc->DrawRectangle(bar.x + 1, bar.y + 1,
                          (int)std::lround((bar.width - 2) * b.fraction),
                          bar.height - 2);
      }
      y += lh;
    }
    if (!b.plot.empty()) {
      if (dc)
        drawPlot(*dc, wxRect(plotX, y0, plotW, plotH), b.plot, b.plotLabel,
                 pal, small);
      y = std::max(y, y0 + plotH);
      if (dc) dc->SetFont(normal);
    }
    y += lh / 3;
  }
  const double a = age();
  if (a >= 0) {
    if (dc) {
      dc->SetFont(small);
      dc->SetTextForeground(pal.axis);
      dc->DrawText(U(JobProgress::updatedText(a)), 0, y);
      dc->SetFont(normal);
    }
    y += lh;
  }
  return wxSize(w, y);
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
