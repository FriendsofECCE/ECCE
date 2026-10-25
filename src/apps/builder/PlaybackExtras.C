#include <cstdlib>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/wrapsizer.h>

#include "PlaybackLoop.H"
#include "PlaybackExtras.H"

PlaybackExtras::PlaybackExtras(wxWindow *parent, wxSizer *sizer, int *delay)
  : p_delay(delay)
{
  wxWrapSizer *row = new wxWrapSizer(wxHORIZONTAL);
  //  Milliseconds per step, as the Geometry Trace's Delay field.
  row->Add(new wxStaticText(parent, wxID_ANY, "Delay:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_speed = new wxTextCtrl(parent, wxID_ANY, std::to_string(delay ? *delay : 500));
  p_speed->SetToolTip("Time between points during playback, 1 to 10000 ms");
  row->Add(p_speed, 0, wxALIGN_CENTER_VERTICAL);
  row->Add(new wxStaticText(parent, wxID_ANY, "ms"), 0,
           wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 4);
  row->AddSpacer(8);
  p_loop = new wxCheckBox(parent, wxID_ANY, "Loop");
  p_loop->SetToolTip("Play forward to the last point, then backward to the "
                     "first, and repeat until stopped");
  row->Add(p_loop, 0, wxALIGN_CENTER_VERTICAL);
  sizer->Add(row, 0, wxLEFT | wxRIGHT, 4);
  p_speed->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { onSpeed(); });
}

void PlaybackExtras::onSpeed()
{
  long ms = 0;
  if (p_delay && p_speed->GetValue().ToLong(&ms) && ms >= 1 && ms <= 10000)
    *p_delay = (int)ms;
}

bool PlaybackExtras::setSpeed(const std::string& name)
{
  const int ms = atoi(name.c_str());
  if (ms < 1 || ms > 10000) return false;
  p_speed->ChangeValue(std::to_string(ms));
  onSpeed();
  return true;
}

std::string PlaybackExtras::speed() const
{
  return p_delay ? std::to_string(*p_delay) : std::string();
}

void PlaybackExtras::setLoop(bool on) { p_loop->SetValue(on); }

bool PlaybackExtras::loop() const { return p_loop->GetValue(); }
