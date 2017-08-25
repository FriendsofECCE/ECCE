/**
 * @file 
 *
 *
 */
#ifndef  COMMANDMANAGER_HH
#define  COMMANDMANAGER_HH

#include <string>
#include <deque>

using std::string;
using std::deque;

#include "util/Command.H"

class Command;

/**
 *  CommandManager provides a container to manage a set of commands.
 *  It can be found in various <b>Command Pattern</b> literature though
 *  the exact implementation is needs-specific.
 *
 *  A key use of the CommandManager is to support undo/redo.
 *
 *  You have a stack of objects (could be implemented as an array). 
 *  When you want to execute a command, you construct a command object 
 *  of the appropriate type, call its "execute" method.   If "execute" 
 *  succeeds, you push it to the command stack.
 *
 *  When you want to undo a command, you call the undo method of the 
 *  command at the stack pointer, and decrement the stack pointer.
 *  When you want to redo a command, you increment the stack pointer 
 *  and call the "do" method of the object at the stack pointer.
 *  Note that the act of pushing a new command to the command stack 
 *  truncates the stack at that point, discarding all of the command 
 *  objects after the current top-of-stack stack entry.
 *  Redoing and undoing just move the stack pointer up and down the stack.
 *
 *  There are other design concepts that could be considered.  For example,
 *  the CommandManager could be used to queue up commands for execution.
 *  This idea is not addressed at this time and maybe a different class.
 *   
 *  CommandManagers are often defined as singletons.  However I opted to
 *  leave this design choice to the client programs in case there is use
 *  for different CommandManager instances for different types of commands
 *  (ie different Receivers).
 *
 *  Example usage:
 *    CommandManager mgr = new CommandManager("mine", 1000);
 *    Receiver r = new Receiver();
 *    Command *c = new Command("x",r);
 *    mgr->add();
 *    msgr->execute();
 *
 *  You can add multiple commands.  Execute will then execute any
 *  that have not yet been executed.
 *   
 *  @todo do we need an undo listener
 *  @todo address record/replay
 *  @todo command queue?
 *  @todo explicitly support the notion of a command that can't be undone?
 */
class CommandManager 
{ 
   public: 

      CommandManager(const string& name, int maxStackSize=65000);
      virtual ~CommandManager();

      bool  operator==(const CommandManager& rhs);

      string getName() const;

      void  append(Command *command);

      int   getNumCommands() const;
      void  clearCommands();
      void  resizeCommandStack(size_t size);

      void  setMaxStackSize(size_t size);
      int   getMaxStackSize() const;

      virtual Command *peek() const;

      virtual string getUndoLabel() const;

      virtual string getRedoLabel() const;

      virtual bool undo();

      virtual bool redo();

      virtual bool isRedoable() const;

      virtual bool isUndoable() const;

   protected:

      void setName(const string& name);

   private:

      /** The name of this object. */
      string p_name;  

      /** 
       * The current command stack.  The most recently executed command
       * is at the end.  When max limit is reached, commands are pulled
       * from the front.
       */
      deque<Command*> p_stack;

      /** Maximum size of the stack. */
      size_t p_maxStackSize;

      /** Stack location for undo/redo action. */
      size_t p_undoRedoIt;

};

#endif 
