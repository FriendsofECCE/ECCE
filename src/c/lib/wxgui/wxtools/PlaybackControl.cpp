
#include <wx/sizer.h>
#include <wx/tglbtn.h>
#include <wx/timer.h>

#include "wxgui/ThingToggle.H"
#include "wxgui/PlaybackControl.H"
#include "wxgui/ewxImage.H"


DEFINE_EVENT_TYPE(wxEVT_PLAYBACK_TICK_EVENT)
DEFINE_EVENT_TYPE(wxEVT_PLAYBACK_LOOP_EVENT)


IMPLEMENT_DYNAMIC_CLASS(PlaybackControl, ewxPanel)


const int BORDER = 2;


const char * TIP_START_UNCHECKED  = "Animate backward until initial step";
const char * TIP_START_CHECKED    = "Stop animating";
const char * TIP_PREVIOUS         = "Previous step";
const char * TIP_PLAY_UNCHECKED   = "Animate with looping";
const char * TIP_PLAY_CHECKED     = "Stop animating";
const char * TIP_STOP             = "Stop animating and go to initial step";
const char * TIP_NEXT             = "Next step";
const char * TIP_END_UNCHECKED    = "Animate forward until last step";
const char * TIP_END_CHECKED      = "Stop animating";
const char * TIP_RECORD_UNCHECKED = "Record each step to output file";
const char * TIP_RECORD_CHECKED   = "Stop recording";


const wxWindowID PlaybackControl::ID_TIMER = wxNewId();
const wxWindowID PlaybackControl::ID_START = wxNewId();
const wxWindowID PlaybackControl::ID_PREVIOUS = wxNewId();
const wxWindowID PlaybackControl::ID_PLAY = wxNewId();
const wxWindowID PlaybackControl::ID_STOP = wxNewId();
const wxWindowID PlaybackControl::ID_NEXT = wxNewId();
const wxWindowID PlaybackControl::ID_END = wxNewId();
const wxWindowID PlaybackControl::ID_RECORD = wxNewId();


BEGIN_EVENT_TABLE(PlaybackControl, ewxPanel)
  EVT_TIMER(ID_TIMER, PlaybackControl::OnTimer)
  EVT_TOGGLEBUTTON(ID_START, PlaybackControl::OnStartClick)
  EVT_BUTTON(ID_PREVIOUS, PlaybackControl::OnPreviousClick)
  EVT_TOGGLEBUTTON(ID_PLAY, PlaybackControl::OnPlayClick)
  EVT_BUTTON(ID_STOP, PlaybackControl::OnStopClick)
  EVT_BUTTON(ID_NEXT, PlaybackControl::OnNextClick)
  EVT_TOGGLEBUTTON(ID_END, PlaybackControl::OnEndClick)
  EVT_TOGGLEBUTTON(ID_RECORD, PlaybackControl::OnRecordClick)
END_EVENT_TABLE()


PlaybackControl::PlaybackControl()
  : ewxPanel(),
    p_timer(new wxTimer(this, ID_TIMER)),
    p_delay(new int(100)),
    p_startIndex(new long(0)),
    p_stopIndex(new long(10)),
    p_stepSize(new long(1)),
    p_currentStep(new long(0)),
    p_delayInternal(true),
    p_startIndexInternal(true),
    p_stopIndexInternal(true),
    p_stepSizeInternal(true),
    p_currentStepInternal(true),
    p_start(NULL),
    p_previous(NULL),
    p_play(NULL),
    p_stop(NULL),
    p_next(NULL),
    p_end(NULL),
    p_record(NULL)
{
}


PlaybackControl::PlaybackControl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : ewxPanel(),
    p_timer(new wxTimer(this, ID_TIMER)),
    p_delay(new int(100)),
    p_startIndex(new long(0)),
    p_stopIndex(new long(10)),
    p_stepSize(new long(1)),
    p_currentStep(new long(0)),
    p_delayInternal(true),
    p_startIndexInternal(true),
    p_stopIndexInternal(true),
    p_stepSizeInternal(true),
    p_currentStepInternal(true),
    p_start(NULL),
    p_previous(NULL),
    p_play(NULL),
    p_stop(NULL),
    p_next(NULL),
    p_end(NULL),
    p_record(NULL)
{
  Create(parent, id, pos, size, style, name);
}


PlaybackControl::~PlaybackControl()
{
}


