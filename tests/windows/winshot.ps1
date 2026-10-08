# Saves a window of a process as a PNG with PrintWindow (no input sent):
# its main window, or with -Title the first visible top-level window of that
# process whose title starts with it (a dialog).  Exit 1 if none was found.
# -Repeat N looks for it every 20 ms N times (a dialog that is up briefly)
# and saves it 0.7 s after it first appears.
param([int]$ProcId, [string]$Png, [string]$Title = "", [switch]$Maximize,
      [int]$Repeat = 1)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public static class W {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32")] public static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  public struct RECT { public int L, T, R, B; }
  public static IntPtr Find(uint pid, string title) {
    IntPtr found = IntPtr.Zero;
    EnumWindows(delegate (IntPtr h, IntPtr l) {
      uint p; GetWindowThreadProcessId(h, out p);
      if (p != pid || !IsWindowVisible(h)) return true;
      StringBuilder s = new StringBuilder(256); GetWindowText(h, s, 256);
      if (s.ToString().StartsWith(title)) { found = h; return false; }
      return true;
    }, IntPtr.Zero);
    return found;
  }
}
"@
$h = [IntPtr]::Zero
for ($i = 0; $i -lt $Repeat -and $h -eq [IntPtr]::Zero; $i++) {
  if ($Title) { $h = [W]::Find([uint32]$ProcId, $Title) } else { $h = (Get-Process -Id $ProcId).MainWindowHandle }
  if ($h -eq [IntPtr]::Zero -and $Repeat -gt 1) { Start-Sleep -Milliseconds 20 }
}
if ($h -eq [IntPtr]::Zero) { exit 1 }
if ($Repeat -gt 1) { Start-Sleep -Milliseconds 700 }
if ($Maximize) { [W]::ShowWindow($h, 3) | Out-Null; Start-Sleep 2 }
$r = New-Object W+RECT; [W]::GetWindowRect($h, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
$g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
[W]::PrintWindow($h, $dc, 2) | Out-Null
$g.ReleaseHdc($dc); $bmp.Save($Png)
