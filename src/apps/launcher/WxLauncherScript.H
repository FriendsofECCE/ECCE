/**
 * @file
 *
 *  Test hook for the Launcher: ECCE_LAUNCHER_SCRIPT=<file> runs the commands
 *  in the file against the real frame, one per timer tick, and exits.  With
 *  it set the Launcher neither subscribes nor publishes, and the Machine
 *  settings button records the request it would send to the gateway instead
 *  of sending it.  Inert unless the variable is set.
 *
 *  Commands, one per line ('#' starts a comment, values may be quoted):
 *    machine NAME | queue NAME    choose, as the drop-downs do
 *    rundir PATH           fill the Run Directory field
    launch                click Launch
 *    click machine-settings   the "Machine settings..." button
 *    reload                what a saved Register Machines causes
 *    exec COMMAND          run a shell command (for example a Register
 *                          Machines run that saves) and wait for it
 *    expect KEY VALUE      KEY: request | machine | machines | queues |
 *                          queue | procs | procsmax | memory | button
 *    wait MS | shot FILE | quit
 */
#ifndef WXLAUNCHERSCRIPT_HH
#define WXLAUNCHERSCRIPT_HH

#include <deque>
#include <string>
#include <vector>

#include "wx/event.h"
#include "wx/timer.h"
#include "wx/weakref.h"

class WxLauncher;

class LauncherScript : public wxEvtHandler
{
    public:
        LauncherScript(WxLauncher* frame, const std::string& file);
        void start();

    private:
        void onTimer(wxTimerEvent& event);
        int runCommand(const std::vector<std::string>& w);
        void fail(const std::string& what);
        bool shot(const std::string& file);
        void finish();

        wxWeakRef<wxWindow>      p_frameRef;
        WxLauncher              *p_frame;
        wxTimer                  p_timer;
        std::deque<std::string>  p_cmds;
        int                      p_failures;
        bool                     p_loaded;
};

#endif
