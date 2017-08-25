/**
 * @file 
 *
 *
 */
#ifndef  COMMAND_HH
#define  COMMAND_HH

#include <iostream>
  using std::ostream;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/CommandParameter.H"
#include "util/EcceException.H"

class CommandParameter;
class Receiver;

/**
 *  Abstract class for <b>Command</b> object of the <b>Command Pattern</b>.
 *  All commands must subclass this class.
 *
 **/
class Command
{
   public:
      Command(const string& name, Receiver * receiver);
      virtual ~Command();

      bool  operator==(const Command& rhs);

      /** 
       * The implementation of the command. 
       * Return true if it did something 
       * execute is called for initial execution and for the redo.
       * redo should typically not create additional undo data.
       */
      virtual bool execute() throw(EcceException)=0;

      virtual bool isUndoable() const;

      /** Undo the command; should do something if isUndoable returns true. */
      virtual void undo() = 0; 

      /** 
       * Class customizable label to use for undo.  name will be
       * used if null is returned.  Expected usage:
       * Undo <label>
       */
      virtual string getUndoLabel() const;

      string getName() const;

      virtual void addParameter(CommandParameter * param);
      virtual CommandParameter * getParameter(const string& name);

      void resetToDefaults();

      vector<string> *getParameterNames() const;

      void     setHelpMessage(const string& msg); 
      string   getHelpMessage() const; 

      void     setErrorMessage(const string& msg ); 
      string   getErrorMessage() const;

      Receiver *getReceiver() const;

      void psuedoConstructor(const string& name, Receiver * receiver);

      void parse(const string& data) throw(EcceException);

   protected:
      /** 
       * By convenction, subclasses should call addParam for 
       * all parameters in the init method.
       */
      virtual void init(void) = 0;

   private:

      /** Help / description for this command. */
      string p_helpMsg;

      /** Error message or empty string. */
      string p_lastError;

      /** The name for this command. */
      string p_name;

      /** The set of all parameters for this command. */
      vector<CommandParameter *> p_params;

      /** The CommandPattern Reciever object. */
      Receiver * p_receiver;

      friend ostream& operator<<(ostream& os, const Command& cmd);
};


#endif
