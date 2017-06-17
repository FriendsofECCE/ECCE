#ifndef PREFLABELS_HH
#define PREFLABELS_HH

///////////////////////////////////////////////////////////////////////////////
// Global Preferences
///////////////////////////////////////////////////////////////////////////////
class PrefLabels {
  public:
static const char *GLOBALPREFFILE;

static const char *COLORPREFFILE;

static const char *GATEWAYPREFFILE;

// RunState keys - these are booleans
static const char *stateCreated     ;
static const char *stateReady       ;
static const char *stateSubmitted   ;
static const char *stateRunning     ;
static const char *stateCompleted   ;
static const char *stateKilled      ;
static const char *stateUnsuccessful;
static const char *stateFailed      ;
static const char *stateLoaded      ;
static const char *stateSystem      ;

static const char *FONTSIZE         ;
static const char *COLORTHEME       ;
static const char *DATEFORMAT       ;
static const char *TIMEFORMAT       ;

// %g doesn't work that well - the precision is the total number of digits
// printed.  The width is only used for padding.  For most numbers,
// doubleFormat works fine.  However for some output values we users want
// lots of precision, use the preciseDoubleFormat.  According to Robert
// H., everything after 10 decimals is junk.
static const char *DOUBLEFORMAT       ;
static const char *PRECISEDOUBLEFORMAT ;

static const char *ERRORBEEP        ;
static const char *WARNINGBEEP      ;
static const char *CONFIRMEXIT      ;
static const char *FOCUSFOLLOWMOUSE ;
static const char *LEFTCLICKNEWAPP  ;
static const char *CLOSESHELLS      ;
static const char *SAVEPASSWORDS    ;
static const char *SHOWBUSY         ;
static const char *UNITFAMILY       ;
static const char *ALWAYSONTOP      ;
static const char *ORIENTATION      ;


/*
static const char *energyUnit       ;
static const char *distanceUnit     ;
static const char *frequencyUnit    ;
*/

static const char *bstTemplateDir   ;

///////////////////////////////////////////////////////////////////////////////
// Periodic Table Element Colors
///////////////////////////////////////////////////////////////////////////////
static const char *atomPrefFile  ;


static const char *fontLabel;
static const char *fontLabelNonbold;
static const char *fontLabelSmall;
static const char *fontLabelBig;
static const char *fontBanner;
static const char *fontTextTypein;
static const char *fontFixedWidth;
static const char *fontFixedNonbold;
static const char *fontUnimplemented;

};

#endif /* PREFLABELS_HH */
