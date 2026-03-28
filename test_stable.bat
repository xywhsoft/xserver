@echo off
call build.bat
if errorlevel 1 exit /b %errorlevel%

call build_debug.bat
if errorlevel 1 exit /b %errorlevel%

powershell -ExecutionPolicy Bypass -File tools\xs_stable_smoke.ps1 %*
