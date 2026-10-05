/**
 * @file
 *
 *  Test hook for Register Machines: ECCE_MACHREG_SCRIPT=<file> runs the
 *  commands in the file against the real frame, one per timer tick, and
 *  exits.  It calls the same handlers the buttons call, so a headless run
 *  (Xvfb) exercises the real window without synthetic X input.  Inert unless
 *  the variable is set.
 *
 *  Commands, one per line ('#' starts a comment, values may be quoted):
 *    select NAME | new | save | delete | close | tab NAME
 *    set FIELD VALUE | queue-apply | queue-remove | queue-clear
 *    answer yes|no|cancel      pre-answer the next prompt
 *    wait MS | snapshot DIR | shot FILE | dump | quit
 *    expect dirty|save-enabled|delete-enabled|title-star|banner 0|1
 *    expect field FIELD VALUE | list NAME FROM | tab NAME | message TEXT
 *    expect label FIELD TEXT | enabled FIELD 0|1 | shown FIELD 0|1
 *    undo KEY       the undo button of a Connection row (jobs: the radios)
 *  Source tags are fields named tag:KEY, e.g. expect label tag:shell site.
 */
#ifndef WXMACHINEREGISTERSCRIPT_HH
#define WXMACHINEREGISTERSCRIPT_HH

#include <deque>
#include <string>
#include <vector>

#include "wx/event.h"
#include "wx/timer.h"
#include "wx/weakref.h"

class WxMachineRegister;

class MachRegScript : public wxEvtHandler
{
    public:
        MachRegScript(WxMachineRegister* frame, const std::string& file);
        bool ok() const { return p_loaded; }
        void start();

    private:
        void onTimer(wxTimerEvent& event);
        int runCommand(const std::vector<std::string>& w);
        void fail(const std::string& what);
        void expectEq(const std::string& what, const std::string& got,
                      const std::string& want);
        std::string get(const std::string& field);
        bool click(const std::string& field);
        bool shot(const std::string& file);
        void finish();

        wxWeakRef<wxWindow>      p_frameRef;
        WxMachineRegister       *p_frame;
        wxTimer                  p_timer;
        std::deque<std::string>  p_cmds;
        int                      p_failures;
        bool                     p_loaded;
};

#endif
