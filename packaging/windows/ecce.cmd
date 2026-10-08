@echo off
rem Starts ECCE (the Organizer) from an unpacked Windows package, using only
rem what is in the package.  Nothing needs to be installed on the machine.
setlocal
set "ECCE_ROOT=%~dp0"
if "%ECCE_ROOT:~-1%"=="\" set "ECCE_ROOT=%ECCE_ROOT:~0,-1%"
rem Forward slashes: ECCE_HOME ends up in sh scripts, where a backslash is an escape.
set "ECCE_HOME=%ECCE_ROOT:\=/%"
rem The user's own PATH last: codes installed on it (MOPAC adds itself) are
rem found by Register Machines' Find and by the job scripts.
set "PATH=%ECCE_ROOT%\bin;%ECCE_ROOT%\scripts;%ECCE_ROOT%\scripts\parsers;%ECCE_ROOT%\usr\bin;%ECCE_ROOT%\python;%ECCE_ROOT%\strawberry\perl\site\bin;%ECCE_ROOT%\strawberry\perl\bin;%ECCE_ROOT%\strawberry\c\bin;%SystemRoot%\System32;%SystemRoot%;%PATH%"
set "ECCE_REALUSER=%USERNAME%"
set "HOST=%COMPUTERNAME%"
set "UH=%USERPROFILE:\=/%"
set "ECCE_REALUSERHOME=%UH%"
rem Temporary files in the user's temp folder (there is no /tmp outside sh).
if not defined ECCE_TMPDIR set "ECCE_TMPDIR=%TEMP:\=/%"
set ECCE_SESSION_LIVENESS=lease
rem Where the user keeps their work is asked once, at the first start (#240):
rem the exit status 3 is "Quit" in that window; any other failure goes on.
if not defined ECCE_NO_FIRST_START if exist "%ECCE_ROOT%\python\python3w.exe" (
  start /wait "" "%ECCE_ROOT%\python\python3w.exe" "%ECCE_ROOT%\bin\ecce-first-start"
  if errorlevel 3 if not errorlevel 4 exit /b 0
)
rem A server given by the installation, or chosen in that window, makes this a
rem -remote session (what ecce-session-lib.sh does on the other systems).
set "ECCE_SERVER_SESSION="
if defined ECCE_LOCAL_DATA goto local
if defined ECCE_REMOTE_SERVER set "ECCE_SERVER_SESSION=1"
if exist "%ECCE_ROOT%\siteconfig\RemoteServer\DataServers" set "ECCE_SERVER_SESSION=1"
if not defined ECCE_SERVER_SESSION if exist "%USERPROFILE%\.ECCE\RemoteServer\DataServers" (
  set "ECCE_SERVER_SESSION=1"
  set "ECCE_REMOTE_DIR=%UH%/.ECCE/RemoteServer"
)
if defined ECCE_SERVER_SESSION goto server
:local
if not defined ECCE_LOCAL_DATA set "ECCE_LOCAL_DATA=%UH%/ecce-local"
if not exist "%USERPROFILE%\ecce-local" mkdir "%USERPROFILE%\ecce-local"
rem No data server; the session's own broker (ecce-broker-win, loopback only,
rem a login made up for the session) carries the messages between the tools.
set ECCE_NO_DATASERVER=1
set "ECCE_LOCAL_BROKER=1"
goto broker
:server
rem The central server's broker and data server: a session of its own, and the
rem broker file the apps read (ecce-gateway-start).
set ECCE_REMOTE_SERVER=1
:broker
for /f %%i in ('python3.exe -c "import os;print(os.urandom(8).hex())"') do set "ECCE_SESSION_ID=%%i"
bash.exe "%ECCE_HOME%/bin/ecce-gateway-start"
if errorlevel 1 (
  if defined ECCE_LOCAL_BROKER (
    echo ECCE could not start its message broker. 1>&2
    if not defined ECCE_NO_FIRST_START mshta "javascript:alert('ECCE could not start its message broker. See the files in your .ECCE folder.');close()"
    exit /b 2
  )
  echo ECCE could not reach its server. 1>&2
  rem Scripts and tests (ECCE_NO_FIRST_START) get no message box.
  if not defined ECCE_NO_FIRST_START mshta "javascript:alert('ECCE could not reach its server. Check that the server is running and your network connection, then start ECCE again.');close()"
  exit /b 2
)
start "" "%ECCE_ROOT%\bin\organizer.exe" %*
rem The local broker ends with the session: when no ECCE window is left.
if defined ECCE_LOCAL_BROKER start "" /b bash.exe "%ECCE_HOME%/bin/ecce-broker-win" watch
