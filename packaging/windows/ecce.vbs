' Starts ECCE through ecce.cmd without a console window (Start-menu shortcut).
Set fso = CreateObject("Scripting.FileSystemObject")
root = fso.GetParentFolderName(WScript.ScriptFullName)
CreateObject("WScript.Shell").Run "cmd.exe /c """ & root & "\ecce.cmd""", 0, False
