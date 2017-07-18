#include <iostream>
#include <strstream>

using namespace std;

#include "util/CmdLineAuthListener.H"

int main(int argc, char** argv)
{

  CmdLineAuthListener clal;

  AuthEvent event;
  event.m_realm = "no realm";
  event.m_retryCount = 0;

  clal.getAuthorization(event);

  cout << "credentials: " << event.m_user << " " << event.m_password << endl;

  return 0;
}
