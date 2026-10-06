@echo off
del C:\Users\andy\shots\*.* /q
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name organizer -exe C:\Users\andy\ECCE\build\organizer.exe -wait 20
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\andy\runapp.ps1 -name builder -exe C:\Users\andy\ECCE\build\builder.exe -arg glycine.pdb -wait 30
