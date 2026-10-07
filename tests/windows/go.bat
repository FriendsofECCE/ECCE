@echo off
del %USERPROFILE%\shots\*.* /q
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name organizer -exe %USERPROFILE%\ECCE\build\organizer.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name builder -exe %USERPROFILE%\ECCE\build\builder.exe -arg %USERPROFILE%\ECCE\tests\fragreaders\data\glycine.pdb -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name calced -exe %USERPROFILE%\ECCE\build\calced.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name basistool -exe %USERPROFILE%\ECCE\build\basistool.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name pertable -exe %USERPROFILE%\ECCE\build\pertable.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name machbrowser -exe %USERPROFILE%\ECCE\build\machbrowser.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name machregister -exe %USERPROFILE%\ECCE\build\machregister.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File %USERPROFILE%\ECCE\tests\windows\runapp.ps1 -name launcher -exe %USERPROFILE%\ECCE\build\launcher.exe -wait 25
