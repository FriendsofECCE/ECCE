# Adds an "ECCE" entry to the current user's Start menu that runs ecce.cmd
# of the package this script sits in (the package root, next to ecce.cmd).
#   powershell -ExecutionPolicy Bypass -File make-shortcuts.ps1
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$dir = Join-Path ([Environment]::GetFolderPath("Programs")) "ECCE"
New-Item -ItemType Directory -Force $dir | Out-Null
$sh = New-Object -ComObject WScript.Shell
$l = $sh.CreateShortcut("$dir\ECCE.lnk")
$l.TargetPath = "$env:SystemRoot\System32\cmd.exe"
$l.Arguments = "/c `"`"$root\ecce.cmd`"`""
$l.WorkingDirectory = $env:USERPROFILE
$l.WindowStyle = 7
$l.IconLocation = "$root\bin\organizer.exe,0"
$l.Save()
"Start menu: $dir\ECCE.lnk"
