# Logs what a process shows on screen, no input sent: every -Ms ms for
# -Seconds, the visible top-level windows of the newest process named -Name
# (or -ProcId), each with its rectangle and a hash of the screen pixels under
# it.  A line is written only when something changed, so the line count is
# the number of states a user saw (a window redrawn several times as it opens
# gives several lines).  The first line waits for the window to appear.
# -Dump <dir> also saves each state seen as <dir>\<ms>-<n>.png (one per
# window), written after the run so saving does not slow the sampling.
# The sampling is C#: in PowerShell one sample took up to half a second,
# longer than the states worth seeing.
param([string]$Name = "", [int]$ProcId = 0, [int]$Seconds = 8, [int]$Ms = 25,
      [string]$Out = "", [string]$Dump = "")
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing @"
using System; using System.Text; using System.Collections.Generic; using System.Diagnostics;
using System.Drawing; using System.Drawing.Imaging; using System.Runtime.InteropServices;
public static class F {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32")] static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32")] static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32")] static extern IntPtr GetForegroundWindow();
  // Not GetWindowText: it waits for a busy window to answer, and a busy
  // window is when the states worth seeing are on screen.
  [DllImport("user32", CharSet = CharSet.Unicode)] static extern int InternalGetWindowText(IntPtr h, StringBuilder s, int n);
  struct RECT { public int L, T, R, B; }
  static List<IntPtr> Of(uint pid) {
    var r = new List<IntPtr>();
    EnumWindows(delegate (IntPtr h, IntPtr l) {
      uint p; GetWindowThreadProcessId(h, out p);
      if (p == pid && IsWindowVisible(h)) r.Add(h);
      return true; }, IntPtr.Zero);
    return r; }
  static uint Find(string name, uint known) {
    if (known != 0) { try { if (!Process.GetProcessById((int)known).HasExited) return known; } catch (Exception) {} }
    Process best = null;
    foreach (var p in Process.GetProcessesByName(name))
      if (best == null || p.StartTime > best.StartTime) best = p;
    return best == null ? 0 : (uint)best.Id;
  }
  static System.Security.Cryptography.MD5 md5 = System.Security.Cryptography.MD5.Create();
  static string Grab(RECT r, out Bitmap bmp) {
    int w = r.R - r.L, h = r.B - r.T;
    bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
    using (var g = Graphics.FromImage(bmp)) g.CopyFromScreen(r.L, r.T, 0, 0, bmp.Size);
    var d = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
    var buf = new byte[d.Stride * h];
    Marshal.Copy(d.Scan0, buf, 0, buf.Length);
    bmp.UnlockBits(d);
    return BitConverter.ToString(md5.ComputeHash(buf), 0, 4).Replace("-", "");
  }
  public static List<string> Run(string name, int procId, int seconds, int ms, string dump) {
    var lines = new List<string>();
    var frames = new List<KeyValuePair<string, Bitmap>>();
    var sw = Stopwatch.StartNew();
    string last = null;
    uint id = (uint)procId;
    int samples = 0;
    while (sw.ElapsedMilliseconds < seconds * 1000L) {
      samples++;
      if (name != "") id = Find(name, id);
      var state = new List<string>();
      var bmps = new List<Bitmap>();
      if (id != 0) {
        foreach (IntPtr h in Of(id)) {
          RECT r; GetWindowRect(h, out r);
          string hash = "-";
          if (r.R > r.L && r.B > r.T) {
            try { Bitmap b; hash = Grab(r, out b); bmps.Add(b); }
            catch (Exception e) { hash = "err:" + e.Message; }
          }
          var t = new StringBuilder(256); InternalGetWindowText(h, t, 256);
          state.Add(String.Format("'{0}' {1},{2} {3}x{4} {5}", t, r.L, r.T, r.R - r.L, r.B - r.T, hash));
        }
      }
      uint fgp; GetWindowThreadProcessId(GetForegroundWindow(), out fgp);
      string s = String.Join(" | ", state) + (id != 0 && fgp == id ? " fg" : " bg");
      if (s != last) {
        long t = sw.ElapsedMilliseconds;
        lines.Add(String.Format("{0,6} {1}", t, s));
        last = s;
        for (int k = 0; k < bmps.Count; k++) {
          if (dump != "") frames.Add(new KeyValuePair<string, Bitmap>(String.Format("{0:D6}-{1}", t, k), bmps[k]));
          else bmps[k].Dispose();
        }
      } else {
        foreach (var b in bmps) b.Dispose();
      }
      System.Threading.Thread.Sleep(ms);
    }
    lines.Add(String.Format("# {0} samples in {1} s", samples, seconds));
    if (dump != "") {
      System.IO.Directory.CreateDirectory(dump);
      foreach (var f in frames) { f.Value.Save(System.IO.Path.Combine(dump, f.Key + ".png"), ImageFormat.Png); f.Value.Dispose(); }
    }
    return lines;
  }
}
"@
$lines = [F]::Run($Name, $ProcId, $Seconds, $Ms, $Dump)
if ($Out) { $lines | Set-Content $Out } else { $lines }
