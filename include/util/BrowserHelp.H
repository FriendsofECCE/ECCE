/**
 * @file 
 *
 *
 */
#ifndef BROWSERHELP_HH
#define BROWSERHELP_HH

#include <string>
using std::string;


class Preferences;


/**
 * BrowserHelp invokes the browser to show Ecce help pages.
 * It maps keys which are usually something like window names to
 * urls which are managed in a text file.
 * See $ECCE_DATA/config/help.urls.
 *
 */
class BrowserHelp
{

public:
        BrowserHelp();

        virtual ~BrowserHelp();

        string URL(const string& key) const;

        void showHelpPage(const string& key) const;
        void showFeedbackPage(void) const;
        void showPage(const string& url) const;

protected:

        void initialize();
        static void displayURL(const string& url,bool new_window=false);

private:

        static string p_helpCmd;
        static string p_helpFile;
        static string p_urlPrefix;
        static string p_filePrefix;
        static Preferences *p_urlTranslations;


};

#endif
