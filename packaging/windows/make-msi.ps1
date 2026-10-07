# Builds a per-user MSI (no admin, installs under %LOCALAPPDATA%\Programs\ECCE)
# from a finished install tree, i.e. the one the zip is made from.
#   make-msi.ps1 -Tree stage\ecce -Version 9.0.0-alpha.6 -Out dist\ECCE-windows-9.0.0-alpha.6.msi
# Needs the .NET tool "wix" (v5) and its Util extension; see build.yml.
# WiX is run directly: CPack's WiX generator wants to do the install step
# itself, but this tree is completed afterwards by the bundle scripts.
param([string]$Tree, [string]$Version, [string]$Out)
$ErrorActionPreference = "Stop"
$Tree = (Resolve-Path $Tree).Path

# MSI wants a.b.c.d numbers.  alpha.N -> a.b.c.N, a release -> a.b.c.1000, so
# a release sorts above its alphas.  Windows Installer ignores the 4th field
# when comparing, hence AllowSameVersionUpgrades below.
if ($Version -notmatch '^(\d+)\.(\d+)\.(\d+)(?:-[A-Za-z]+\.?(\d+))?') { throw "version $Version" }
$n = if ($Matches[4]) { $Matches[4] } else { "1000" }
$msiVer = "$($Matches[1]).$($Matches[2]).$($Matches[3]).$n"

$ids = @{}; $nid = 0
$sb = New-Object System.Text.StringBuilder
function Esc($s) { [Security.SecurityElement]::Escape($s) }
function Emit($dir, $indent) {
  foreach ($f in Get-ChildItem -LiteralPath $dir -File) {
    $script:nid++
    [void]$sb.AppendLine("$indent<Component Id=`"c$nid`"><File Id=`"f$nid`" Source=`"$(Esc $f.FullName)`" KeyPath=`"yes`" /></Component>")
  }
  foreach ($d in Get-ChildItem -LiteralPath $dir -Directory) {
    $script:nid++
    [void]$sb.AppendLine("$indent<Directory Id=`"d$nid`" Name=`"$(Esc $d.Name)`">")
    Emit $d.FullName "$indent  "
    [void]$sb.AppendLine("$indent</Directory>")
  }
}
Emit $Tree "      "

$refs = New-Object System.Text.StringBuilder
foreach ($m in [regex]::Matches($sb.ToString(), 'Component Id="(c\d+)"')) {
  [void]$refs.AppendLine("      <ComponentRef Id=`"$($m.Groups[1].Value)`" />")
}

# Fixed: the same product line must keep the same UpgradeCode.
$upgrade = "6b1f5d0e-3c52-4c0a-9a43-2f6f3c1e9d17"
$wxs = @"
<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs" xmlns:util="http://wixtoolset.org/schemas/v4/wxs/util">
  <Package Name="ECCE" Manufacturer="FriendsofECCE" Version="$msiVer" UpgradeCode="$upgrade"
           Scope="perUser" InstallerVersion="500" Language="1033">
    <MajorUpgrade AllowSameVersionUpgrades="yes"
      DowngradeErrorMessage="A newer version of ECCE is already installed." />
    <MediaTemplate EmbedCab="yes" CompressionLevel="high" />
    <Icon Id="ecce.exe" SourceFile="$(Esc "$Tree\bin\organizer.exe")" />
    <Property Id="ARPPRODUCTICON" Value="ecce.exe" />
    <Property Id="ARPCOMMENTS" Value="ECCE $Version (Windows client, experimental)" />
    <Property Id="INSTALLFOLDER_PATH">
      <RegistrySearch Id="InstDir" Root="HKCU" Key="Software\FriendsofECCE\ECCE" Name="InstallDir" Type="directory" />
    </Property>

    <StandardDirectory Id="LocalAppDataFolder">
      <Directory Id="ProgramsDir" Name="Programs">
        <Directory Id="INSTALLFOLDER" Name="ECCE">
$($sb.ToString())
        </Directory>
      </Directory>
    </StandardDirectory>
    <StandardDirectory Id="ProgramMenuFolder">
      <Directory Id="MenuDir" Name="ECCE">
        <Component Id="MenuShortcut" Guid="*">
          <Shortcut Id="EcceShortcut" Name="ECCE" Description="ECCE computational chemistry"
                    Target="[SystemFolder]wscript.exe"
                    Arguments="//B //Nologo &quot;[INSTALLFOLDER]ecce.vbs&quot;"
                    WorkingDirectory="INSTALLFOLDER" Icon="ecce.exe" />
          <RemoveFolder Id="RmMenu" On="uninstall" />
          <RegistryValue Root="HKCU" Key="Software\FriendsofECCE\ECCE" Name="InstallDir"
                         Type="string" Value="[INSTALLFOLDER]" KeyPath="yes" />
          <!-- Files the program writes into its own folder; ecce-local and ~/.ECCE are elsewhere. -->
          <util:RemoveFolderEx On="uninstall" Property="INSTALLFOLDER_PATH" />
        </Component>
      </Directory>
    </StandardDirectory>
    <Feature Id="Main" Title="ECCE" Level="1">
      <ComponentRef Id="MenuShortcut" />
$($refs.ToString())    </Feature>
  </Package>
</Wix>
"@
$work = Join-Path ([IO.Path]::GetTempPath()) "ecce-msi"
New-Item -ItemType Directory -Force $work | Out-Null
$wxsFile = "$work\ecce.wxs"
[IO.File]::WriteAllText($wxsFile, $wxs, (New-Object Text.UTF8Encoding $false))
New-Item -ItemType Directory -Force (Split-Path -Parent $Out) | Out-Null
# Per-user files have no HKCU key path (ICE38/64/91 are about that); validation is skipped.
wix build -arch x64 -ext WixToolset.Util.wixext -sval -o $Out $wxsFile
if ($LASTEXITCODE -ne 0) { throw "wix build failed" }
"{0}: {1:N1} MB, ProductVersion {2}" -f $Out, ((Get-Item $Out).Length / 1MB), $msiVer
