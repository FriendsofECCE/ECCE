param([string]$name, [string]$exe, [string]$arg = "", [int]$wait = 25)
$out = "C:\Users\andy\shots"
New-Item -ItemType Directory -Force $out | Out-Null
# Strawberry Perl (portable) is what the Windows client bundles; MSYS2's perl is the fallback.
$sp = "C:\Users\andy\strawberry"
if (Test-Path "$sp\perl\bin\perl.exe") { $perlDirs = "$sp\perl\site\bin;$sp\perl\bin;$sp\c\bin;" } else { $perlDirs = "" }
$env:Path = $perlDirs + "C:\msys64\ucrt64\bin;" + $env:Path + ";C:\msys64\usr\bin"
$env:ECCE_HOME = "C:\Users\andy\ECCE"
$env:ECCE_DATA = "C:\Users\andy\ECCE\data"
$env:ECCE_REALUSERHOME = "C:\Users\andy\eccehome"
New-Item -ItemType Directory -Force $env:ECCE_REALUSERHOME | Out-Null
$env:ECCE_REALUSER = "andy"
$env:ECCE_REALUSERHOME = "C:/Users/andy/eccehome"
$env:ECCE_LOCAL_DATA = "C:/Users/andy/ecce-local"
New-Item -ItemType Directory -Force "C:/Users/andy/ecce-local" | Out-Null
$env:ECCE_NO_MESSAGING = "1"
$env:ECCE_NO_DATASERVER = "1"
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
function Shot($f) {
  $b = [System.Windows.Forms.SystemInformation]::VirtualScreen
  $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($b.Left, $b.Top, 0, 0, $bmp.Size)
  $bmp.Save($f, [System.Drawing.Imaging.ImageFormat]::Png)
}
Shot "$out\$name-0.png"
$sp = @{FilePath=$exe; PassThru=$true; WorkingDirectory="C:\Users\andy"; RedirectStandardOutput="$out\$name.out.txt"; RedirectStandardError="$out\$name.err.txt"}
if ($arg -ne "") { $sp.ArgumentList = $arg }
$p = Start-Process @sp
Start-Sleep -Seconds $wait
$alive = -not $p.HasExited
Shot "$out\$name-1.png"
$w = Get-Process | Where-Object { $_.MainWindowTitle -ne "" } | Select-Object ProcessName, MainWindowTitle | Out-String
$p.WaitForExit(100); "alive=$alive exit=$($p.ExitCode)`r`n$w" | Out-File "$out\$name.status.txt"
if ($alive) { Stop-Process -Id $p.Id -Force }
