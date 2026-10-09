# Logs what a process shows on screen, no input sent: every -Ms ms for
# -Seconds, the visible top-level windows of the newest process named -Name
# (or -ProcId), each with its rectangle and a hash of the screen pixels under
# it.  A line is written only when something changed, so the line count is
# the number of states a user saw (a window redrawn several times as it opens
# gives several lines).  The first line waits for the window to appear.
param([string]$Name = "", [int]$ProcId = 0, [int]$Seconds = 8, [int]$Ms = 25,
      [string]$Out = "")
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Text; using System.Collections.Generic; using System.Runtime.InteropServices;
public static class F {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32")] public static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  public struct RECT { public int L, T, R, B; }
  public static List<IntPtr> Of(uint pid) {
    var r = new List<IntPtr>();
    EnumWindows(delegate (IntPtr h, IntPtr l) {
      uint p; GetWindowThreadProcessId(h, out p);
      if (p == pid && IsWindowVisible(h)) r.Add(h);
      return true; }, IntPtr.Zero);
    return r; }
  public static string Title(IntPtr h) { var s = new StringBuilder(256); GetWindowText(h, s, 256); return s.ToString(); }
  public static uint FgPid() { uint p; GetWindowThreadProcessId(GetForegroundWindow(), out p); return p; }
}
"@
$md5 = [System.Security.Cryptography.MD5]::Create()
function Pid1 {
  if ($ProcId) { return $ProcId }
  $p = Get-Process -Name $Name -ErrorAction SilentlyContinue | Sort-Object StartTime | Select-Object -Last 1
  if ($p) { return $p.Id } else { return 0 }
}
$lines = @()
$last = ""
$t0 = [DateTime]::Now
$end = $t0.AddSeconds($Seconds)
while ([DateTime]::Now -lt $end) {
  $id = Pid1
  $state = @()
  if ($id) {
    foreach ($h in [F]::Of([uint32]$id)) {
      $r = New-Object F+RECT
      [void][F]::GetWindowRect($h, [ref]$r)
      $w = $r.R - $r.L; $hh = $r.B - $r.T
      $hash = "-"
      if ($w -gt 0 -and $hh -gt 0) { try {
        $bmp = New-Object System.Drawing.Bitmap $w, $hh
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
        $g.Dispose()
        $stream = New-Object System.IO.MemoryStream
        $bmp.Save($stream, [System.Drawing.Imaging.ImageFormat]::Bmp)
        $bmp.Dispose()
        $hash = [BitConverter]::ToString($md5.ComputeHash($stream.ToArray())).Replace("-", "").Substring(0, 8)
      } catch { $hash = "err:" + $_.Exception.Message.Substring(0, 20) } }
      $state += ("'{0}' {1},{2} {3}x{4} {5}" -f [F]::Title($h), $r.L, $r.T, $w, $hh, $hash)
    }
  }
  $fg = if ($id -and [F]::FgPid() -eq [uint32]$id) { "fg" } else { "bg" }
  $s = ($state -join " | ") + " " + $fg
  if ($s -ne $last) {
    $lines += ("{0,6} {1}" -f [int]([DateTime]::Now - $t0).TotalMilliseconds, $s)
    $last = $s
  }
  Start-Sleep -Milliseconds $Ms
}
if ($Out) { $lines | Set-Content $Out } else { $lines }
