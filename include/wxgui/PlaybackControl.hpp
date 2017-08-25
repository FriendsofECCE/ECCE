#ifndef PLAYBACKCONTROL_H_
#define PLAYBACKCONTROL_H_

#include <wx/timer.h>

#include "wxgui/ewxPanel.H"
#include "wxgui/ThingToggle.H"


DECLARE_EVENT_TYPE(wxEVT_PLAYBACK_TICK_EVENT, -1)
DECLARE_EVENT_TYPE(wxEVT_PLAYBACK_LOOP_EVENT, -1)


class PlaybackControl : public ewxPanel
{
  DECLARE_DYNAMIC_CLASS(PlaybackControl)
  DECLARE_EVENT_TABLE()

  public:

    static const wxWindowID ID_TIMER;
    static const wxWindowID ID_START;
    static const wxWindowID ID_PREVIOUS;
    static const wxWindowID ID_PLAY;
    static const wxWindowID ID_STOP;
    static const wxWindowID ID_NEXT;
    static const wxWindowID ID_END;
    static const wxWindowID ID_RECORD;

    // constructors
    PlaybackControl();
  
    PlaybackControl(wxWindow* parent, wxWindowID id = -1,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxTAB_TRAVERSAL,
             const wxString& name = "PlaybackControl");

    virtual ~PlaybackControl();
  
    virtual bool Create(wxWindow* parent, wxWindowID id = -1,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxTAB_TRAVERSAL,
                        const wxString& name = "PlaybackControl");

    // Setters
    void SetDelay(const int& val);
    void SetStartIndex(const long& val);
    void SetStopIndex(const long& val);
    void SetStepSize(const long& val);
    void SetCurrentStep(const long& val);

    void SetDelay(int * val);
    void SetStartIndex(long * val);
    void SetStopIndex(long * val);
    void SetStepSize(long * val);
    void SetCurrentStep(long * val);

    // Getters
    bool IsPlaying() const { return p_timer->IsRunning(); }
    bool IsRecording() const { return p_record->GetValue(); }
    int GetDelay() const { return *p_delay; }
    long GetStartIndex() const { return *p_startIndex; }
    long GetStopIndex() const { return *p_stopIndex; }
    long GetStepSize() const { return *p_stepSize; }
    long GetCurrentStep() const { return *p_currentStep; }

    void Pause();

  protected:
    void ClearPlayToggles(wxWindow * keep=NULL);
    void SendTickEvent();
    void SendLoopEvent(const bool& isForward);

    void OnTimer(wxTimerEvent& event);
    void DoTimer(const bool& allowLoop);
    void OnStartClick(wxCommandEvent& event);
    void OnPreviousClick(wxCommandEvent& event);
    void OnPlayClick(wxCommandEvent& event);
    void OnStopClick(wxCommandEvent& event);
    void OnNextClick(wxCommandEvent& event);
    void OnEndClick(wxCommandEvent& event);
    void OnRecordClick(wxCommandEvent& event);

    void UpdateTooltips();

    wxTimer * p_timer;
    int * p_delay;
    long * p_startIndex;
    long * p_stopIndex;
    long * p_stepSize;
    long * p_currentStep;

    bool p_delayInternal;
    bool p_startIndexInternal;
    bool p_stopIndexInternal;
    bool p_stepSizeInternal;
    bool p_currentStepInternal;

    wxCustomButton * p_start;
    wxCustomButton * p_previous;
    wxCustomButton * p_play;
    wxCustomButton * p_stop;
    wxCustomButton * p_next;
    wxCustomButton * p_end;
    wxCustomButton * p_record;
};

#endif // PLAYBACKCONTROL_H_
