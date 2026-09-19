@echo off
setlocal
rem Usage: build.bat [all] [sqlite] [xtp] [xllm] [xllm-session] [xwork] [xmail] [xsmtp] [xpop3] [ximap] [md4c] [xacme] [qrcodegen] [webview] [--help]
python "%~dp0tools\build.py" %*
exit /b %errorlevel%
