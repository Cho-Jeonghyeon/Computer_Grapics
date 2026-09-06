[CmdletBinding()]
param(
    [string]$SolutionPath = (Join-Path (Split-Path $PSScriptRoot -Parent) 'computer_grapics.slnx'),
    [long]$TerminalWindowHandle = 0,
    [switch]$Inspect
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
if (-not ('ComgDesktop' -as [type])) {
    Add-Type @'
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public class ComgWindow {
    public long Handle;
    public int ProcessId;
    public string Title;
    public int X, Y, Width, Height;
}
public static class ComgDesktop {
    public delegate bool EnumProc(IntPtr h, IntPtr p);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr p);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder text, int n);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT rect);
    [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(IntPtr h, int attr, out RECT rect, int size);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int w, int ht, uint flags);
    [DllImport("user32.dll")] static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    public static void EnableDpi() { SetThreadDpiAwarenessContext(new IntPtr(-4)); }
    public static ComgWindow[] Windows() {
        var result = new List<ComgWindow>();
        EnumWindows((h,p) => {
            if (!IsWindowVisible(h)) return true;
            var s = new StringBuilder(2048); GetWindowText(h,s,s.Capacity);
            if (s.Length == 0) return true;
            uint pid; GetWindowThreadProcessId(h,out pid);
            RECT r;
            if (DwmGetWindowAttribute(h,9,out r,16)!=0) GetWindowRect(h,out r);
            result.Add(new ComgWindow { Handle=h.ToInt64(), ProcessId=(int)pid, Title=s.ToString(), X=r.L,Y=r.T,Width=r.R-r.L,Height=r.B-r.T });
            return true;
        },IntPtr.Zero);
        return result.ToArray();
    }
    public static void Place(long handle, int x, int y, int width, int height, bool bottom) {
        IntPtr h = new IntPtr(handle);
        ShowWindow(h,9);
        RECT outer, frame;
        GetWindowRect(h,out outer);
        int left=0,top=0,right=0,btm=0;
        if (DwmGetWindowAttribute(h,9,out frame,16)==0) {
            left=frame.L-outer.L; top=frame.T-outer.T;
            right=outer.R-frame.R; btm=outer.B-frame.B;
        }
        // Remove always-on-top, then order only the requested windows.
        SetWindowPos(h,new IntPtr(-2),0,0,0,0,0x0013);
        if (!SetWindowPos(h,bottom ? new IntPtr(1) : IntPtr.Zero,
            x-left,y-top,width+left+right,height+top+btm,0x0050))
            throw new InvalidOperationException("Could not position window " + handle);
    }
}
'@
}
[ComgDesktop]::EnableDpi()

function Get-ComgWindows {
    foreach ($window in [ComgDesktop]::Windows()) {
        $process = Get-Process -Id $window.ProcessId -ErrorAction SilentlyContinue
        if ($process) {
            $window | Add-Member -NotePropertyName ProcessName -NotePropertyValue $process.ProcessName -PassThru
        }
    }
}

function Find-ComgWindow([string]$Name, [string]$Title = '.*') {
    $matches = @(Get-ComgWindows | Where-Object { $_.ProcessName -match $Name -and $_.Title -match $Title })
    if ($matches.Count -gt 1) { throw "Multiple matching windows: $Name / $Title. Close duplicates or select a specific terminal handle." }
    if ($matches.Count -eq 1) { return $matches[0] }
}

if ($Inspect) {
    [System.Windows.Forms.Screen]::PrimaryScreen.WorkingArea | Format-List
    Get-ComgWindows | Where-Object { $_.ProcessName -match '^(devenv|ChatGPT|chrome|WindowsTerminal|powershell|pwsh)$' } |
        Select-Object Handle,ProcessName,Title,X,Y,Width,Height | Format-Table -AutoSize
    return
}

$SolutionPath = (Resolve-Path -LiteralPath $SolutionPath).Path
$solutionName = [regex]::Escape([System.IO.Path]::GetFileNameWithoutExtension($SolutionPath))
$vsWindow = Find-ComgWindow '^devenv$' "^$solutionName(?:\s|$)"
if (-not $vsWindow) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer / vswhere.exe not found.' }
    $vsExe = @(& $vswhere -latest -products '*' -find 'Common7\IDE\devenv.exe') | Select-Object -First 1
    if (-not $vsExe) { throw 'Visual Studio IDE not found.' }
    Start-Process -FilePath $vsExe -ArgumentList ('"' + $SolutionPath + '"')
}

