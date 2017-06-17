#include <iostream.h>
#include <unistd.h>

#include "util/EcceException.H"
#include "util/CancelException.H"
#include "util/ParseException.H"
#include "util/SystemCommandException.H"
#include "util/NullPointerException.H"
#include "util/IndexOutOfRangeException.H"

int main(int argc, char** argv)
{

   EcceException::setDefaultHandler();
   try {
      throw CancelException( __FILE__, __LINE__);
   } catch (EcceException& ex) {
      cout << "Caught exception: " << ex.what() << endl;
   }

   //NULLPOINTEREXCEPTION(0, "found null pointer");
   //throw CommandException(-3, "fix data", WHERE);
   throw IndexOutOfRangeException(0,10,-1,WHERE);

   return 0;
}
