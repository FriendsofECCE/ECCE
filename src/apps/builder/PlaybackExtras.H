#ifndef PLAYBACKEXTRAS__H
#define PLAYBACKEXTRAS__H

#include <string>
#include <wx/panel.h>

class wxCheckBox;
class wxTextCtrl;
class wxSizer;

/**
 * The playback delay (ms per step) and Loop toggle under the playback buttons of
 * the Relaxed Scan and IRC Path panels.
 */
class PlaybackExtras
{
   public:
      /** Adds the row to <sizer>; <delay> is the playback control's delay. */
      PlaybackExtras(wxWindow *parent, wxSizer *sizer, int *delay);

      /** Milliseconds per step, 1 to 10000; false for anything else. */
      bool setSpeed(const std::string& name);
      std::string speed() const;
      void setLoop(bool on);
      bool loop() const;

   private:
      void onSpeed();
      wxTextCtrl *p_speed;
      wxCheckBox *p_loop;
      int *p_delay;
};

#endif
