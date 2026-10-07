@echo off
del C:\Users\andy\shots\*.* /q
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name organizer -exe C:\Users\andy\ECCE\build\organizer.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name builder -exe C:\Users\andy\ECCE\build\builder.exe -arg glycine.pdb -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name calced -exe C:\Users\andy\ECCE\build\calced.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name basistool -exe C:\Users\andy\ECCE\build\basistool.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name pertable -exe C:\Users\andy\ECCE\build\pertable.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name machbrowser -exe C:\Users\andy\ECCE\build\machbrowser.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name machregister -exe C:\Users\andy\ECCE\build\machregister.exe -wait 25
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name launcher -exe C:\Users\andy\ECCE\build\launcher.exe -wait 25
