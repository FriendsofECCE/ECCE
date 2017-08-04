#include "util/PreferenceLabels.H"

const char *PrefLabels::GLOBALPREFFILE = "EcceGlobal";

const char *PrefLabels::COLORPREFFILE = "MyAppColors";

const char *PrefLabels::GATEWAYPREFFILE = "Gateway";

// RunState keys - these are booleans
const char *PrefLabels::stateCreated      = "RunState.Created";
const char *PrefLabels::stateReady        = "RunState.Ready";
const char *PrefLabels::stateSubmitted    = "RunState.Submitted";
const char *PrefLabels::stateRunning      = "RunState.Running";
const char *PrefLabels::stateCompleted    = "RunState.Completed";
const char *PrefLabels::stateKilled       = "RunState.Killed";
const char *PrefLabels::stateUnsuccessful = "RunState.Unsuccessful";
const char *PrefLabels::stateFailed       = "RunState.Failed";
const char *PrefLabels::stateLoaded       = "RunState.Loaded";
const char *PrefLabels::stateSystem       = "RunState.System";

const char *PrefLabels::FONTSIZE          = "FontSize";      // Int
const char *PrefLabels::COLORTHEME        = "Theme";         // String
const char *PrefLabels::DATEFORMAT        = "DateFormat";    // Int
const char *PrefLabels::TIMEFORMAT        = "TimeFormat";    // Int

// %g doesn't work that well - the precision is the total number of digits
// printed.  The width is only used for padding.  For most numbers,
// doubleFormat works fine.  However for some output values we users want
// lots of precision, use the preciseDoubleFormat.  According to Robert
// H., everything after 10 decimals is junk.
const char *PrefLabels::DOUBLEFORMAT        = "% 13.6lf";
const char *PrefLabels::PRECISEDOUBLEFORMAT  = "% 17.10lf";   

const char *PrefLabels::ERRORBEEP         = "ErrorBeep";     // Boolean
const char *PrefLabels::WARNINGBEEP       = "WarningBeep";    // Boolean
const char *PrefLabels::CONFIRMEXIT       = "Gateway.ConfirmExit";    // Boolean
const char *PrefLabels::FOCUSFOLLOWMOUSE  = "Gateway.FocusFollowMouse";  // Boolean
const char *PrefLabels::LEFTCLICKNEWAPP   = "Gateway.LeftClickNewApp"; // Boolean
const char *PrefLabels::CLOSESHELLS       = "Gateway.CloseShells"; // Boolean
const char *PrefLabels::SAVEPASSWORDS     = "Gateway.SavePasswords";  // Boolean
const char *PrefLabels::SHOWBUSY          = "Gateway.ShowBusy";    // Boolean
const char *PrefLabels::UNITFAMILY        = "UnitFamily";     // String
const char *PrefLabels::ALWAYSONTOP       = "Gateway.AlwaysOnTop"; // Boolean
const char *PrefLabels::ORIENTATION       = "Gateway.Orientation"; // Int


/*
const char *PrefLabels::energyUnit        = "Unit.Energy";    // String
const char *PrefLabels::distanceUnit      = "Unit.Distance";  // String
const char *PrefLabels::frequencyUnit     = "Unit.Frequency"; // String
*/

const char *PrefLabels::bstTemplateDir    = "BasisToolTemplates";

///////////////////////////////////////////////////////////////////////////////
// Periodic Table Element Colors
///////////////////////////////////////////////////////////////////////////////
const char *PrefLabels::atomPrefFile   = "atomcolors";


const char *PrefLabels::fontLabel = "-adobe-helv*-b*-r-*-12-*";
const char *PrefLabels::fontLabelNonbold = "-adobe-helv*-m*-r-*-12-*";
const char *PrefLabels::fontLabelSmall = "-adobe-helv*-m*-r-*-10-*";
const char *PrefLabels::fontLabelBig = "-adobe-helv*-b*-r-*-14-*";
const char *PrefLabels::fontBanner = "-adobe-helv*-b*-r-*-24-*";
const char *PrefLabels::fontTextTypein = "-adobe-helv*-b*-r-*-12-*";
const char *PrefLabels::fontFixedWidth = "-b&h-lucidatype*-b*-r-*-12-*";
const char *PrefLabels::fontFixedNonbold = "-b&h-lucidatype*-m*-r-*-12-*";
const char *PrefLabels::fontUnimplemented = "-adobe-helv*-b*-o-*-12-*";