bool PlaybackControl::Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!ewxPanel::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PlaybackControl creation failed") );
    return false;
  }

  if (!GetSizer()) {
    SetSizer(new wxBoxSizer(wxHORIZONTAL));
  }

  p_start = new wxCustomButton(this, ID_START,
          wxBitmap(ewxImage::get(ewxImage::PLAYER_START)));
  p_previous = new wxCustomButton(this, ID_PREVIOUS,
          wxBitmap(ewxImage::get(ewxImage::PLAYER_REW)),
          wxDefaultPosition, wxDefaultSize, wxCUSTBUT_BUTTON);
  p_play = new wxCustomButton(this, ID_PLAY,
          wxBitmap(ewxImage::get(ewxImage::PLAYER_PLAY)));
  p_stop = new wxCustomButton(this, ID_STOP,
          wxBitmap(ewxImage::get(ewxImage::PLAYER_STOP)),
          wxDefaultPosition, wxDefaultSize, wxCUSTBUT_BUTTON);
  p_next = new wxCustomButton(this, ID_NEXT,
          wxBitmap(ewxImage::get(ewxImage::PLAYER_FWD)),
          wxDefaultPosition, wxDefaultSize, wxCUSTBUT_BUTTON);
  p_end = new wxCustomButton(this, ID_END,
          wxBitmap(ewxImage::get(ewxImage::PLAYER_END)));
  p_record = new wxCustomButton(this, ID_RECORD,
          wxBitmap(ewxImage::get(ewxImage::PLAYER_RECORD)));
  UpdateTooltips();

  GetSizer()->Add(p_start, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(p_previous, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(p_play, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(p_stop, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(p_next, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(p_end, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);
  GetSizer()->Add(p_record, 0, wxALIGN_CENTER_VERTICAL|wxALL, BORDER);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  return true;
}


void PlaybackControl::SetDelay(int * val)
{
  if (p_delayInternal) {
    delete p_delay;
    p_delayInternal = false;
  }
  p_delay = val;
}


void PlaybackControl::SetStartIndex(long * val)
{
  if (p_startIndexInternal) {
    delete p_startIndex;
    p_startIndexInternal = false;
  }
  p_startIndex = val;
}


void PlaybackControl::SetStopIndex(long * val)
{
  if (p_stopIndexInternal) {
    delete p_stopIndex;
    p_stopIndexInternal = false;
  }
  p_stopIndex = val;
}


void PlaybackControl::SetStepSize(long * val)
{
  if (p_stepSizeInternal) {
    delete p_stepSize;
    p_stepSizeInternal = false;
  }
  p_stepSize = val;
}


void PlaybackControl::SetCurrentStep(long * val)
{
  if (p_currentStepInternal) {
    delete p_currentStep;
    p_currentStepInternal = false;
  }
  p_currentStep = val;
}


void PlaybackControl::SetDelay(const int& val)
{
  if (!p_delayInternal) {
    p_delay = new int;
    p_delayInternal = true;
  }
  *p_delay = val;
}


void PlaybackControl::SetStartIndex(const long& val)
{
  if (!p_startIndexInternal) {
    p_startIndex = new long;
    p_startIndexInternal = true;
  }
  *p_startIndex = val;
}


void PlaybackControl::SetStopIndex(const long& val)
{
  if (!p_stopIndexInternal) {
    p_stopIndex = new long;
    p_stopIndexInternal = true;
  }
  *p_stopIndex = val;
}


void PlaybackControl::SetStepSize(const long& val)
{
  if (!p_stepSizeInternal) {
    p_stepSize = new long;
    p_stepSizeInternal = true;
  }
  *p_stepSize = val;
}


void PlaybackControl::SetCurrentStep(const long& val)
{
  if (!p_currentStepInternal) {
    p_currentStep = new long;
    p_currentStepInternal = true;
  }
  *p_currentStep = val;
}


void PlaybackControl::Pause()
{
  ClearPlayToggles();
  p_timer->Stop();
}


void PlaybackControl::PlaybackControl::ClearPlayToggles(wxWindow * keep)
{
  if (p_start != keep) p_start->SetValue(false);
  if (p_play != keep) p_play->SetValue(false);
  if (p_end != keep) p_end->SetValue(false);
}


void PlaybackControl::SendTickEvent()
{
  wxCommandEvent event(wxEVT_PLAYBACK_TICK_EVENT, GetId());
  event.SetEventObject(this);
  event.SetInt(*p_stepSize > 0 ? 1 : 0); // indicates direction
  event.SetExtraLong(*p_currentStep);
  GetEventHandler()->ProcessEvent(event);
}


void PlaybackControl::SendLoopEvent(const bool& isForward)
{
  wxCommandEvent event(wxEVT_PLAYBACK_LOOP_EVENT, GetId());
  event.SetEventObject(this);
  event.SetInt(isForward ? 1 : 0);
  event.SetExtraLong(*p_currentStep);
  GetEventHandler()->ProcessEvent(event);
}


void PlaybackControl::OnTimer(wxTimerEvent& event)
{
  DoTimer(p_play->GetValue());
}


void PlaybackControl::DoTimer(const bool& allowLoop)
{
  if (*p_stepSize > 0)
    while (++(*p_currentStep) % *p_stepSize != 0);
  else
    while (--(*p_currentStep) % *p_stepSize != 0);

  if (*p_currentStep <= *p_startIndex) {
    if (!allowLoop) {
      ClearPlayToggles();
      UpdateTooltips();
      *p_currentStep = *p_startIndex;
      SendTickEvent();
    } else { // allowLoop
      if (*p_currentStep < *p_startIndex) {
        *p_currentStep = *p_stopIndex;
        SendLoopEvent(false);
      } else {
        SendTickEvent();
      }
    }
  }
  else if (*p_currentStep >= *p_stopIndex) {
    if (!allowLoop) {
      ClearPlayToggles();
      UpdateTooltips();
      *p_currentStep = *p_stopIndex;
      SendTickEvent();
    } else { // allowLoop
      if (*p_currentStep > *p_stopIndex) {
        *p_currentStep = *p_startIndex;
        SendLoopEvent(true);
      } else {
        SendTickEvent();
      }
    }
  }
  else {
    SendTickEvent();
  }

  if (p_play->GetValue() || p_start->GetValue() || p_end->GetValue()) {
    p_timer->Start(*p_delay, wxTIMER_ONE_SHOT);
  }
}


void PlaybackControl::OnStartClick(wxCommandEvent& event)
{
  ClearPlayToggles(p_start);
  if (event.IsChecked()) {
    if (*p_stepSize > 0)
      *p_stepSize = -*p_stepSize;
    if (!p_timer->IsRunning())
      DoTimer(false);
  }
  UpdateTooltips();
  event.Skip();
}


void PlaybackControl::OnPreviousClick(wxCommandEvent& event)
{
  Pause();
  if (*p_stepSize > 0)
    *p_stepSize = -*p_stepSize;
  DoTimer(true);
  UpdateTooltips();
  event.Skip();
}


void PlaybackControl::OnPlayClick(wxCommandEvent& event)
{
  ClearPlayToggles(p_play);
  if (event.IsChecked()) {
    if (*p_stepSize < 0)
      *p_stepSize = -*p_stepSize;
    if (!p_timer->IsRunning())
      DoTimer(true);
  }
  UpdateTooltips();
  event.Skip();
}


void PlaybackControl::OnStopClick(wxCommandEvent& event)
{
  Pause();
  *p_currentStep = *p_startIndex;
  SendTickEvent();
  UpdateTooltips();
  event.Skip();
}


void PlaybackControl::OnNextClick(wxCommandEvent& event)
{
  Pause();
  if (*p_stepSize < 0)
    *p_stepSize = -*p_stepSize;
  DoTimer(true);
  UpdateTooltips();
  event.Skip();
}


void PlaybackControl::OnEndClick(wxCommandEvent& event)
{
  ClearPlayToggles(p_end);
  if (event.IsChecked()) {
    if (*p_stepSize < 0)
      *p_stepSize = -*p_stepSize;
    if (!p_timer->IsRunning())
      DoTimer(false);
  }
  UpdateTooltips();
  event.Skip();
}


void PlaybackControl::OnRecordClick(wxCommandEvent& event)
{
  UpdateTooltips();
  event.Skip();
}


void PlaybackControl::UpdateTooltips()
{
  p_start->SetToolTip(p_start->GetValue() ? TIP_START_CHECKED :
                                            TIP_START_UNCHECKED);
  p_previous->SetToolTip(TIP_PREVIOUS);
  p_play->SetToolTip(p_play->GetValue() ? TIP_PLAY_CHECKED :
                                          TIP_PLAY_UNCHECKED);
  p_stop->SetToolTip(TIP_STOP);
  p_next->SetToolTip(TIP_NEXT);
  p_end->SetToolTip(p_end->GetValue() ? TIP_END_CHECKED :
                                        TIP_END_UNCHECKED);
  p_record->SetToolTip(p_record->GetValue() ? TIP_RECORD_CHECKED :
                                              TIP_RECORD_UNCHECKED);
}

