@echo off
rem xs3 baseline: build + config + TCC custom echo loop + graceful stop
setlocal
cd /d "%~dp0"
set XS_TEST_RC=1

call build.bat
if errorlevel 1 goto :done
gcc tools\wingrp.c -O2 -s -o tools\wingrp.exe
if errorlevel 1 goto :done

set XS_HTTP_PORT=
set XS_UDP_PORT=
set XS_TCP_PORT=
set XS_WS_PORT=
set XS_ECHO_PORT=
for /f "tokens=1-5" %%A in ('python tools\smoke_config.py release\xs.json release\xs_smoke_config.json') do (
	set XS_HTTP_PORT=%%A
	set XS_UDP_PORT=%%B
	set XS_TCP_PORT=%%C
	set XS_WS_PORT=%%D
	set XS_ECHO_PORT=%%E
)
if not defined XS_ECHO_PORT (echo SMOKE FAIL: temp port config & goto :done)

cd release
if exist xs_smoke.log del xs_smoke.log
if exist ..\tools\wingrp.pid del ..\tools\wingrp.pid

start "" /b ..\tools\wingrp.exe run xs_smoke.log ..\tools\wingrp.pid xs.exe xs_smoke_config.json
powershell -Command "Start-Sleep -Milliseconds 3500" >nul

findstr /c:"config loaded" xs_smoke.log >nul || (echo SMOKE FAIL: config & goto :done)
findstr /c:"engine started" xs_smoke.log >nul || (echo SMOKE FAIL: engine & goto :done)
findstr /c:"script loaded" xs_smoke.log >nul || (echo SMOKE FAIL: tcc script & goto :done)
findstr /c:"server 'echo' custom ready" xs_smoke.log >nul || (echo SMOKE FAIL: custom assembly & goto :done)
findstr /c:"rounds ok=3/3" xs_smoke.log >nul || (echo SMOKE FAIL: crt/winapi probe & goto :done)
findstr /c:"server 'include_probe' custom ready" xs_smoke.log >nul || (echo SMOKE FAIL: sdk header probe & goto :done)
findstr /c:"server 'tcp-echo' tcp bound on" xs_smoke.log >nul || (echo SMOKE FAIL: tcp driver & goto :done)
findstr /c:"server 'telemetry' udp bound on" xs_smoke.log >nul || (echo SMOKE FAIL: udp driver & goto :done)
findstr /c:"server 'main' http bound on" xs_smoke.log >nul || (echo SMOKE FAIL: http driver & goto :done)
findstr /c:"server 'ws-echo' ws bound on" xs_smoke.log >nul || (echo SMOKE FAIL: ws driver & goto :done)

rem WebSocket behavior check
python ../tools/smoke_ws.py --port %XS_WS_PORT%
if errorlevel 1 (echo SMOKE FAIL: ws behavior & goto :done)

rem HTTP behavior check: routes, static, errors, keep-alive, takeover
python ../tools/smoke_http.py --port %XS_HTTP_PORT%
if errorlevel 1 (echo SMOKE FAIL: http behavior & goto :done)

rem TCC custom echo loop check
python -c "import socket;s=socket.create_connection(('127.0.0.1',%XS_ECHO_PORT%),timeout=3);s.settimeout(2);s.recv(200);s.sendall(b'xs3-smoke');import time;time.sleep(0.3);d=s.recv(200);s.close();exit(0 if d==b'xs3-smoke' else 1)"
if errorlevel 1 (echo SMOKE FAIL: tcc custom echo loop & goto :done)

rem tcp driver loop check (banner + echo)
python -c "import socket,time;s=socket.create_connection(('127.0.0.1',%XS_TCP_PORT%),timeout=3);s.settimeout(2);b=s.recv(200);s.sendall(b'tcp-smoke');time.sleep(0.3);d=s.recv(200);s.close();exit(0 if b.startswith(b'[xs3-tcp]') and d==b'tcp-smoke' else 1)"
if errorlevel 1 (echo SMOKE FAIL: tcp echo loop & goto :done)

rem udp driver dgram echo check
python -c "import socket;u=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);u.settimeout(2);u.sendto(b'udp-smoke',('127.0.0.1',%XS_UDP_PORT%));d,_=u.recvfrom(2048);u.close();exit(0 if d==b'udp-smoke' else 1)"
if errorlevel 1 (echo SMOKE FAIL: udp dgram loop & goto :done)

set /p XPID=<..\tools\wingrp.pid
..\tools\wingrp.exe signal %XPID%
if errorlevel 1 (echo SMOKE FAIL: graceful signal & goto :done)
powershell -Command "Start-Sleep -Milliseconds 3000" >nul

findstr /c:"engine stopped" xs_smoke.log >nul || (echo SMOKE FAIL: graceful stop & goto :done)
findstr /c:"[xs] bye" xs_smoke.log >nul || (echo SMOKE FAIL: exit & goto :done)
if exist xs_smoke_config.json del xs_smoke_config.json
if exist ..\tools\wingrp.pid del ..\tools\wingrp.pid

rem Functional test: config matrix / behavior / reload semantics / idle
cd ..
python tools\func_test.py
if errorlevel 1 (echo FUNC FAIL & goto :done)

rem Generation lifetime: keep-alive old generation + structural rebuild + exact finalization
python tools\lifecycle_reload_test.py
if errorlevel 1 (echo LIFECYCLE FAIL & goto :done)

rem Same-endpoint listener handoff across HTTP/TCP/UDP/WS.
python tools\reload_matrix_test.py
if errorlevel 1 (echo RELOAD MATRIX FAIL & goto :done)
cd release

set XS_TEST_RC=0

:done
cd /d "%~dp0release"
if "%XS_TEST_RC%"=="0" goto :cleanup
if not exist ..\tools\wingrp.pid goto :cleanup
set XPID=
set /p XPID=<..\tools\wingrp.pid
if not defined XPID goto :cleanup
..\tools\wingrp.exe signal %XPID% >nul 2>&1
powershell -Command "Start-Sleep -Milliseconds 1500" >nul

:cleanup
if exist xs_smoke_config.json del xs_smoke_config.json
if exist ..\tools\wingrp.pid del ..\tools\wingrp.pid
cd /d "%~dp0"
if "%XS_TEST_RC%"=="0" echo SMOKE PASS
exit /b %XS_TEST_RC%
