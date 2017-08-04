# Simple script that demonstrates creating a CommandManager, populating
# with a bunch of commands, then using undo/redo
import ecce

mgr = ecce.CommandManager("xxx");
sr = ecce.StringReceiver();

DaysOfWeek=["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]

## Create a bunch of commands
for X in range(len(DaysOfWeek)):
     cmd = ecce.StringAppendEC("Append",sr)
     # We don't want python to delete this object.  Will cause crash
     cmd.thisown = 0
     p = cmd.getParameter("append")
     p.setString(DaysOfWeek[X])
     cmd.execute();
     mgr.addCommand(cmd)


print mgr.getNumCommands()
print sr.debugPrint()

while (mgr.isUndoable()):
    mgr.undo()
    print sr.debugPrint()

while (mgr.isRedoable()):
    mgr.redo()
    print sr.debugPrint()

