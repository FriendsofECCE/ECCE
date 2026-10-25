#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/wrapsizer.h>

#include "PlaybackLoop.H"
#include "PlaybackExtras.H"

PlaybackExtras::PlaybackExtras(wxWindow *parent, wxSizer *sizer, int *delay)
  : p_delay(delay)
{
  wxWrapSizer *row = new wxWrapSizer(wxHORIZONTAL);
  row->Add(new wxStaticText(parent, wxID_ANY, "Playback speed:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_speed = new wxChoice(parent, wxID_ANY);
  p_speed->Append("Slow");
  p_speed->Append("Normal");
  p_speed->Append("Fast");
  p_speed->SetSelection(playbackloop::NORMAL);
  p_speed->SetToolTip("Time between points: 1 s, 0.5 s or 0.15 s");
  row->Add(p_speed, 0, wxRIGHT, 12);
  p_loop = new wxCheckBox(parent, wxID_ANY, "Loop");
  p_loop->SetToolTip("Play forward to the last point, then backward to the "
                     "first, and repeat until stopped");
  row->Add(p_loop, 0, wxALIGN_CENTER_VERTICAL);
  sizer->Add(row, 0, wxLEFT | wxRIGHT, 4);
  p_speed->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onSpeed(); });
}

void PlaybackExtras::onSpeed()
{
  if (p_delay) *p_delay = playbackloop::delayMs(p_speed->GetSelection());
}

bool PlaybackExtras::setSpeed(const std::string& name)
{
  int i = name == "slow" ? 0 : name == "normal" ? 1 : name == "fast" ? 2 : -1;
  if (i < 0) return false;
  p_speed->SetSelection(i);
  onSpeed();
  return true;
}

std::string PlaybackExtras::speed() const
{
  static const char *names[] = {"slow", "normal", "fast"};
  return names[p_speed->GetSelection()];
}

void PlaybackExtras::setLoop(bool on) { p_loop->SetValue(on); }

bool PlaybackExtras::loop() const { return p_loop->GetValue(); }
