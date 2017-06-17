#include <iostream>
using std::cout;
using std::cin;
using std::endl;
using std::flush;

#include <unistd.h>
#include <termios.h>

#include "util/InvalidException.H"

#include "util/ErrMsg.H"
#include "util/EcceURL.H"
#include "util/CmdLineAuthListener.H"
#include "util/RetryException.H"
#include "util/NotImplementedException.H"

/**
  This class implements the AuthEventListener interface by using the
  authorization cache and prompting when necessary.

  A typical use would be to inherit from this class in any app that
  accesses dav.
 */
CmdLineAuthListener::CmdLineAuthListener() 
{
}


CmdLineAuthListener::CmdLineAuthListener(const CmdLineAuthListener& rhs) 
{
  throw NotImplementedException("CmdLineAuthListener copy constructor",WHERE);
}



CmdLineAuthListener::~CmdLineAuthListener()
{
}

// Prompt for the user name and password and return it.
bool CmdLineAuthListener::getAuthorization(AuthEvent& event) 
                                                   throw (RetryException)
{
  string user, password;

  // throw an exception after a completely arbitrary # of attempts
  if ((event.m_retryCount) > 3) {
    throw RetryException("Maximum retries exceeded (3)", WHERE);
  }

  EcceURL url(event.m_url);

  cout << endl;
  cout << "************************************* \n" << endl;
  cout << "Authentication required for : \n";
  cout << "\tServer:\t" << url.getHost() << endl;
  cout << "\tRealm:\t" << event.m_realm << endl;
  cout << endl;
  cout << "************************************* \n" << endl;

  // Get user name.
  cout << "User: " << flush;
  cin >> user;


  // Get password
  cout << "Password: " << flush;


  // Turn echo off
  struct termios tty, oldtty;
  tcgetattr(1, &oldtty);
  tty = oldtty;
  tty.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 0;
  tcsetattr(1, TCSANOW, &tty);

  cin >> password;

  // Turn echo back on
  tcsetattr(1, TCSANOW, &oldtty);

  cout << endl;


  // Set return info.
  event.m_user = user;
  event.m_password = password;


  return (!user.empty() && !password.empty());
}

//
// This classes implementation is a noop.
//
void CmdLineAuthListener::authorizationAccepted(const AuthEvent& event)
{

}
