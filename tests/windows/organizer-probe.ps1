# What a user meets first on a Windows desktop, checked without clicking:
# the local machine is localhost even with HOST the computer's name, the home
# is open and selected at start, the focus handler settles on a selection
# (it looped forever on wxMSW when nothing was selected), a project can be
# made in Local data, the tools open from the Organizer over the session's
# broker (and an open Builder is reused), and the GUI programs carry the
# application icon.
# Exit 1 on any failure.
#   organizer-probe.ps1 -Ecce <install root with bin\organizer.exe> -Out <dir>
#     [-Exe <a build's organizer.exe>] [-PathPrefix <its DLL dirs>]
param([string]$Ecce, [string]$Out, [string]$Exe = "", [string]$PathPrefix = "")
$ErrorActionPreference = "Continue"
$Ecce = (Resolve-Path $Ecce).Path
New-Item -ItemType Directory -Force "$Out\home", "$Out\local" | Out-Null
$Out = (Resolve-Path $Out).Path
$summary = "$Out\summary.txt"
$fail = 0
function Say($t) { Write-Host $t; Add-Content $summary $t }
function Check($ok, $what) { Say ("{0}: {1}" -f $(if ($ok) { "ok  " } else { "FAIL" }), $what); if (-not $ok) { $script:fail = 1 } }

# The icon: an RT_GROUP_ICON (14) resource in each GUI program.
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class Res {
  [DllImport("kernel32", SetLastError=true, CharSet=CharSet.Unicode)] static extern IntPtr LoadLibraryExW(string f, IntPtr h, uint fl);
  [DllImport("kernel32")] static extern bool FreeLibrary(IntPtr h);
  delegate bool EnumProc(IntPtr h, IntPtr t, IntPtr n, IntPtr p);
  [DllImport("kernel32")] static extern bool EnumResourceNamesW(IntPtr h, IntPtr t, EnumProc f, IntPtr p);
  public static int Icons(string file) {
    IntPtr h = LoadLibraryExW(file, IntPtr.Zero, 0x22);   // as image resource + datafile
    if (h == IntPtr.Zero) return -1;
    int n = 0;
    EnumResourceNamesW(h, (IntPtr)14, (a, b, c, d) => { n++; return true; }, IntPtr.Zero);
    FreeLibrary(h);
    return n;
  }
}
"@
foreach ($a in "organizer", "builder", "calced", "basistool", "machregister", "launcher") {
  $f = "$Ecce\bin\$a.exe"
  if (Test-Path $f) { Check ([Res]::Icons($f) -gt 0) "$a.exe has an icon resource" }
}

# The environment ecce.cmd gives, with HOST the real computer name as there.
$sys = "$env:SystemRoot\System32;$env:SystemRoot"
if ($Exe -eq "") { $Exe = "$Ecce\bin\organizer.exe" }
$env:Path = "$PathPrefix$Ecce\bin;$Ecce\usr\bin;$Ecce\python;$Ecce\strawberry\perl\site\bin;$Ecce\strawberry\perl\bin;$Ecce\strawberry\c\bin;$sys"
$env:ECCE_HOME = $Ecce -replace "\\", "/"
$env:ECCE_REALUSER = $env:USERNAME
$env:HOST = $env:COMPUTERNAME
$env:ECCE_REALUSERHOME = "$Out\home" -replace "\\", "/"
$env:ECCE_LOCAL_DATA = "$Out\local" -replace "\\", "/"
Remove-Item Env:ECCE_NO_MESSAGING -ErrorAction SilentlyContinue
$env:ECCE_NO_DATASERVER = "1"
$env:ECCE_SESSION_LIVENESS = "lease"
$env:ECCE_SESSION_ID = -join ((1..16) | ForEach-Object { "{0:x}" -f (Get-Random -Maximum 16) })
$cmds = "$Out\commands.txt"
Set-Content $cmds -Value $null
$env:ECCE_TEST_ORGANIZER = $cmds
$calcedCmds = "$Out\calced-commands.txt"
Set-Content $calcedCmds -Value $null
$env:ECCE_TEST_CALCED = $calcedCmds
Say "HOST=$env:HOST"
# The session's broker, as ecce.cmd starts it (ecce-gateway-start -> ecce-broker-win).
$b = & "$Ecce\usr\bin\bash.exe" "$Ecce/bin/ecce-gateway-start" 2>&1
Say "broker: $b"
$bf = Get-ChildItem "$Out\home\.ECCE" -Filter "broker_*_$env:ECCE_SESSION_ID" -ErrorAction SilentlyContinue
Check ($null -ne $bf -and ((Get-Content $bf.FullName) -match "^host=127\.0\.0\.1$")) "the session broker is up, on loopback"

