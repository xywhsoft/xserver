@echo off
rem xs3 baseline: build + config + TCC custom echo loop + graceful stop
setlocal
cd /d "%~dp0"

call build.bat
if errorlevel 1 exit /b 1
gcc tools\wingrp.c -O2 -s -o tools\wingrp.exe
if errorlevel 1 exit /b 1

cd release
if exist xs_smoke.log del xs_smoke.log
if exist ..\tools\wingrp.pid del ..\tools\wingrp.pid

start "" /b ..\tools\wingrp.exe run xs_smoke.log ..\tools\wingrp.pid xs.exe
powershell -Command "Start-Sleep -Milliseconds 3500" >nul

findstr /c:"config loaded" xs_smoke.log >nul || (echo SMOKE FAIL: config & exit /b 1)
findstr /c:"engine started" xs_smoke.log >nul || (echo SMOKE FAIL: engine & exit /b 1)
findstr /c:"script loaded" xs_smoke.log >nul || (echo SMOKE FAIL: tcc script & exit /b 1)
findstr /c:"server 'echo' custom ready" xs_smoke.log >nul || (echo SMOKE FAIL: custom assembly & exit /b 1)
findstr /c:"rounds ok=3/3" xs_smoke.log >nul || (echo SMOKE FAIL: crt/winapi probe & exit /b 1)
findstr /c:"server 'include_probe' custom ready" xs_smoke.log >nul || (echo SMOKE FAIL: sdk header probe & exit /b 1)
findstr /c:"tcp ready on" xs_smoke.log >nul || (echo SMOKE FAIL: tcp driver & exit /b 1)
findstr /c:"udp ready on" xs_smoke.log >nul || (echo SMOKE FAIL: udp driver & exit /b 1)

rem TCC custom echo loop check
python -c "import socket;s=socket.create_connection(('127.0.0.1',9099),timeout=3);s.settimeout(2);s.recv(200);s.sendall(b'xs3-smoke');import time;time.sleep(0.3);d=s.recv(200);s.close();exit(0 if d==b'xs3-smoke' else 1)"
if errorlevel 1 (echo SMOKE FAIL: tcc custom echo loop & exit /b 1)

rem tcp driver loop check (banner + echo)
python -c "import socket,time;s=socket.create_connection(('127.0.0.1',9097),timeout=3);s.settimeout(2);b=s.recv(200);s.sendall(b'tcp-smoke');time.sleep(0.3);d=s.recv(200);s.close();exit(0 if b.startswith(b'[xs3-tcp]') and d==b'tcp-smoke' else 1)"
if errorlevel 1 (echo SMOKE FAIL: tcp echo loop & exit /b 1)

rem udp driver dgram echo check
python -c "import socket;u=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);u.settimeout(2);u.sendto(b'udp-smoke',('127.0.0.1',9090));d,_=u.recvfrom(2048);u.close();exit(0 if d==b'udp-smoke' else 1)"
if errorlevel 1 (echo SMOKE FAIL: udp dgram loop & exit /b 1)

set /p XPID=<..\tools\wingrp.pid
..\tools\wingrp.exe signal %XPID%
powershell -Command "Start-Sleep -Milliseconds 3000" >nul

findstr /c:"engine stopped" xs_smoke.log >nul || (echo SMOKE FAIL: graceful stop & exit /b 1)
findstr /c:"[xs] bye" xs_smoke.log >nul || (echo SMOKE FAIL: exit & exit /b 1)

echo SMOKE PASS
exit /b 0
