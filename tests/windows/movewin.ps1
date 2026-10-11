# Moves the main window of a process (no input sent), e.g. out of the way of
# another window being photographed.
#   movewin.ps1 -ProcId <pid> -X <x> -Y <y>
param([int]$ProcId, [int]$X, [int]$Y)
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class M {
  [DllImport("user32")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
}
"@
$h = (Get-Process -Id $ProcId).MainWindowHandle
# SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
[void][M]::SetWindowPos($h, [IntPtr]::Zero, $X, $Y, 0, 0, 0x15)