$err = "$Out\organizer.err.txt"
$p = Start-Process $Exe -PassThru -RedirectStandardError $err -RedirectStandardOutput "$Out\organizer.out.txt"
function Ask($cmd, $wait = 60) {
  Add-Content $cmds $cmd
  for ($i = 0; $i -lt $wait; $i++) {
    Start-Sleep 1
    $l = Select-String -Path $err -Pattern "^ECCE_TEST_ORGANIZER: $([regex]::Escape($cmd)): (.*)$" -ErrorAction SilentlyContinue | Select-Object -Last 1
    if ($l) { return $l.Matches[0].Groups[1].Value }
    if ($p.HasExited) { return "exited rc=$($p.ExitCode)" }
  }
  return "no answer in ${wait}s"
}
$probe = Ask "probe"
Say "probe: $probe"
Check ($probe -match "register=(added|known)") "local machine registered"
Check ($probe -match "homenode=yes selected-before=yes") "the home is open and selected at start"
Check ($probe -match "selection=\S" -and $probe -notmatch "selection=none") "the tree settles on a selection"
$m = Get-Content "$Out\home\.ECCE\MyMachines" -ErrorAction SilentlyContinue
Check (-not ($m | Select-String -SimpleMatch "$env:COMPUTERNAME")) "no machine registered under $env:COMPUTERNAME (it is localhost)"
$np = Ask "newproject probeproj"
Say "newproject: $np"
Check ($np -like "ok *") "a project made in Local data"
$walk = Ask "walk"
Say "walk: $walk"
Check ($walk -like "ok*") "the tree walks from the home"

# Tools started from the Organizer (its tool buttons), and CalcEd's Builder.
function Windows($name) {
  @(Get-Process $name -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 })
}
function WaitWindow($name, $count = 1, $wait = 60) {
  for ($i = 0; $i -lt $wait; $i++) { if ((Windows $name).Count -ge $count) { return $true }; Start-Sleep 1 }
  return $false
}
$nc = Ask "newcalc probeproj water MOPAC"
Say "newcalc: $nc"
$calc = if ($nc -match "^ok (\S+)") { $Matches[1] } else { "" }
Check ($calc -ne "") "a MOPAC calculation made in the project"
Say ("start CalcEd: " + (Ask "start CalculationEditor $calc"))
Check (WaitWindow "calced") "the Calculation Editor opens from the Organizer"
Add-Content $calcedCmds "builder"
Check (WaitWindow "builder") "the Builder opens from the Calculation Editor"
Say ("start Builder: " + (Ask "start Builder $calc"))
Start-Sleep 10
Check (@(Get-Process builder -ErrorAction SilentlyContinue).Count -eq 1) "the open Builder is reused, not started again"
foreach ($t in @(@("Basistool", "basistool"), @("Launcher", "launcher"), @("MachineRegister", "machregister"))) {
  Say ("start $($t[0]): " + (Ask "start $($t[0]) $calc"))
  Check (WaitWindow $t[1]) "$($t[0]) opens from the Organizer"
}
Say ("start CalculationViewer: " + (Ask "start CalculationViewer $calc"))
Check (WaitWindow "builder" 2) "the Viewer opens from the Organizer"

$p.Refresh()
Check ((-not $p.HasExited) -and $p.Responding) "organizer alive and responding"
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
foreach ($n in "calced", "builder", "basistool", "launcher", "machregister") { Get-Process $n -ErrorAction SilentlyContinue | Stop-Process -Force }
& "$Ecce\usr\bin\bash.exe" "$Ecce/bin/ecce-broker-win" stop 2>&1 | Out-Null
Check (-not (Get-ChildItem "$Out\home\.ECCE" -Filter "broker_*_$env:ECCE_SESSION_ID" -ErrorAction SilentlyContinue)) "the broker file is gone after stop"
Get-Content $err -Tail 15 -ErrorAction SilentlyContinue | ForEach-Object { Say "  | $_" }
exit $fail
