# HTTP/TLS complete-body receive window

HTTP RequestProc is called after the whole request body is buffered. The TLS
plaintext capacity must therefore reach the same receive window as TCP. A
process default TLS context with a 256 KiB plaintext queue stalls an admitted
larger body: HTTP waits for the final bytes before dispatch, while TLS waits for
HTTP to consume buffered bytes.

Each HTTP TLS listener now owns a context snapshot with the shared policy and
other limits, and PlainLimit equal to its effective recv_limit plus one TLS
plaintext record. That slack allows a final record to reach HTTP's rejection
boundary. Configured body/header/wire limits still reject excess requests; the
change neither preallocates the window nor enlarges the process context or
unrelated protocols. Addition overflow and context allocation fail startup.
ListenerStart retains its context, and every success/failure path releases the
temporary owner. Session/listener references handle endpoint retirement.

Run the bounded regression against the host being shipped:

    python tools/test_http_tls_receive.py --exe release/xs.exe

The isolated test covers default, larger and smaller windows, real HTTP/TLS
fixed/chunked requests, exact byte counts and SHA-256, subsequent requests on
the same connection, and header-only over-budget rejection. The largest normal
payload is 600 KiB. It does not run pressure or high-load tests. The committed
loopback test certificate is accepted only in the test client's TLS context.