$chatWindow = Find-ComgWindow '^ChatGPT$' '^ChatGPT$'
if (-not $chatWindow) {
    $chatApp = @(Get-StartApps | Where-Object { $_.Name -eq 'ChatGPT' }) | Select-Object -First 1
    if (-not $chatApp) { throw 'ChatGPT app not installed.' }
    Start-Process explorer.exe -ArgumentList ('shell:AppsFolder\' + $chatApp.AppID)
}

$musicWindow = Find-ComgWindow '^chrome$' 'YouTube Music'
if (-not $musicWindow) {
    $shortcut = Get-ChildItem -LiteralPath (Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs') -Filter '*YouTube Music*.lnk' -Recurse |
        Sort-Object FullName | Select-Object -First 1
    if ($shortcut) {
        Start-Process -FilePath $shortcut.FullName
    } else {
        $chromeExe = Join-Path $env:ProgramFiles 'Google\Chrome\Application\chrome.exe'
        if (-not (Test-Path -LiteralPath $chromeExe)) { throw 'Chrome not installed.' }
        Start-Process -FilePath $chromeExe -ArgumentList '--app=https://music.youtube.com'
    }
}

if ($TerminalWindowHandle) {
    $terminalWindow = Get-ComgWindows | Where-Object { $_.Handle -eq $TerminalWindowHandle -and $_.ProcessName -match '^(WindowsTerminal|powershell|pwsh)$' }
    if (-not $terminalWindow) { throw 'The specified terminal window does not exist.' }
} else {
    $terminalWindow = Find-ComgWindow '^(WindowsTerminal|powershell|pwsh)$'
    if (-not $terminalWindow) {
        # This visible PowerShell window is part of the requested workspace.
        $terminalProcess = Start-Process powershell.exe -WorkingDirectory (Split-Path $SolutionPath -Parent) -ArgumentList '-NoLogo','-NoExit' -PassThru
    }
}

$deadline = (Get-Date).AddSeconds(60)
do {
    $vsWindow = Find-ComgWindow '^devenv$' "^$solutionName(?:\s|$)"
    $chatWindow = Find-ComgWindow '^ChatGPT$' '^ChatGPT$'
    $musicWindow = Find-ComgWindow '^chrome$' 'YouTube Music'
    if (-not $terminalWindow) { $terminalWindow = Find-ComgWindow '^(WindowsTerminal|powershell|pwsh)$' }
    if ($vsWindow -and $chatWindow -and $musicWindow -and $terminalWindow) { break }
    Start-Sleep -Milliseconds 500
} while ((Get-Date) -lt $deadline)
if (-not ($vsWindow -and $chatWindow -and $musicWindow -and $terminalWindow)) {
    throw 'Timed out waiting for a required app window. Finish any app startup dialogs and run again.'
}

$area = [System.Windows.Forms.Screen]::PrimaryScreen.WorkingArea
$third = [int][Math]::Floor($area.Width / 3)
foreach ($pass in 1..3) {
    [ComgDesktop]::Place($musicWindow.Handle,$area.X,$area.Y,$third,$area.Height,$true)
    [ComgDesktop]::Place($terminalWindow.Handle,($area.X+$third),$area.Y,$third,$area.Height,$false)
    [ComgDesktop]::Place($vsWindow.Handle,($area.X+2*$third),$area.Y,($area.Width-2*$third),$area.Height,$false)
    [ComgDesktop]::Place($chatWindow.Handle,$area.X,$area.Y,$third,$area.Height,$false)
    if ($pass -lt 3) { Start-Sleep -Seconds 2 }
}
Write-Output 'Ready: ChatGPT + Music (left), PowerShell (center), Visual Studio (right).'
Get-ComgWindows | Where-Object { $_.Handle -in @($musicWindow.Handle,$terminalWindow.Handle,$vsWindow.Handle,$chatWindow.Handle) } |
    Select-Object Handle,ProcessName,Title,X,Y,Width,Height | Format-Table -AutoSize
