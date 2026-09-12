"""Probe a built xs, including absent headers/symbols and nested TCC.

python tools/test_extensions_runtime.py --exe release/xs.exe sqlite xtp
Only temporary scripts/configs and short-lived child processes are used.
The custom probe explicitly exits its own process after completing checks;
it does not create a listener or touch any running xs instance.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from pathlib import Path

from xs_extensions import select_extensions

PROBE = r'''
#include <xsbase.h>
#include <libtcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do { if (!(x)) { printf("[extensions] FAIL: %s\n", #x); exit(5); } } while (0)

#ifdef XS_USE_SQLITE
#include <sqlite3.h>
static void probe_sqlite(void)
{
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    REQUIRE(sqlite3_open(":memory:", &db) == SQLITE_OK);
    REQUIRE(sqlite3_exec(db, "CREATE TABLE t(n); INSERT INTO t VALUES(7)", NULL, NULL, NULL) == SQLITE_OK);
    REQUIRE(sqlite3_prepare_v2(db, "SELECT n FROM t", -1, &stmt, NULL) == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 7);
    REQUIRE(sqlite3_finalize(stmt) == SQLITE_OK);
    REQUIRE(sqlite3_close(db) == SQLITE_OK);
}
#endif

#ifdef XS_USE_XTP2
#include <xtp2.h>
#ifdef XTP2_IMPLEMENTATION
#error The probe must call the host implementation, not compile another copy.
#endif
static void probe_xtp(void)
{
    unsigned char bytes[128];
    xtp2_packet p = {0};
    xtp2_config config;
    xtp2_processor *rx = NULL;
    xtp2_param param, field;
    xtp2_iter iter;
    xtp2_message message;
    xtp2_span value;
    size_t size, written, used;
    xtp2_config_init(&config);
    p.id = 7; p.type = XTP2_REQUEST; p.command = xtp2_text("ping");
    field.key = xtp2_text("a"); field.value = xtp2_text("b");
    p.params = &field; p.param_count = 1; p.body = xtp2_text("body");
    REQUIRE(xtp2_measure(&p, NULL, &size) == XTP2_OK);
    REQUIRE(xtp2_encode(&p, NULL, bytes, sizeof(bytes), &written) == XTP2_OK && written == size);
    REQUIRE(xtp2_create(&config, &rx) == XTP2_OK);
    REQUIRE(xtp2_feed(rx, bytes, 7, &used, &message) == XTP2_MORE && used == 7);
    REQUIRE(xtp2_feed(rx, bytes + 7, written - 7, &used, &message) == XTP2_MESSAGE);
    REQUIRE(used == written - 7 && message.id == 7 && xtp2_wants_reply(&message));
    REQUIRE(message.body.size == 4 && memcmp(message.body.data, "body", 4) == 0);
    iter = xtp2_params(&message);
    REQUIRE(xtp2_param_next(&iter, &param) && param.key.size == 1 && param.value.size == 1);
    REQUIRE(!xtp2_param_next(&iter, &param));
    REQUIRE(xtp2_find(&message, xtp2_text("a"), &value) && value.size == 1);
    REQUIRE(*(const char *)value.data == 'b');
    REQUIRE(strcmp(xtp2_error_string(XTP2_OK), "ok") == 0);
    REQUIRE(xtp2_finish(rx) == XTP2_OK);
    xtp2_reset(rx);
    REQUIRE(xtp2_finish(rx) == XTP2_OK);
    xtp2_destroy(rx);
    REQUIRE(xtp2_decode(bytes, written, NULL, &message) == XTP2_OK && message.id == 7);
}
#endif

static const char nested_source[] =
    "#ifdef XS_USE_SQLITE\n#include <sqlite3.h>\n#endif\n"
    "#ifdef XS_USE_XTP2\n#include <xtp2.h>\n#endif\n"
    "int nested(void) { int mask = 0;\n"
    "#ifdef XS_USE_SQLITE\nif(sqlite3_libversion_number()>0) mask |= 1;\n#endif\n"
    "#ifdef XS_USE_XTP2\nif(xtp2_error_string(XTP2_OK)[0]=='o') mask |= 2;\n#endif\n"
    "return mask; }\n";

void ServiceInit(XS_HostInfo *host)
{
    TCCState *nested;
    int (*fn)(void);
    int mask = 0;
    (void)host;
#ifdef XS_USE_SQLITE
    mask |= 1; probe_sqlite();
#endif
#ifdef XS_USE_XTP2
    mask |= 2; probe_xtp();
#endif
    REQUIRE(mask == EXPECTED_MASK);
    nested = xsCreateTCC();
    REQUIRE(nested != NULL && tcc_compile_string(nested, nested_source) == 0);
    REQUIRE(tcc_relocate(nested) == 0);
    fn = (int (*)(void))tcc_get_symbol(nested, "nested");
    REQUIRE(fn != NULL && fn() == EXPECTED_MASK);
    xsDestroyTCC(nested);
    printf("[extensions] ok mask=%d\n", mask);
    fflush(stdout);
    exit(0);
}
'''


def invoke(exe: Path, source: str, directory: Path, name: str) -> subprocess.CompletedProcess:
    script = directory / (name + ".c")
    config = directory / (name + ".json")
    script.write_text(source, encoding="utf-8")
    config.write_text(json.dumps({
        "engine": {"workers": 1},
        "services": [{"class": "custom", "name": name, "enabled": True,
                      "devlang": "c", "devfile": str(script)}],
    }), encoding="utf-8")
    return subprocess.run([str(exe), str(config)], cwd=directory, capture_output=True,
                          text=True, encoding="utf-8", errors="replace", timeout=25)


def check(exe: Path, names: list[str]) -> None:
    selected = select_extensions(names)
    mask = (1 if "sqlite" in selected else 0) | (2 if "xtp" in selected else 0)
    with tempfile.TemporaryDirectory(prefix="xs-extension-probe-") as temp:
        directory = Path(temp)
        proc = invoke(exe, f"#define EXPECTED_MASK {mask}\n" + PROBE, directory, "enabled")
        output = proc.stdout + proc.stderr
        if proc.returncode != 0 or f"[extensions] ok mask={mask}" not in output:
            raise RuntimeError(f"enabled/nested probe failed ({proc.returncode}):\n{output}")
        print(f"PASS enabled + nested TCC: {', '.join(selected) or '(none)'}", flush=True)
        absent = {
            "sqlite": ("sqlite3.h", "extern int sqlite3_libversion_number(void);",
                       "sqlite3_libversion_number()"),
            "xtp": ("xtp2.h", "extern const char *xtp2_error_string(int);",
                    "xtp2_error_string(0)"),
        }
        for name, (header, declaration, call) in absent.items():
            if name in selected:
                continue
            source = (f"#include <xsbase.h>\n#include <stdlib.h>\n#include <{header}>\n"
                      "void ServiceInit(XS_HostInfo *h){(void)h;exit(0);}\n")
            proc = invoke(exe, source, directory, name + "-absent-header")
            if proc.returncode != 1 or header not in proc.stdout + proc.stderr:
                raise RuntimeError(f"{name} header was not gated:\n{proc.stdout}{proc.stderr}")
            source = (f"#include <xsbase.h>\n#include <stdlib.h>\n{declaration}\n"
                      f"void ServiceInit(XS_HostInfo *h){{(void)h;(void){call};exit(0);}}\n")
            proc = invoke(exe, source, directory, name + "-absent-symbol")
            if proc.returncode != 1 or call.split("(")[0] not in proc.stdout + proc.stderr:
                raise RuntimeError(f"{name} symbol was not gated:\n{proc.stdout}{proc.stderr}")
            print(f"PASS absent header + symbol: {name}", flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("extensions", nargs="*")
    args = parser.parse_intermixed_args()
    try:
        check(args.exe.resolve(), args.extensions)
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"FAIL: {error}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
