@echo off
setlocal
rem Usage: build.bat [sqlite] [xtp] [--help]
python "%~dp0tools\build.py" %*
exit /b %errorlevel%
