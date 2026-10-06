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

 *    mark-size | expect size-kept 1   the frame is not smaller than at mark-size
 *    click FIELD    a button or checkbox by name, e.g. blk:setup:copy, edit-file, raw:save
 *    shot-dialog FILE | expect raw-dialog 0|1 | contains FIELD TEXT   the Advanced: edit file dialog
 *    (set accepts \\n for a line break in the multi-line boxes)
 *    click help     the Help button; records the URL it would open (expect message)
 *    undo KEY      the undo button of a Connection row (jobs: the radios)
 *    code NAME | expect code-listed NAME 0|1   the Codes tab's list
 *    focus FIELD | cursor FIELD POS | words | words-pick WORD | words-activate WORD
 *      | click words:insert   insert a placeholder at the cursor of the last box
 *    pick FIELD ITEM...   tick exactly these rows of a check list (disc:list)
 *    scroll FIELD TEXT (a text box to the first line holding TEXT)
 *    save-field FIELD FILE | expect checked FIELD a,b | expect tool-dialog 0|1
 *    click discover | disc:add | disc:cancel | preview | code:preview | prev:update
 *      | prev:close | test-submission | test:run | test:close   (#212 tools; their
 *      fields are disc:*, prev:*, test:*; the dialogs are not modal here)
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
        bool shot(const std::string& file, bool dialog = false);
        void finish();

        wxWeakRef<wxWindow>      p_frameRef;
        WxMachineRegister       *p_frame;
        wxTimer                  p_timer;
        std::deque<std::string>  p_cmds;
        int                      p_failures;
        bool                     p_loaded;
        wxSize                   p_size;
};

#endif
