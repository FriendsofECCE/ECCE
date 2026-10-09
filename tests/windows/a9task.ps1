# Runs a9_look.py on the desktop through a scheduled task (ssh sessions have
# no desktop) and prints its log.
#   powershell -File a9task.ps1 <tree> <state> <png dir> <log> [qm]
param([string]$Tree, [string]$State, [string]$Pngs, [string]$Log, [string]$Extra = "")
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$done = "$Log.done"
Remove-Item $done -ErrorAction SilentlyContinue
$job = Join-Path $here "a9job.ps1"
@"
& "$Tree\python\python3.exe" "$here\a9_look.py" "$Tree" "$State" "$Pngs" $Extra *> "$Log"
"exit `$LASTEXITCODE" | Add-Content "$Log"
"done" | Set-Content "$done"
"@ | Set-Content $job
schtasks /create /f /tn a9look /sc once /st 00:00 /it /tr "powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File $job" | Out-Null
schtasks /run /tn a9look | Out-Null
for ($i = 0; $i -lt 160; $i++) { if (Test-Path $done) { break }; Start-Sleep 5 }
schtasks /delete /f /tn a9look | Out-Null
Get-Content $Log
