#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <iomanip.h>    // for setw()
#include <sched.h>
#include <iostream>
  using namespace std;

#include "util/JMSPublisher.H"

int main(int argc, char** argv) {

  cout << "in execer" << endl;
  cout << "pid = " << getpid() << endl;
  JMSPublisher pub("execer");
  
  char* argvp[4] = {0,0,0,0};
  char path[] = "/bin/sh";
  argvp[0] = "/bin/sh";
  argvp[1] = "-c";
  argvp[2] = "basistool";

  pid_t pid; 
  if ( (pid = fork()) == 0) {
    cout << "going to call execv" << endl;  
    signal(SIGCHLD, SIG_IGN);
    execv(path, argvp);
    perror("Exec failed!");

  } else if (pid > 0) {
    
    cerr << "child pid =  " << pid << endl;
    cerr << "parent = " << getpid() << endl;
    
    // Can't wait with WNOHANG or the child process terminates
    int status = waitpid(pid, 0, 0);

    // print out diagnostic info on why child died
    if(WIFEXITED(status)) {
      cout << pid << " terminated normally" << endl;
    } else if (WIFSIGNALED(status)) {
      cout << pid << " terminated abnormally w/ signal " 
           << WTERMSIG(status) << endl;
    } else if (WIFSTOPPED(status)) {
      cout << pid << " is currently stopped by signal "
           << WSTOPSIG(status);
    }
    
    cout << getpid() << " terminating" << endl;
  
  } else {
    
    cerr << "Fork failed!" << endl;
  }
  signal(SIGCHLD, SIG_DFL);
  return 0;
}

