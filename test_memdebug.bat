@echo off
setlocal
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File tools\xs_memdebug_check.ps1 %*
exit /b %errorlevel%
