#ifndef PLAYBACKEXTRAS__H
#define PLAYBACKEXTRAS__H

#include <string>
#include <wx/panel.h>

#include "PlaybackLoop.H"

class PlaybackControl;
class wxCheckBox;
class wxChoice;
class wxTextCtrl;
class wxSizer;

/**
 * The playback delay (ms per step) and Repeat choice under the playback buttons
 * of the Relaxed Scan and IRC Path panels.
 */
class PlaybackExtras
{
   public:
      /** Adds the row to <sizer>; <delay> is <control>'s delay, <repeat>
          the Repeat choice to start with. */
      PlaybackExtras(wxWindow *parent, wxSizer *sizer, int *delay,
                     PlaybackControl *control, playbackloop::Repeat repeat,
                     bool withMo = false);

      /** The "Show MO while playing" check box (IRC Path only; off by default). */
      void setShowMo(bool on);
      bool showMo() const;
      /** Greys the box out, with <why> as its tooltip (no per-point orbitals). */
      void disableMo(const std::string& why);
      bool moEnabled() const;

      /** Milliseconds per step, 1 to 10000; false for anything else. */
      bool setSpeed(const std::string& name);
      std::string speed() const;
      void setRepeat(playbackloop::Repeat r);
      playbackloop::Repeat repeat() const;

   private:
      void onSpeed();
      wxTextCtrl *p_speed;
      wxChoice *p_repeat;
      PlaybackControl *p_control;
      wxCheckBox *p_mo = 0;
      int *p_delay;
};

#endif
