@echo off
setlocal
cd /d "%~dp0"
if not exist "%~dp0updates\first_setup.ps1" (
 echo [ERROR] updates\first_setup.ps1 not found. Unzip the whole ZIP first.
 pause
 exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0updates\first_setup.ps1"
if errorlevel 1 pause
exit /b 0
