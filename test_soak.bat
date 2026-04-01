@echo off
setlocal
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File tools\xs_soak_check.ps1 %*
exit /b %errorlevel%
