# Does ECCE start on Windows? (#133), for CI: runs each installed GUI app on
# the runner's desktop, screenshots it, records how it ended.  Never fails
# the job; the verdict is in <Out>\summary.txt.
#   ci-run.ps1 -Stage <dir with ecce\bin> -Out <dir> [-Msys D:\a\_temp\msys64] [-Strawberry <dir>]
param([string]$Stage, [string]$Out, [string]$Msys = "C:\msys64", [string]$Strawberry = "")
$ErrorActionPreference = "Continue"
$Stage = (Resolve-Path $Stage).Path
New-Item -ItemType Directory -Force "$Out\shots", "$Out\logs", "$Out\home", "$Out\local" | Out-Null
$Out = (Resolve-Path $Out).Path
$summary = "$Out\summary.txt"
function Say($t) { Write-Host $t; Add-Content $summary $t }

$perl = ""
if ($Strawberry -ne "" -and (Test-Path "$Strawberry\perl\bin\perl.exe")) {
  $perl = "$Strawberry\perl\site\bin;$Strawberry\perl\bin;$Strawberry\c\bin;"
}
# ucrt64 first: Strawberry's c\bin has its own libstdc++/libgcc that crash our exes.
$env:Path = "$Msys\ucrt64\bin;" + $perl + $env:Path + ";$Msys\usr\bin"
$env:ECCE_HOME = ("$Stage\ecce" -replace "\\", "/")
$env:ECCE_REALUSER = $env:USERNAME
$env:ECCE_REALUSERHOME = ("$Out\home" -replace "\\", "/")
$env:ECCE_LOCAL_DATA = ("$Out\local" -replace "\\", "/")
$env:ECCE_NO_MESSAGING = "1"
$env:ECCE_NO_DATASERVER = "1"
$env:ECCE_SESSION_LIVENESS = "lease"

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
function Shot($f) {
  try {
    $b = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($b.Left, $b.Top, 0, 0, $bmp.Size)
    $bmp.Save($f, [System.Drawing.Imaging.ImageFormat]::Png)
  } catch { Say "   screenshot failed: $_" }
}
function RunApp($name, $exe, $arg = "", $wait = 25) {
  Say "== ${name}: $exe $arg"
  if (-not (Test-Path $exe)) { Say "   missing"; return }
  $sp = @{FilePath=$exe; PassThru=$true; WorkingDirectory=$Out;
          RedirectStandardOutput="$Out\logs\$name.out.txt"; RedirectStandardError="$Out\logs\$name.err.txt"}
  if ($arg -ne "") { $sp.ArgumentList = $arg }
  $p = Start-Process @sp
  $found = $false
  for ($i = 0; $i -lt $wait; $i++) {
    Start-Sleep -Seconds 1
    $p.Refresh()
    if ($p.HasExited) { break }
    if ($p.MainWindowHandle -ne 0) { $found = $true; break }
  }
  if (-not $p.HasExited) { Start-Sleep -Seconds 4 }
  Shot "$Out\shots\$name-1.png"
  $w = Get-Process | Where-Object { $_.MainWindowTitle -ne "" } | ForEach-Object { "$($_.ProcessName): $($_.MainWindowTitle)" }
  $w | Out-File "$Out\logs\$name.windows.txt"
  if (-not $p.HasExited) {
    Say "   alive, window=$found"
    Stop-Process -Id $p.Id -Force
  } else {
    Say ("   exited rc={0} (0xC0000005 = access violation), window={1}" -f $p.ExitCode, $found)
  }
  Get-Content "$Out\logs\$name.err.txt" -Tail 6 -ErrorAction SilentlyContinue | ForEach-Object { Say "   | $_" }
}

Say "Windows $([System.Environment]::OSVersion.Version)"
Get-CimInstance Win32_VideoController | ForEach-Object { Say "video: $($_.Name)" }
Say ("screen: " + [System.Windows.Forms.SystemInformation]::VirtualScreen)
Shot "$Out\shots\00-desktop.png"

$bin = "$Stage\ecce\bin"
Get-ChildItem $bin -Filter *.exe | ForEach-Object { $_.Name } | Out-File "$Out\logs\bin.txt"
$glycine = (Resolve-Path "$PSScriptRoot\..\fragreaders\data\glycine.pdb").Path
foreach ($a in "organizer", "builder", "calced", "basistool", "pertable", "machbrowser", "machregister", "launcher") {
  $arg = ""
  if ($a -eq "builder") { $arg = $glycine }
  RunApp $a "$bin\$a.exe" $arg
}

# The Builder renders a molecule through the scene hook, which writes PPMs and exits.
New-Item -ItemType Directory -Force "$Out\shots\scene" | Out-Null
"style Ball And Stick`nviewall`nsnap builder-glycine`n" | Out-File -Encoding ascii "$Out\glycine.scene"
$env:ECCE_VIEWER_SCENE = "$Out\glycine.scene"
$env:ECCE_VIEWER_SCENE_OUT = "$Out\shots\scene"
RunApp "builder-scene" "$bin\builder.exe" $glycine 60
Remove-Item Env:ECCE_VIEWER_SCENE, Env:ECCE_VIEWER_SCENE_OUT
if (Test-Path "$Out\shots\scene\FAILED") { Say "   scene FAILED: $(Get-Content "$Out\shots\scene\FAILED")" }
$ppm = Get-ChildItem "$Out\shots\scene" -Filter *.ppm -ErrorAction SilentlyContinue
if ($ppm) { Say "   scene rendered: $($ppm.Name -join ' ')" }
exit 0
