<#
  ecce-report.ps1 - what a Windows machine does to ECCE, for a bug report.

  Needs no admin rights. Measures how long ECCE's helper programs take to
  start (bash, perl) against a Windows program, how long name lookups take,
  reads the display scale and the installed security products, and packs
  that with ECCE's log files into one zip. Passwords and other files in
  .ECCE are not collected: only *.log files and a file listing.

  Usage (normal PowerShell):
    powershell -ExecutionPolicy Bypass -File ecce-report.ps1
    powershell -ExecutionPolicy Bypass -File ecce-report.ps1 -SendTo user@host:dir -Key path\to\key
#>
param(
    [string]$EcceRoot = "$env:LOCALAPPDATA\Programs\ECCE",
    [string]$SendTo = "",
    [string]$Key = "$env:USERPROFILE\.ssh\ecce_report"
)

$ErrorActionPreference = "Continue"
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$name = "ecce-report-$env:COMPUTERNAME-$stamp"
$work = Join-Path $env:TEMP $name
New-Item -ItemType Directory -Force -Path $work | Out-Null
$report = Join-Path $work "report.txt"

# UTF-8: Windows PowerShell 5 writes UTF-16 by default.
function Say($text) { Write-Host $text; $text | Out-File -FilePath $report -Append -Encoding utf8 }

# Seconds for $n starts of a program; a spawn slowed by a security scanner
# shows here long before it shows anywhere in ECCE.
function Time-Starts($label, $n, [scriptblock]$run) {
    try {
        $s = (Measure-Command { 1..$n | ForEach-Object { & $run | Out-Null } }).TotalSeconds
        Say ("{0,-34} {1,7:N2} s for {2} starts" -f $label, $s, $n)
    } catch {
        Say ("{0,-34} failed: {1}" -f $label, $_.Exception.Message)
    }
}

Say "ECCE Windows report $stamp"
Say "Computer: $env:COMPUTERNAME  User: $env:USERNAME  Domain: $env:USERDOMAIN"
$os = Get-CimInstance Win32_OperatingSystem
Say "Windows: $($os.Caption) $($os.Version)"
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
Say "CPU: $($cpu.Name)  RAM: $([math]::Round($os.TotalVisibleMemorySize/1MB,1)) GB"
$model = Get-CimInstance Win32_ComputerSystem
Say "Model: $($model.Manufacturer) $($model.Model)  Part of domain: $($model.PartOfDomain)"
Say "ECCE folder: $EcceRoot  (exists: $(Test-Path $EcceRoot))"
if (Test-Path "$EcceRoot\VERSION") { Say "ECCE version: $(Get-Content "$EcceRoot\VERSION" -TotalCount 1)" }
Say "Home folder: $env:USERPROFILE"
$docs = [Environment]::GetFolderPath("MyDocuments")
Say "Documents folder: $docs"

Say ""
Say "== Display"
try {
    Add-Type -AssemblyName System.Windows.Forms
    foreach ($s in [System.Windows.Forms.Screen]::AllScreens) {
        Say "Screen $($s.DeviceName): $($s.Bounds.Width)x$($s.Bounds.Height) primary=$($s.Primary)"
    }
    $dpi = (Get-ItemProperty "HKCU:\Control Panel\Desktop\WindowMetrics" -Name AppliedDPI -ErrorAction Stop).AppliedDPI
    Say "Scale: $([math]::Round($dpi/96*100)) % (AppliedDPI $dpi)"
} catch { Say "Display: could not read ($($_.Exception.Message))" }

Say ""
Say "== Program start times (normal: well under 1 s for 5 starts)"
$bash = "$EcceRoot\usr\bin\bash.exe"
$perl = "$EcceRoot\strawberry\perl\bin\perl.exe"
Time-Starts "Windows cmd.exe (baseline)" 5 { & "$env:SystemRoot\System32\cmd.exe" /c exit }
Time-Starts "ECCE bash.exe" 5 { & $bash -c true }
Time-Starts "ECCE perl.exe" 5 { & $perl -e 1 }
Time-Starts "ECCE bash.exe running perl" 5 { & $bash -c "perl -e 1" }

Say ""
Say "== Name lookups"
foreach ($h in @("localhost", $env:COMPUTERNAME, "$env:COMPUTERNAME.$env:USERDNSDOMAIN")) {
    if ($h -eq "." -or $h.EndsWith(".")) { continue }
    try {
        $t = Measure-Command { $r = [System.Net.Dns]::GetHostEntry($h) }
        Say ("{0,-40} {1,6:N2} s -> {2}" -f $h, $t.TotalSeconds, (($r.AddressList | ForEach-Object { $_.IPAddressToString }) -join ", "))
    } catch { Say ("{0,-40} failed: {1}" -f $h, $_.Exception.InnerException.Message) }
}

Say ""
Say "== Security products"
try {
    Get-CimInstance -Namespace root/SecurityCenter2 -ClassName AntiVirusProduct -ErrorAction Stop |
        ForEach-Object { Say "Antivirus: $($_.displayName)" }
} catch { Say "Antivirus: could not read" }
try {
    $al = Get-AppLockerPolicy -Effective -ErrorAction Stop
    $rules = @($al.RuleCollections | ForEach-Object { $_ }).Count
    Say "AppLocker: $rules rules in effect"
    $al | Set-Content -Path (Join-Path $work "applocker.txt")
} catch { Say "AppLocker: could not read" }
try {
    $mp = Get-MpComputerStatus -ErrorAction Stop
    Say "Defender real-time protection: $($mp.RealTimeProtectionEnabled)"
} catch { Say "Defender status: could not read" }

Say ""
Say "== ECCE files"
$ecceDir = Join-Path $env:USERPROFILE ".ECCE"
if (Test-Path $ecceDir) {
    Get-ChildItem -Recurse -Force $ecceDir -ErrorAction SilentlyContinue |
        Select-Object FullName, Length, LastWriteTime |
        Format-Table -AutoSize | Out-String -Width 300 |
        Set-Content (Join-Path $work "ecce-dir-listing.txt")
    $logs = Join-Path $work "logs"
    New-Item -ItemType Directory -Force -Path $logs | Out-Null
    Get-ChildItem -Recurse -Force $ecceDir -Filter *.log -ErrorAction SilentlyContinue |
        ForEach-Object { Copy-Item $_.FullName (Join-Path $logs ($_.Directory.Name + "_" + $_.Name)) }
    Say "Logs copied: $((Get-ChildItem $logs).Count)"
} else { Say "No .ECCE folder in $env:USERPROFILE" }
$runs = Join-Path $env:USERPROFILE "ecce-runs"
if (Test-Path $runs) {
    Get-ChildItem -Recurse -Force $runs -ErrorAction SilentlyContinue |
        Select-Object FullName, Length, LastWriteTime |
        Format-Table -AutoSize | Out-String -Width 300 |
        Set-Content (Join-Path $work "ecce-runs-listing.txt")
}

$zip = Join-Path $env:TEMP "$name.zip"
Compress-Archive -Path "$work\*" -DestinationPath $zip -Force
Write-Host ""
Write-Host "Report: $zip"

if ($SendTo -ne "") {
    & "$env:SystemRoot\System32\OpenSSH\scp.exe" -i $Key -o StrictHostKeyChecking=accept-new $zip $SendTo
    if ($LASTEXITCODE -eq 0) { Write-Host "Sent to $SendTo" } else { Write-Host "Sending failed; send $zip by hand." }
}
