# Installs the MSI per user, runs the local-job and start tests from the
# installed location, uninstalls, checks the files are gone and user data stays.
#   msi-test.ps1 -Msi dist\x.msi -Launchjob build\launchjob.exe -Out msi-run
param([string]$Msi, [string]$Launchjob, [string]$Out)
$ErrorActionPreference = "Continue"
$Msi = (Resolve-Path $Msi).Path
New-Item -ItemType Directory -Force $Out | Out-Null
$Out = (Resolve-Path $Out).Path
$summary = "$Out\summary.txt"
$fail = 0
function Say($t) { Write-Host $t; Add-Content $summary $t }
function Check($ok, $what) { Say ("{0}: {1}" -f $(if ($ok) { "ok  " } else { "FAIL" }), $what); if (-not $ok) { $script:fail = 1 } }

$inst = "$env:LOCALAPPDATA\Programs\ECCE"
$lnk = "$([Environment]::GetFolderPath('Programs'))\ECCE\ECCE.lnk"
# User data that uninstall must not touch.
New-Item -ItemType Directory -Force "$env:USERPROFILE\ecce-local", "$env:USERPROFILE\.ECCE" | Out-Null
"keep" | Set-Content "$env:USERPROFILE\ecce-local\keep.txt"
"keep" | Set-Content "$env:USERPROFILE\.ECCE\keep.txt"

$p = Start-Process msiexec -ArgumentList "/i `"$Msi`" /qn /l*v `"$Out\install.log`"" -Wait -PassThru
Check ($p.ExitCode -eq 0) "msiexec /i exit $($p.ExitCode)"
Check (Test-Path "$inst\bin\organizer.exe") "organizer.exe under $inst"
Check (Test-Path $lnk) "Start-menu shortcut"
foreach ($h in "HKCU", "HKLM") { foreach ($k in "Software\Microsoft\Windows\CurrentVersion\Uninstall", "Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall") {
  Get-ItemProperty "${h}:\$k\*" -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -like "ECCE*" } | ForEach-Object { Say "uninstall entry: ${h}:\$k\$($_.PSChildName) $($_.DisplayName)" } } }
# Elevated (as on the runner) Windows Installer files a per-user package's entry under HKLM; unelevated it is HKCU.
$arp = (Get-ItemProperty "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*", "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*" -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -eq "ECCE" })
Check ($null -ne $arp) "Apps & Features entry present"
Say ("installed: {0:N0} MB, {1} files" -f ((Get-ChildItem $inst -Recurse -File | Measure-Object Length -Sum).Sum / 1MB), (Get-ChildItem $inst -Recurse -File).Count)
$t = New-Object -ComObject WScript.Shell
Say ("shortcut: " + $t.CreateShortcut($lnk).TargetPath + " " + $t.CreateShortcut($lnk).Arguments)

# The tests expect <stage>\ecce\bin: a junction gives them that without admin.
$stage = "$env:RUNNER_TEMP\msistage"
Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
New-Item -ItemType Directory $stage | Out-Null
cmd /c mklink /J "$stage\ecce" "$inst" | Out-Null
Copy-Item $Launchjob "$inst\bin\launchjob.exe"

$r = "$stage\ecce"
$sys = "$env:SystemRoot\System32;$env:SystemRoot"
$env:Path = $sys
$env:WINTEST_HOME = $r
$env:WINTEST_BASE_PATH = "$r\bin;$r\usr\bin;$r\python;$r\strawberry\perl\site\bin;$r\strawberry\perl\bin;$r\strawberry\c\bin;$sys"
foreach ($c in "complete", "cancel") {
  & "$r\python\python3.exe" tests/windows/launch_local.py $c --build "$r\bin" --state "$Out\state-$c" 2>&1 | Tee-Object "$Out\$c.log"
  Check ($LASTEXITCODE -eq 0) "local job $c from the installed copy"
}
$env:Path = $sys
tests\windows\ci-run.ps1 -Stage $stage -Out "$Out\apps"
Get-Content "$Out\apps\summary.txt" | ForEach-Object { Say "apps: $_" }
Check (-not (Select-String -Path "$Out\apps\summary.txt" -Pattern "exited rc=-?[1-9]|missing|FAILED" -Quiet)) "start test summary has no app exited, missing or failed"

& tests\windows\organizer-probe.ps1 -Ecce $inst -Out "$Out\probe"
Check ($LASTEXITCODE -eq 0) "organizer probe from the installed copy"
$env:Path = $sys

# The shortcut's own path: wscript + ecce.vbs + ecce.cmd, no console window.
Start-Process "$env:SystemRoot\System32\wscript.exe" -ArgumentList "//B //Nologo `"$inst\ecce.vbs`""
Start-Sleep 20
$o = Get-Process organizer -ErrorAction SilentlyContinue
Check ($null -ne $o) "organizer running after the shortcut's command"
if ($o) { Check ($o[0].MainWindowHandle -ne 0) "organizer has a window"; $o | Stop-Process -Force }
Check (-not (Get-Process cmd -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 })) "no console window left"
# With the last window gone, ecce.cmd's watcher stops the session broker.
# (An editor the probe above opened may still be up: close every window.)
Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$inst\bin\*" -and $_.MainWindowHandle -ne 0 } | Stop-Process -Force
$left = $null
for ($i = 0; $i -lt 30; $i++) {
  Start-Sleep 1
  $left = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$inst\*" })
  if ($left.Count -eq 0) { break }
}
Check ($left.Count -eq 0) "the session broker stopped with the last window ($($left.ProcessName -join ' '))"
$left | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep 3

# Uninstall.  The test driver is not ours; other files the run wrote are removed by the MSI.
Remove-Item "$inst\bin\launchjob.exe" -Force -ErrorAction SilentlyContinue
cmd /c rmdir "$stage\ecce"
$p = Start-Process msiexec -ArgumentList "/x `"$Msi`" /qn /l*v `"$Out\uninstall.log`"" -Wait -PassThru
Check ($p.ExitCode -eq 0) "msiexec /x exit $($p.ExitCode)"
Check (-not (Test-Path $inst)) "install folder gone"
if (Test-Path $inst) { Get-ChildItem $inst -Recurse | Select-Object -First 20 | ForEach-Object { Say "  left: $($_.FullName)" } }
Check (-not (Test-Path $lnk)) "shortcut gone"
Check (Test-Path "$env:USERPROFILE\ecce-local\keep.txt") "ecce-local kept"
Check (Test-Path "$env:USERPROFILE\.ECCE\keep.txt") "~/.ECCE kept"
exit $fail
