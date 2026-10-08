# What a user meets first on a Windows desktop, checked without clicking:
# the local machine is registered under the computer's real name, the home
# is open and selected at start, the focus handler settles on a selection
# (it looped forever on wxMSW when nothing was selected), a project can be
# made in Local data, and the GUI programs carry the application icon.
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
$env:ECCE_NO_MESSAGING = "1"
$env:ECCE_NO_DATASERVER = "1"
$env:ECCE_SESSION_LIVENESS = "lease"
$cmds = "$Out\commands.txt"
Set-Content $cmds -Value $null
$env:ECCE_TEST_ORGANIZER = $cmds
Say "HOST=$env:HOST"

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
Check ([bool]($m | Select-String -SimpleMatch "$env:COMPUTERNAME")) "MyMachines lists $env:COMPUTERNAME"
$np = Ask "newproject probeproj"
Say "newproject: $np"
Check ($np -like "ok *") "a project made in Local data"
$walk = Ask "walk"
Say "walk: $walk"
Check ($walk -like "ok*") "the tree walks from the home"
$p.Refresh()
Check ((-not $p.HasExited) -and $p.Responding) "organizer alive and responding"
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
Get-Content $err -Tail 15 -ErrorAction SilentlyContinue | ForEach-Object { Say "  | $_" }
exit $fail
