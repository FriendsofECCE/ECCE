# Runs one command line on the desktop through a scheduled task (ssh sessions
# have no desktop), waits for it and prints its log.
#   powershell -File desktask.ps1 -Cmd "<command line>" -Log <file> [-Minutes 15]
param([string]$Cmd, [string]$Log, [int]$Minutes = 15)
$done = "$Log.done"
Remove-Item $done -ErrorAction SilentlyContinue
$job = "$Log.ps1"
@"
& $Cmd *> "$Log"
"exit `$LASTEXITCODE" | Add-Content "$Log"
"done" | Set-Content "$done"
"@ | Set-Content $job
schtasks /create /f /tn eccedesk /sc once /st 00:00 /it /tr "powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File $job" 2>&1 | Out-Null
schtasks /run /tn eccedesk | Out-Null
for ($i = 0; $i -lt $Minutes * 12; $i++) { if (Test-Path $done) { break }; Start-Sleep 5 }
schtasks /delete /f /tn eccedesk | Out-Null
Get-Content $Log
