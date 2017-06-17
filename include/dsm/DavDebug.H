#ifndef _DAVDEBUG_H
#define _DAVDEBUG_H

#include <string>
  using std::string;

class DavDebug {

  public:
    DavDebug(const char *file=0);
    virtual ~DavDebug();

         // assign context and thus output file so we can have separate
         // debug stacks for separate tools.  If this is never called, a
         // default (class static) will be used.
    static void setDebugContext(const char *context);

         // just dump the stream
    void dump(const string& stream, const char *description);

         // dump the stream but with timing info
    void startTimer();
    void endTimer(const char *description=0);
  private:
    char *p_file;
    unsigned long p_lasttime;

    static char *s_context;
};
#endif
