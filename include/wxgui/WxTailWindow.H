#ifndef WXTAILWINDOW_H_
#define WXTAILWINDOW_H_

#include <string>

#include <wx/timer.h>

#include "wxgui/ewxFrame.H"
#include "comm/TailSource.H"

class wxTextCtrl;
class ewxButton;
class ewxStaticText;

/**
 * Run Management's Tail: a calculation's output file as it grows, read over
 * ECCE's own connection (TailSource), so that following a job never asks for
 * a second login.  Read-only; Pause holds the view while lines keep arriving,
 * and only the last maxLines lines are kept.
 */
class WxTailWindow : public ewxFrame
{
public:
  static const int DEFAULT_LINES = 5000;

  // Connects and opens the window, or returns 0 with error set.  The window
  // deletes itself when closed.
  static WxTailWindow* open(wxWindow* parent, const std::string& title,
                            const std::string& machineName,
                            const std::string& shell, const std::string& user,
                            const std::string& path, std::string& error,
                            int maxLines = DEFAULT_LINES);

  const std::string& backend() const { return p_source.backend(); }
  int lineCount() const { return p_lines; }
  std::string text() const;
  // The window's client area as a PNG.
  bool snapshot(const std::string& png);

private:
  WxTailWindow(wxWindow* parent, const std::string& title,
               const std::string& path, int maxLines);

  void OnTimer(wxTimerEvent& event);
  void OnPause(wxCommandEvent& event);
  void OnCloseButton(wxCommandEvent& event);
  void OnClose(wxCloseEvent& event);

  void append(const std::string& data);
  void showPending();
  void setStatus();

  TailSource p_source;
  std::string p_path;
  int p_maxLines;
  int p_lines;            // complete lines shown
  std::string p_pending;  // received while paused
  bool p_paused, p_ended, p_missing;
  wxTimer p_timer;
  wxTextCtrl* p_text;
  ewxButton* p_pause;
  ewxStaticText* p_status;
};

#endif
