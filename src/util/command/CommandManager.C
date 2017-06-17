/**
 * @file
 *
 *
 */
#include <iostream>
   using namespace std;

#include "util/Command.H"
#include "util/CommandManager.H"

/**
 * Constructor.
 * @param name name of this command - ususally the same as the class name.
 */
CommandManager::CommandManager(const string& name, int maxStackSize)
{
  p_name =  name;
  p_maxStackSize = maxStackSize;

  p_undoRedoIt = p_stack.size();
}


/**
 * Destructor.
 */
CommandManager::~CommandManager()
{
   clearCommands();
}



/**
 * Equality check based on command name only.
 */
bool CommandManager::operator==(const CommandManager& rhs)
{
  return (p_name == rhs.p_name);
}



string CommandManager::getName() const
{ 
  return p_name;
}



void CommandManager::setName(const string& name)
{ 
  p_name = name;
}



void  CommandManager::append(Command *command)
{
   if (p_undoRedoIt < p_stack.size()) {
      int nItems = p_stack.size();
      for (int idx=p_undoRedoIt; idx<nItems; idx++) {
         delete p_stack.back();
         p_stack.pop_back();
      }
   }
   
   // Make room on the stack if we're out
   if (p_stack.size() > p_maxStackSize) {
      Command *cmd = p_stack.front();
      delete cmd;
      p_stack.pop_front();

   }
   p_stack.push_back(command);

   // By definition, the stack pointer is at the end.
   p_undoRedoIt = p_stack.size();
}


int   CommandManager::getNumCommands() const
{
   return p_stack.size();
}



void  CommandManager::clearCommands()
{
   while (p_stack.size() > 0) {
      delete p_stack.front();
      p_stack.pop_front();
   }
   p_undoRedoIt = p_stack.size();
  
}



void  CommandManager::setMaxStackSize(size_t size)
{
   if (size < p_stack.size()) {
      resizeCommandStack(size);
   }
   p_maxStackSize = size;
}



int   CommandManager::getMaxStackSize() const
{
   return p_maxStackSize;
}


void  CommandManager::resizeCommandStack(size_t size)
{
   while (p_stack.size() > size) {
      delete p_stack.front();
      p_stack.pop_front();
      if (p_undoRedoIt > 0) {
         p_undoRedoIt--;
      }
   }
}


Command *CommandManager::peek() const
{
   Command *ret = 0;
   if (!p_stack.empty()) {
      ret = p_stack.back();
   }
   return ret;
}


string CommandManager::getUndoLabel() const
{
   string ret;
   if (isUndoable()) {
      Command *cmd = p_stack[p_undoRedoIt-1];
      //cout << "undo cmd " << ret << endl;
      ret = cmd->getUndoLabel();
   }
   return ret;
}



bool CommandManager::undo()
{
   bool ret = false;
   if (isUndoable()) {
      ret = true;
      p_undoRedoIt--;
      Command *cmd = p_stack[p_undoRedoIt];
      cmd->undo();

   }
   return ret;
}


string CommandManager::getRedoLabel() const
{
   string ret;
   if (isRedoable()) {
      Command *cmd = p_stack[p_undoRedoIt];
      //cout << "undo cmd " << ret << endl;
      ret = cmd->getUndoLabel();
   }
   return ret;
}


bool CommandManager::redo()
{
   bool ret = false;
   if (isRedoable()) {
      ret = true;
      Command *cmd = p_stack[p_undoRedoIt];
      cmd->execute();
      p_undoRedoIt++;

   }
   return ret;
}

bool CommandManager::isUndoable() const
{
   return (p_stack.size() > 0 && p_undoRedoIt > 0 );
}


bool CommandManager::isRedoable() const
{
   //cout << "is redoable " << p_undoRedoIt << " " << p_stack.size() << endl;
   return p_stack.size() > 0 && p_undoRedoIt != p_stack.size();
}
