#ifndef  UCOMMAND_HH
#define  UCOMMAND_HH

#include <iostream>
using std::ostream;

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "util/UCommandParam.H"

class ICommandObject;

class UCommand
{
public:
    UCommand(const string& name);
    virtual ~UCommand();

    bool  operator==(const UCommand& rhs);

    // Subclasses should call addParam for all parameters int the init
    // method.
    virtual void init(void) = 0;

    // Psuedo run type type info
    virtual string getCommandClassId() const;

    string name() const;

    // addParam, setParam, and getParam have two implementations - one
    // for STl and one for char*.  it's ugly
    bool addParam(const string& paramName, const char *value);
    bool addParam(const char* paramName, const char *value);
    bool addParam(const string& paramName, double value);
    bool addParam(const char* paramName, double value);
    bool addParam(const string& paramName, int value);
    bool addParam(const char* paramName, int value);
    bool addParam(const char* paramName, const vector<int>& value);
    bool addParam(const string& paramName, const vector<int>& value);
    bool addBoolParam(const string& paramName, bool value);
    bool addBoolParam(const char* paramName, bool value);

    bool setParam(const string& paramName, const char* value);
    bool setParam(const char* paramName, const char* value);
    bool setParam(const string& paramName, int value);
    bool setParam(const char* paramName, int value);
    bool setParam(const string& paramName, double value);
    bool setParam(const char* paramName, double value);
    bool setParam(const string& paramName, const vector<int>& value);
    bool setParam(const char* paramName, const vector<int>& value);
    bool setBoolParam(const string& paramName, bool value);
    bool setBoolParam(const char* paramName, bool value);

    void getParam(const string& paramName, int& value);
    void getParam(const char* paramName, int& value);
    void getParam(const string& paramName, double& value);
    void getParam(const char* paramName, double& value);
    void getParam(const string& paramName, string& value);
    void getParam(const char* paramName, vector<int>& value);
    void getParam(const string& paramName, vector<int>& value);
    void getBoolParam(const string& paramName, bool& value);
    void getBoolParam(const char* paramName, bool& value);

    void getParamDefault(const string& paramName, string& value);
    void getParamDefault(const string& paramName, double& value);
    void getParamDefault(const string& paramName, int& value);
    void getParamDefault(const string& paramName, vector<int>& value);
    void getBoolParamDefault(const string& paramName, bool& value);

    void resetToDefaults();

    vector<string> *getParameterNames() const;

    // This is where the work of the command is performed
    virtual bool execute(ICommandObject *) = 0; 

    // Record the command request for later playbak
    virtual bool record(ostream& os, ICommandObject *) = 0; 

    // This is the help info or instructions for the command.
    void        setMessage(string msg); 
    string   getMessage(void) const; 

    // This is an error message from the command or empty string if ok
    void        setErrorMessage(string msg ); 
    string   getErrorMessage(void) const;

protected:

    string p_helpMsg;
    string p_lastError;
    string p_name;
    vector<UCommandParam*> *p_params;

private:

    // Friends
    friend ostream& operator<< (ostream& os, const UCommand& rhs);
};


#endif // UCommand
