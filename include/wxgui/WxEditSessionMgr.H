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
class WxEditProcess;
class WxEditWatcher;

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
            bool readonly=true, const string & name= 0);

      void edit(const string& text, const string& id, EditListener *l,
            bool readonly=true, const string & name= 0);


   protected:

      void startSession(const string& app, /*const*/ char *args[],
              SFile* file, const string& id, EditListener *l);


      static bool notifyIfChanged(EditSession& s);
      static void checkSession(int pid);
      static void finishSession(EditSession& s);

      void editFile(SFile* file, const string& id, EditListener *l,
            bool readOnly=true,const string & name=0);


      SFile* makeTemporaryFile(const string& data);

      static vector<EditSession> p_sessions;

      friend class WxEditProcess;
      friend class WxEditWatcher;


};



#endif
