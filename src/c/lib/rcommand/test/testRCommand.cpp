#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

//#include <rw/cstring.h>

#include "expect.h"
#include "comm/RCommand.H"

#include "util/EcceRegexp.H"

int main(int argc, char** argv)
{
#if 000
  string output;
  bool status;
#endif

#if 111
  {
    int numit = 1;
    if (argc > 1)
      numit = atoi(argv[1]);
    RCommand* rcmd;
    for (int it=0; it<numit; it++) {
      rcmd = new RCommand("system");
      if (rcmd->isOpen())
        cout << "spawned csh #" << it << endl;
      else {
        cout << "************** FAILED spawning csh #" << it << endl;
        cout << "commError (" << rcmd->commError() << ")" << endl;
        break;
      }
    }
  }
#endif
#if 000
  {
    string msg;
    status = RCommand::get(msg, "eccetude.pnl.gov", "sftp", "ecce_cm",
                           "fakepass", 2,
                           "/tmp/ecce_archive/Study/Dynamics2/*.trj",
                           "/tmp/gary");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << msg << endl;
    }
}
#endif
#if 000
  {
    string msg;
    //status = RCommand::get(msg, "aeneas.mit.edu", "telnet", "anonymous",
    //                       "gary.black@pnl.gov", 3, "/pub/gnu/webstump.README",
    //                       "/pub/gnu/w3.README", "/tmp");
    status = RCommand::put(msg, "aeneas.mit.edu", "telnet", "anonymous",
                           "gary.black@pnl.gov", 3, 
                           "eccejobmaster", "eccejobstore", "/home/ecce1/newdir/twodir/threedir");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << msg << endl;
    }
  }
#endif
#if 000
  {
    string msg;
    status = RCommand::put(msg, "n2.emsl.pnl.gov", "sftp", "d3j766",
                           "fakepass", 3, 
                           "eccejobmaster", "eccejobstore", "/nwfs/d3j766/test_put");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << msg << endl;
    }
    cout << "done testput" << endl;
  }
#endif
#if 000
  RCommand rcmd("eccentric.pnl.gov", "ssh", "csh", "ecce1", "fakepass");
  if (rcmd.isOpen()) {
    cout << "eccentric connection established!" << endl;
    rcmd.execout("hostname", output);
    cout << "first hostname (" << output << ")" << endl;
    if (rcmd.hop("booboo.pnl.gov", "csh", "ecce1", "fakepass")) {
      cout << "hop to booboo established!" << endl;
      rcmd.execout("hostname", output);
      cout << "second hostname (" << output << ")" << endl;
    }else
      cout << "hop to booboo failed!" << endl;
  }
#endif
#if 000
#if 111
    //RCommand rcmd("mpp2.emsl.pnl.gov", "ssh", "csh", "d39842", "fakepass");
    RCommand rcmd("booboo.pnl.gov", "ssh", "csh", "ecce1", "fakepass");
    if (rcmd.isOpen()) {
      cout << "eccentric connection established!" << endl;
      char *fromFiles[10];
      for (int it=1; it<argc; it++)
        fromFiles[it-1] = strdup(argv[it]);
      fromFiles[argc-1] = NULL;
      //rcmd.shellput((const char**)fromFiles, "/dtemp/d39842/xfer");
      rcmd.shellput((const char**)fromFiles, "/tmp/xfer");
    }
#else
    string errmsg;
    char *fromFiles[10];
    for (int it=1; it<argc; it++)
      fromFiles[it-1] = strdup(argv[it]);
    fromFiles[argc-1] = NULL;
    status = RCommand::put(errmsg,
                           "eccentric.pnl.gov", "ssh", "ecce1", "fakepass",
                           (const char**)fromFiles, "/tmp/xfer");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << errmsg << endl;
    }
#endif
#endif
#if 000
#if 111
    //RCommand rcmd("mpp2.emsl.pnl.gov", "ssh", "csh", "d39842", "fakepass");
    RCommand rcmd("mpp2.emsl.pnl.gov", "ssh", "csh", "d39842", "fakepass",
                  "", "", "", "m437");
    if (rcmd.isOpen()) {
      cout << "eccentric connection established!" << endl;
      char *fromFiles[10];
      for (int it=1; it<argc; it++)
        fromFiles[it-1] = strdup(argv[it]);
      fromFiles[argc-1] = NULL;
      rcmd.shellget((const char**)fromFiles, "/tmp/xfer");
    }
