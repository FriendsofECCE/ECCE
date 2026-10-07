@echo off
rem Starts ECCE (the Organizer) from an unpacked Windows package, using only
rem what is in the package.  Nothing needs to be installed on the machine.
setlocal
set "ECCE_ROOT=%~dp0"
if "%ECCE_ROOT:~-1%"=="\" set "ECCE_ROOT=%ECCE_ROOT:~0,-1%"
rem Forward slashes: ECCE_HOME ends up in sh scripts, where a backslash is an escape.
set "ECCE_HOME=%ECCE_ROOT:\=/%"
set "PATH=%ECCE_ROOT%\bin;%ECCE_ROOT%\usr\bin;%ECCE_ROOT%\python;%ECCE_ROOT%\strawberry\perl\site\bin;%ECCE_ROOT%\strawberry\perl\bin;%ECCE_ROOT%\strawberry\c\bin;%SystemRoot%\System32;%SystemRoot%"
set "ECCE_REALUSER=%USERNAME%"
set "HOST=%COMPUTERNAME%"
set "UH=%USERPROFILE:\=/%"
set "ECCE_REALUSERHOME=%UH%"
set "ECCE_LOCAL_DATA=%UH%/ecce-local"
if not exist "%USERPROFILE%\ecce-local" mkdir "%USERPROFILE%\ecce-local"
rem Local mode without messaging and without a data server, as tested in CI.
set ECCE_NO_MESSAGING=1
set ECCE_NO_DATASERVER=1
set ECCE_SESSION_LIVENESS=lease
start "" "%ECCE_ROOT%\bin\organizer.exe" %*
