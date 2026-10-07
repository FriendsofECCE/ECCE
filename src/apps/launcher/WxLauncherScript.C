/**
 * @file
 *
 *  Test hook for the Launcher; see the header.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dcmemory.h"

#include "WxLauncher.H"
#include "WxLauncherScript.H"
#include "wxgui/WindowShot.H"

using std::string;
using std::vector;

//  Words split on blanks; '...' and "..." keep their blanks.
static vector<string> words(const string& line)
{
    vector<string> out;
    size_t i = 0;
    while (i < line.size())
    {
        while (i < line.size() && isspace((unsigned char)line[i]))
            i++;
        if (i >= line.size() || line[i] == '#')
            break;
        string w;
        char q = 0;
        for (; i < line.size(); i++)
        {
            char c = line[i];
            if (q) { if (c == q) q = 0; else w += c; }
            else if (c == '\'' || c == '"') q = c;
            else if (isspace((unsigned char)c)) break;
            else w += c;
        }
        out.push_back(w);
    }
    return out;
}


LauncherScript::LauncherScript(WxLauncher* frame, const string& file)
    : p_frameRef(frame), p_frame(frame), p_timer(this), p_failures(0),
      p_loaded(false)
{
    std::ifstream in(file.c_str());
    p_loaded = in.good();
    string line;
    while (std::getline(in, line))
        if (!words(line).empty())
            p_cmds.push_back(line);
    Bind(wxEVT_TIMER, &LauncherScript::onTimer, this);
}


void LauncherScript::start()
{
    if (!p_loaded)
        fail("cannot read the script");
    p_timer.StartOnce(500);
}


void LauncherScript::fail(const string& what)
{
    p_failures++;
    fprintf(stderr, "[LAUNCHER] FAIL %s\n", what.c_str());
}


bool LauncherScript::shot(const string& file)
{
    return ecceWindowShot(p_frame, file);
}


void LauncherScript::finish()
{
    fprintf(stderr, "[LAUNCHER] script done failures=%d\n", p_failures);
    fflush(stderr);
    _exit(p_failures ? 1 : 0);
}


void LauncherScript::onTimer(wxTimerEvent&)
{
    if (p_cmds.empty() || p_frameRef.get() == NULL)
    {
        finish();
        return;
    }
    string line = p_cmds.front();
    p_cmds.pop_front();
    fprintf(stderr, "[LAUNCHER] > %s\n", line.c_str());
    int delay = runCommand(words(line));
    p_timer.StartOnce(delay);
}


int LauncherScript::runCommand(const vector<string>& w)
{
    const string& cmd = w[0];
    size_t n = w.size();

    if (cmd == "machine" && n == 2)
    {
        if (!p_frame->selectMachine(w[1]))
            fail("machine " + w[1] + ": not in the list");
    }
    else if (cmd == "queue" && n == 2)
    {
        if (!p_frame->selectQueue(w[1]))
            fail("queue " + w[1] + ": not in the list");
    }
    else if (cmd == "click" && n == 2 && w[1] == "machine-settings")
    {
        if (!p_frame->clickMachineSettings())
            fail("the Machine settings button is missing or disabled");
    }
    else if (cmd == "rundir" && n == 2)
        p_frame->setRunDirectory(w[1]);
    else if (cmd == "reload")
        p_frame->machRegChanged();
    else if (cmd == "exec" && n >= 2)
    {
        int rc = system(w[1].c_str());
        if (rc != 0)
            fail("exec '" + w[1] + "' returned " + std::to_string(rc));
    }
    else if (cmd == "expect" && n >= 3)
    {
        string got = p_frame->hookGet(w[1]);
        string want = w[2];
        if (got != want)
            fail("expect " + w[1] + ": got '" + got + "', wanted '" + want + "'");
        else
            fprintf(stderr, "[LAUNCHER] ok %s = '%s'\n", w[1].c_str(), got.c_str());
    }
    else if (cmd == "wait" && n == 2)
        return atoi(w[1].c_str());
    else if (cmd == "shot" && n == 2)
    {
        if (!shot(w[1]))
            fail("shot " + w[1]);
    }
    else if (cmd == "quit")
        finish();
    else
        fail("bad command: " + cmd);
    return 200;
}