#else
    string errmsg;
    char *fromFiles[2];
    fromFiles[0] = strdup(argv[1]);
    fromFiles[1] = NULL;
    status = RCommand::get(errmsg,
                           "eccentric.pnl.gov", "ssh", "ecce1", "fakepass",
                           (const char**)fromFiles, "/tmp/xfer");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << errmsg << endl;
    }
#endif
#endif
#if 000
  {
    RCommand rcmd("mpp2.emsl.pnl.gov", "ssh", "csh", "d39842", "fakepass",
                  "", "", "", argv[1]);
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    status  = rcmd.execout("uname -a", output);
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    cout << "output (" << output << ")" << endl;
  }
#endif
#if 000
  {
#if 000
    int count = 100;
    char **fileStrs = (char**)malloc((count+1)*sizeof(char*));
    char buf[64];
    for (int idx = 0; idx < count; idx++) {
      sprintf(buf, "/tmp/ecce1/Study-6/Dynamics1/gary%d.rst", idx+1);
      fileStrs[idx] = strdup(buf);
    }
#else
      char **fileStrs = (char**)malloc(2*sizeof(char*));
      fileStrs[0] = strdup("/tmp/ecce1/Study-6/Dynamics1/gary\*.rst");
      fileStrs[1] = 0;
#endif

    string errmsg;
    status = RCommand::get(errmsg,
                           "eccetude.pnl.gov", "ssh", "ecce1", "fakepass",
                           (const char**)fileStrs, "/tmp/gary");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << errmsg << endl;
    }
  }
#endif
#if 000
  {
    string msg;
    status = RCommand::put(msg, "eccentric.pnl.gov", "ssh", "ecce1",
                           "fakepass", 3, 
                           "/home/d39842/ecce/src/libs/comm/rcommand/test/testRCommand.C", "/home/d39842/ecce/src/libs/comm/rcommand/test/Makefile", "/msrc/home/ecce1");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << msg << endl;
    }
    cout << "done testput" << endl;
  }
#endif
#if 000
    RCommand* rcmd = new RCommand("system");
    delete rcmd;
#endif
#if 000
  {
    string err;
#if 111
    status = RCommand::command("xterm -e csh -i", true, output, err,
                      "rocky.chpc.utah.edu", "ssh", "black", "decme");
#else
    status = RCommand::command("who", false, output, err,
                      "ecs.ecs1.emsl.pnl.gov", "ssh", "ecce1", "born2run");
#endif
    if (status) {
      cout << "status: TRUE" << endl;
      cout << output << endl;
    }
    else
      cout << "status: FALSE" << endl;
  }
#endif
#if 000
  {
    RCommand rcmd("inca.chpc.utah.edu", "ssh", "black", "Jubathehut");
    if (rcmd.isOpen()) {
      cout << "Established inca ssh connection!" << endl;
      status = rcmd.execout("who", output);
      if (status) {
        cout << "status: TRUE" << endl;
        cout << output << endl;
      }
      else
        cout << "status: FALSE" << endl;
    }
    else
      cout << "Unable to establish inca ssh connection" << endl;
  }
#endif
#if 000
  {
    status = RCommand::get("x01n01.ecs1.emsl.pnl.gov", "ssh", "ecce1",
                           "fakepass", 3,
                           "/x/ecce1/gary.proj/Project/ecs1_9_4_9/nwch.out",
                           "/x/ecce1/gary.proj/Project/ecs1_9_4_9/ecce.out",
                           "/msrc/home/d39842/Ecce/gary.proj/Project/ecs1_9_4_9");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << RCommand::commError() << endl;
    }
  }
#endif
#if 000
  {
    output = "hello";
    status = RCommand::globusrun(
                      "$(GLOBUS_INSTALL_PATH)/bin/globus-tools-path", output,
                      "peace.emsl.pnl.gov", "abc123");

    cout << "out (" << output << ")" << endl;
  }
#endif
#if 000
  {
    status = RCommand::get("goofey.emsl.pnl.gov", "telnet", "ecce1",
                           "fakepass", 3, "/msrc/home/ecce1/cool.ps",
                           "/msrc/home/ecce1/fort.7", "/msrc/home/d39842");
    if (status)
      cout << "status: TRUE" << endl;
    else {
      cout << "status: FALSE" << endl;
      cout << RCommand::commError() << endl;
    }
  }
#endif
#if 000
  {
    RCommand rcmd("goofey.emsl.pnl.gov", "telnet", "ecce1", "fakepass");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    else {
      cout << "working bad!" << endl;
      cout << "got line (" << rcmd.expout() << ")" << endl;
    }
#if 000
    status = rcmd.execout("tcsh -fc 'perl /msrc/tmp/timeout.pl'", output);
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    cout << "output (" << output << ")" << endl;
#endif
  }
