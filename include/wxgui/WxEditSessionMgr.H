/**
 * @file
 *
 *
 */
#ifndef _WXEDITSESSIONMGR_H
#define _WXEDITSESSIONMGR_H


#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/EcceException.H"
#include "util/EditSession.H"

class SFile;
class EditListener;
class WxEditTimer;

/**
 * A class that manages user edit sessions with their favorite editor.
 */
class WxEditSessionMgr 
{    
   public:
      WxEditSessionMgr( );
      virtual ~WxEditSessionMgr( );

      static void stopAll();
      static bool isEditSessionInProgress();

      void edit(SFile* file, const string& id, EditListener *l,
            bool readonly=true, const string & name= 0) throw(EcceException);

      void edit(const string& text, const string& id, EditListener *l,
            bool readonly=true, const string & name= 0) throw(EcceException);


   protected:

      void startSession(const string& app, /*const*/ char *args[],
              SFile* file, const string& id, EditListener *l);

      static void editSessionCompleted(int pid);

      //static void processDeadChild(XtPointer clientData, XtIntervalId *);
      //static void checkFileWrite(XtPointer clientData, XtIntervalId *);

      void editFile(SFile* file, const string& id, EditListener *l,
            bool readOnly=true,const string & name=0) throw(EcceException);


      SFile* makeTemporaryFile(const string& data);

      static vector<EditSession> p_sessions;

      friend class WxEditTimer;


};



#endif
