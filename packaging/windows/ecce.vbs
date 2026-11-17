' Starts ECCE through ecce.cmd without a console window (Start-menu shortcut).
' File names given (Open with ECCE, #1069) go on to ecce.cmd, which hands them to the Organizer.
Set fso = CreateObject("Scripting.FileSystemObject")
root = fso.GetParentFolderName(WScript.ScriptFullName)
q = Chr(34)
args = ""
For Each a In WScript.Arguments
  args = args & " " & q & a & q
Next
CreateObject("WScript.Shell").Run "cmd.exe /c " & q & q & root & "\ecce.cmd" & q & args & q, 0, False