#endif
#if 000
  {
    RCommand rcmd("yawl.emsl.pnl.gov", "ssh", "ecce1", "fakepass");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    status  = rcmd.execout("uname -a", output);
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    cout << "output (" << output << ")" << endl;
  }
#endif
#if 000
  {
    RCommand rcmd("lionheart.emsl.pnl.gov", "rsh", "ecce1", "fakepass");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    rcmd.exptimeout(3);
    status = rcmd.execbg("sleep 2; echo 'this is bg process output'");
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    switch (rcmd.expect(1, "\r\n")) {
      case 1:
        cout << "got line (" << rcmd.expout() << ")" << endl;
        break;
      case EXP_TIMEOUT:
        cout << "got timeout" << endl;
        break;
      default:
        cout << "got bad pattern match" << endl;
        break;
    }
  }
#endif
#if 000
  {
    RCommand* rcmd;
    for (int it=0; it<1000; it++) {
      rcmd = new RCommand("system");
      if (rcmd->isOpen())
        cout << "spawning csh #" << it << endl;
      else {
        cout << "************** FAILED spawning csh #" << it << endl;
        cout << "commError (" << rcmd->commError() << ")" << endl;
        break;
      }
    }
  }
#endif
#if 000
  {
    char fileName[32];
    ofstream *file;
    for (int it=0; it<1000; it++) {
      sprintf(fileName, "toss%d.out", it);
      file = new ofstream(fileName);
      if (*file)
        cout << "opened file (" << fileName << ")" << endl;
      else {
        cout << "************** FAILED opening file (" << fileName << ")" << endl;
        break;
      }
    }
  }
#endif
#if 000
    RCommand rcmd("goofey", "ssh", "ecce1", "fakepass");
    if (rcmd.isOpen()) {
      cout << "working great!" << endl;
      status = rcmd.execout("echo $TERM", output);
      if (status)
        cout << "status: TRUE" << endl;
      else
        cout << "status: FALSE" << endl;
      cout << "output (" << output << ")" << endl;
    }
    else
      cout << "connection not open" << endl;
  }
#endif
#if 000
  {
    RCommand rcmd("mpp1", "telnet", "ecce1", "fakepass");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    status = rcmd.execout("echo $TERM", output);
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    cout << "output (" << output << ")" << endl;
  }
#endif
#if 000
  {
    RCommand rcmd("t3e.osc.edu", "telnet", "osc249", "gr8chess");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    status = rcmd.execout("echo $TERM", output);
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    cout << "output (" << output << ")" << endl;
  }
#endif
#if 000
  {
    RCommand rcmd("mcurie.nersc.gov", "telnet", "eccetera", "4globus");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    status = rcmd.execout("echo $TERM", output);
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    cout << "output (" << output << ")" << endl;
  }
#endif
#if 000
  status = RCommand::command("whoami", output,
                           "goofey.emsl.pnl.gov", "rsh", "ecce1", "");
  if (status)
    cout << "status: TRUE" << endl;
  else
    cout << "status: FALSE" << endl;
  cout << "output (" << output << ")" << endl;
  {
    RCommand rcmd("goofey.emsl.pnl.gov", "ssh", "eccecow", "doit4loc");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    status = rcmd.execout("whoami", output);
    if (status)
      cout << "status: TRUE" << endl;
    else
      cout << "status: FALSE" << endl;
    cout << "output (" << output << ")" << endl;
  }
#endif
#if 000
  {
    RCommand rcmd("goofey.emsl.pnl.gov", "ssh", "eccecow", "doit4loc");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    rcmd.exec("whoami");
  }
  {
    RCommand rcmd;
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    rcmd.exec("whoami");
  }
  {
    RCommand rcmd("fiatlux.emsl.pnl.gov", "rsh", "d39842", "");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    rcmd.exec("whoami");
  }
  {
    RCommand rcmd("fiatlux", "rsh", "d39842");
    if (rcmd.isOpen())
      cout << "working great!" << endl;
    rcmd.exec("whoami");
  }
#endif
#if 000
  RCommand::get("goofey.emsl.pnl.gov", "ssh", "eccecow", "doit4loc",
                3, "a*", "ecce_log", "/tmp");

  const char* fromFiles[3] = {"a*", "ecce_log", NULL};
  RCommand::get("goofey.emsl.pnl.gov", "ssh", "eccecow", "doit4loc",
                fromFiles, "/tmp");
#endif

  return 0;
}
