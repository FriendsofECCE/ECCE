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
 *    pick NAME             choose a machine through the choice's own event
 *                          path (a read-only Launcher must not take it)
 *    editable 0|1          as a calculation in the Ready state (1) or not
 *    rundir PATH           fill the Run Directory field
 *    procs N               set the processors, as typed into the field
    click advanced        the Advanced fold button
    username NAME         type into the Username field
    launch                click Launch
 *    mpicheck CODE         Launch's missing-MPI check for CODE on the chosen
 *                          machine; logs "go" or "cancel"
 *    sizecheck             Launch's ECCE-QM size warning for the calculation
 *                          the Launcher was opened on; logs "go" or "cancel"
 *    launchwarn TEXT       Launch's dialog for a launch-check problem with this
 *                          text (a literal \n is a newline; a line
 *                          ECCE-ORIGINAL-MESSAGE starts the closed details
 *                          pane); logs "go" or "cancel"
 *    click machine-settings   the "Machine settings..." button
 *    reload                what a saved Register Machines causes
 *    exec COMMAND          run a shell command (for example a Register
 *                          Machines run that saves) and wait for it
 *    expect KEY VALUE      KEY: request | machine | machines | queues |
 *                          block | password | username | scratchdir |
 *                          advanced | rundir | connect | launchable |
 *                          queue | procs | wsprocs | procsmax | memory | memunits |
 *                          scratchunits | button
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
