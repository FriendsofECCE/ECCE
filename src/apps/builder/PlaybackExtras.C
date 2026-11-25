#include <cstdlib>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/wrapsizer.h>

#include "PlaybackLoop.H"
#include "PlaybackExtras.H"
#include "wxgui/PlaybackControl.H"

PlaybackExtras::PlaybackExtras(wxWindow *parent, wxSizer *sizer, int *delay,
                               PlaybackControl *control,
                               playbackloop::Repeat repeat, bool withMo)
  : p_control(control), p_delay(delay)
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
  row->Add(new wxStaticText(parent, wxID_ANY, "Repeat:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_repeat = new wxChoice(parent, wxID_ANY);
  for (playbackloop::Repeat r : {playbackloop::OFF, playbackloop::FROM_START,
                                 playbackloop::BACK_AND_FORTH})
    p_repeat->Append(playbackloop::repeatLabel(r));
  p_repeat->SetToolTip("What Play does at the last point: stop there, start "
                       "again from the first, or play backward to the first "
                       "and on until stopped");
  row->Add(p_repeat, 0, wxALIGN_CENTER_VERTICAL);
  setRepeat(repeat);
  p_repeat->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { setRepeat(this->repeat()); });
  if (withMo) {
    row->AddSpacer(8);
    p_mo = new wxCheckBox(parent, wxID_ANY, "Show MO while playing");
    p_mo->SetToolTip("Show the orbital selected in the MOs panel on every "
                     "point, following it by its character from point to point");
    row->Add(p_mo, 0, wxALIGN_CENTER_VERTICAL);
  }
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

void PlaybackExtras::setRepeat(playbackloop::Repeat r)
{
  p_repeat->SetSelection((int)r);
  if (p_control) p_control->SetRepeat(r != playbackloop::OFF);
}

playbackloop::Repeat PlaybackExtras::repeat() const
{
  return (playbackloop::Repeat)p_repeat->GetSelection();
}

void PlaybackExtras::setShowMo(bool on) { if (p_mo) p_mo->SetValue(on); }

bool PlaybackExtras::showMo() const { return moEnabled() && p_mo->GetValue(); }

void PlaybackExtras::disableMo(const std::string& why)
{
  if (!p_mo) return;
  p_mo->SetValue(false);
  p_mo->Enable(false);
  p_mo->SetToolTip(wxString::FromUTF8(why.c_str()));
}

bool PlaybackExtras::moEnabled() const { return p_mo && p_mo->IsEnabled(); }
