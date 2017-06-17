/**
 * @file 
 *
 *
 */
#ifndef EWXNUMERICVALIDATOR_H
#define EWXNUMERICVALIDATOR_H

#include <string>
  using std::string;

#include "util/InvalidException.H"

#include "wxgui/ewxGenericValidator.H"


/**
 * wxValidator that validates numeric fields.
 * This code copied from wxTextValidator and then modified.  The
 * wxWidget coding style is maintained.
 */
class ewxNumericValidator : public ewxGenericValidator
{
  DECLARE_DYNAMIC_CLASS(ewxNumericValidator)
  DECLARE_EVENT_TABLE()

  public:
    ewxNumericValidator();
    ewxNumericValidator(double min, double max);
    ewxNumericValidator(double min, double max, double val);
    ewxNumericValidator(double min, double max, double * val);
    ewxNumericValidator(double min, double max, const string& val);
    ewxNumericValidator(int min, int max);
    ewxNumericValidator(int min, int max, int val);
    ewxNumericValidator(int min, int max, int * val);
    ewxNumericValidator(int min, int max, const string& val);
    ewxNumericValidator(const ewxNumericValidator& val);

    virtual ~ewxNumericValidator();

    virtual ewxNumericValidator& HardMin(int val);
    virtual ewxNumericValidator& HardMax(int val);
    virtual ewxNumericValidator& SoftMin(int val);
    virtual ewxNumericValidator& SoftMax(int val);
    virtual ewxNumericValidator& Value(int val);

    virtual ewxNumericValidator& HardMin(double min);
    virtual ewxNumericValidator& HardMax(double min);
    virtual ewxNumericValidator& SoftMin(double min);
    virtual ewxNumericValidator& SoftMax(double min);
    virtual ewxNumericValidator& Value(double min);

    virtual ewxNumericValidator& HardMinOpen(bool val);
    virtual ewxNumericValidator& HardMaxOpen(bool val);
    virtual ewxNumericValidator& SoftMinOpen(bool val);
    virtual ewxNumericValidator& SoftMaxOpen(bool val);

    virtual ewxNumericValidator& AllowEmpty(bool val);
    virtual ewxNumericValidator& IsInt(bool val);

    virtual void setValue(int* val);
    virtual void setValue(double* val);

    virtual void setValue(int val);
    virtual void setValue(double val);
         
    // Make a clone of this validator (or return NULL) - currently necessary
    // if you're passing a reference to a validator.
    // Another possibility is to always pass a pointer to a new validator
    // (so the calling code can use a copy constructor of the relevant class).
    virtual wxObject *Clone() const { return new ewxNumericValidator(*this); }

    bool Copy(const ewxNumericValidator& val);

    virtual bool isUsingHardRange() const { return p_useHardRange; }
    virtual void getHardRange(double& min, double& max);
    virtual void getHardRange(int& min, int& max);
    virtual bool isUsingSoftRange() const { return p_useSoftRange; }
    virtual void getSoftRange(double& min, double& max);
    virtual void getSoftRange(int& min, int& max);
    virtual bool isIntRange() const { return p_isInt; }

    virtual void setHardRange(const double& min, const double& max);
    virtual void setHardRange(const int& min, const int& max);
    virtual void setHardRange(string range);
    virtual void setSoftRange(const double& min, const double& max);
    virtual void setSoftRange(const int& min, const int& max);
    virtual void setSoftRange(string range);
    virtual void setEmptyOk(bool emptyOk) { p_emptyOk = emptyOk; }

    // Called when the value in the window must be validated.
    // This function can pop up an error message.
    virtual bool Validate(wxWindow *parent);

    void dump();

  protected:
    // Filter keystrokes
    void onChar(wxKeyEvent& event);
    bool isValidKey(int key) const;

    bool CheckValidator() const;

    void parseRange(string range, bool isHard);

    void check(const wxString& val, bool isHard,
               bool noMin, bool noMax,
               bool minOpen, bool maxOpen,
               double min, double max) throw (InvalidException);

    void initHardRange();
    void initSoftRange();

    void postMessage(const string& msg);

  private:
    bool        p_isInt;
    bool        p_emptyOk;

    bool        p_useHardRange;
    bool        p_useHardMin;
    bool        p_useHardMax;
    double      p_hardMin;
    double      p_hardMax;
    bool        p_hardMinOpen;
    bool        p_hardMaxOpen;

    bool        p_useSoftRange;
    bool        p_useSoftMin;
    bool        p_useSoftMax;
    double      p_softMin;
    double      p_softMax;
    bool        p_softMinOpen;
    bool        p_softMaxOpen;

// Cannot use
//  DECLARE_NO_COPY_CLASS(wxTextValidator)
// because copy constructor is explicitly declared above;
// but no copy assignment operator is defined, so declare
// it private to prevent the compiler from defining it:
    ewxNumericValidator& operator=(const ewxNumericValidator&);
};

#endif // EWXNUMERICVALIDATOR_H
