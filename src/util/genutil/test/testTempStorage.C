#include <iostream>
  using namespace std;

#include <unistd.h>

#include "util/TempStorage.H"
#include "util/SFile.H"
#include "util/SDirectory.H"

int main(int argc, char** argv)
{

   SFile *file;
   SDirectory *dir;

   cout << "Making temp file with no prefix " << endl;
   file = TempStorage::file(0);
   cout << "file is " << file->path() << endl;

   cout << "Making temp file with prefix ECCE" << endl;
   file = TempStorage::file("ECCE");
   cout << "file is " << file->path() << endl;

   cout << "Making temp dir with no prefix" << endl;
   dir = TempStorage::directory(0);
   cout << "dir is " << dir->path() << endl;

   cout << "Making temp dir with prefix ECCE" << endl;
   dir = TempStorage::directory("ECCE");
   cout << "dir is " << dir->path() << endl;

  return 0;
}
