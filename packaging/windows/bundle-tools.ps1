# Adds Strawberry Perl (portable, trimmed) and an embeddable Python with
# wxPython to an ECCE install tree, so gensub, the importers and the
# code-registration dialogs run without anything installed on the machine.
#
#   bundle-tools.ps1 -Install <install-dir> [-Cache <download-dir>]
#
# Result: <install>\strawberry\{perl,c\bin} and <install>\python (python.exe
# and python3.exe: the scripts call python3).  The pins below are the source
# of truth; tests/windows/README.md lists them.
param([Parameter(Mandatory)][string]$Install, [string]$Cache = "$env:TEMP\ecce-dl")
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
$pins = @{
  strawberry = @{ url = "https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_54051_64bit/strawberry-perl-5.40.5.1-64bit-portable.zip";
                  sha256 = "6619fe7eeef921ccddb4aac3972fb602a0c690a3074205b863ade998d7bc79a6" }
  python     = @{ url = "https://www.python.org/ftp/python/3.13.5/python-3.13.5-embed-amd64.zip";
                  sha256 = "7d2650fd9d1b9d002d4a315d5f354247fd6a44f30517c7ef577b08f57a0fb6d9" }
  # pythonhosted.org file path of wxpython-4.3.1-cp313-cp313-win_amd64.whl
  wxpython   = @{ url = "";
                  sha256 = "0ae85e266dbb99fe46bc26920aa87b22fa3f83dc51b321bcd9afa51fa68340e7" }
}

function Fetch($name, $url, $sha, $file) {
  New-Item -ItemType Directory -Force $Cache | Out-Null
  $f = Join-Path $Cache $file
  if (-not (Test-Path $f)) { Invoke-WebRequest $url -OutFile $f }
  $h = (Get-FileHash $f -Algorithm SHA256).Hash.ToLower()
  if ($h -ne $sha) { Remove-Item $f; throw "$name`: sha256 $h, expected $sha" }
  return $f
}
function Unzip($zip, $to) {
  if (Test-Path $to) { Remove-Item -Recurse -Force $to }
  New-Item -ItemType Directory -Force $to | Out-Null
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  [System.IO.Compression.ZipFile]::ExtractToDirectory($zip, $to)
}
function Size($d) { "{0:N0} MB" -f ((Get-ChildItem $d -Recurse -File | Measure-Object Length -Sum).Sum / 1MB) }

$Install = (Resolve-Path $Install).Path

# --- Strawberry Perl: perl itself and the DLLs its modules load; no compiler.
$zip = Fetch "strawberry" $pins.strawberry.url $pins.strawberry.sha256 "strawberry.zip"
$tmp = Join-Path $Cache "strawberry-unpacked"
Unzip $zip $tmp
"strawberry unpacked: $(Size $tmp)"
$sb = Join-Path $Install "strawberry"
if (Test-Path $sb) { Remove-Item -Recurse -Force $sb }
New-Item -ItemType Directory -Force "$sb\c\bin" | Out-Null
Move-Item "$tmp\perl" "$sb\perl"
Get-ChildItem "$tmp\c\bin" -Filter *.dll | Move-Item -Destination "$sb\c\bin"
foreach ($d in "perl\html", "perl\man", "perl\lib\pods", "perl\site\lib\auto\share\dist\Alien-Build") {
  if (Test-Path "$sb\$d") { Remove-Item -Recurse -Force "$sb\$d" }
}
Get-ChildItem "$sb\perl" -Recurse -Include *.pod, *.a, *.h -File | Remove-Item -Force
Remove-Item -Recurse -Force $tmp
"strawberry bundled: $(Size $sb)"

# --- Python: python.org's embeddable zip, plus the wxPython wheel unpacked
# into its site-packages (the embeddable build has no pip).
$py = Join-Path $Install "python"
Unzip (Fetch "python" $pins.python.url $pins.python.sha256 "python-embed.zip") $py
$whl = Join-Path $Cache "wxpython-4.3.1-cp313-cp313-win_amd64.whl"
if (-not (Test-Path $whl)) {
  # The wheel's pythonhosted URL has a content hash in it; ask PyPI.
  $meta = Invoke-RestMethod "https://pypi.org/pypi/wxPython/4.3.1/json"
  $u = ($meta.urls | Where-Object { $_.filename -eq "wxpython-4.3.1-cp313-cp313-win_amd64.whl" }).url
  $pins.wxpython.url = $u
}
$whl = Fetch "wxpython" $pins.wxpython.url $pins.wxpython.sha256 "wxpython-4.3.1-cp313-cp313-win_amd64.whl"
$site = "$py\Lib\site-packages"
New-Item -ItemType Directory -Force $site | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($whl, $site)
Get-ChildItem $site -Recurse -Include *.pyi -File | Remove-Item -Force
# site-packages on sys.path, and `import site` on (the embeddable default is off).
$pth = Get-ChildItem $py -Filter "python*._pth" | Select-Object -First 1
# ..\scripts\codereg: the Theory/Runtype dialogs import their shared
# modules (templates.py) from there, and a ._pth file replaces the usual
# "script's own directory first" (and PYTHONPATH).
Set-Content -Encoding ascii $pth.FullName @("python313.zip", ".", "Lib\site-packages", "..\scripts\codereg", "import site")
Copy-Item "$py\python.exe" "$py\python3.exe"
Copy-Item "$py\pythonw.exe" "$py\python3w.exe"
"python bundled: $(Size $py)"
