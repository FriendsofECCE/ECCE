#include <iostream.h>
#include <fstream.h>
#include <unistd.h>

#include "dsm/GLQuery.H"

int main(int argc, char** argv)
{

  const GLQuery& query = GLQuery::getGLQuery();



  string server;
  int major, minor;
  int result;
  while (cin.good()) {
    major = minor = -1;


    cout << "\nServer: ";
    cout.flush();
    cin >> server;

    cout << "Major: ";
    cout.flush();
    cin >> major;

    cout << "Minor: ";
    cout.flush();
    cin >> minor;

    result = query.getLimit(server, major,minor);
    cout << "result is " << result << endl;
  }



  return 0;
}
