@echo off
setlocal
rem Usage: build.bat [all] [LIB ...] [--list-extensions] [--help]
python "%~dp0tools\build.py" %*
exit /b %errorlevel%
