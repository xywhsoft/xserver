@echo off
setlocal
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File tools\xs_pressure_baseline.ps1 %*
exit /b %errorlevel%
