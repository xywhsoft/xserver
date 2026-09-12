"""路由基准总跑批：4 档 x 5 服务 x 3 轮 x 10000 请求，输出 JSON 与 Markdown 表"""
import json, os, socket, subprocess, sys, time, statistics

ROOT = os.path.dirname(os.path.abspath(__file__))          # tools/routebench
REPO = os.path.abspath(os.path.join(ROOT, "..", ".."))
CORPUS = os.path.join(ROOT, "corpus")
SERVERS = os.path.join(ROOT, "servers")
LOADGEN = os.path.join(ROOT, "loadgen.exe")
REL = os.path.join(REPO, "release")
SCALES = [10, 100, 1000, 10000]
ROUNDS = 3
TOTAL = 10000
NODE = r"D:\runtimes\node-v20.18.1-win-x64\node.exe"
PHP = r"D:\runtimes\php\php.exe"
PY = sys.executable

def wait_port(port, timeout=90):
    t0 = time.time()
    while time.time() - t0 < timeout:
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=1)
            s.close()
            return True
        except OSError:
            time.sleep(0.3)
    return False

def run_loadgen(port, paths):
    for _ in range(3):  # 端口级预热尝试
        r = subprocess.run([LOADGEN, "127.0.0.1", str(port), paths, str(100), "20"],
                           capture_output=True, text=True, timeout=60)
        if r.returncode == 0 and "nan" not in r.stdout:
            break
        time.sleep(0.5)
    vals = []
    for i in range(ROUNDS):
        r = subprocess.run([LOADGEN, "127.0.0.1", str(port), paths, str(TOTAL), "200"],
                           capture_output=True, text=True, timeout=300)
        if r.returncode != 0 or "nan" in r.stdout:
            vals.append(None)
        else:
            us, rps = r.stdout.split()
            vals.append((float(us), float(rps)))
    return vals

def kill(pid):
    subprocess.run(["taskkill", "/F", "/T", "/PID", str(pid)],
                   capture_output=True)

def start_xs(scale):
    subprocess.run([PY, os.path.join(ROOT, "gen_route_h.py"),
                    os.path.join(CORPUS, f"n{scale}", "routes.txt"),
                    os.path.join(REL, "routebench", "route.h")], check=True, capture_output=True)
    p = subprocess.Popen([os.path.join(REL, "xs.exe"), "routebench/xs.json"],
                         cwd=REL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return p, 9091

def start_go(scale):
    p = subprocess.Popen([os.path.join(SERVERS, "go_router.exe"),
                          "-routes", os.path.join(CORPUS, f"n{scale}", "routes.txt"),
                          "-port", "9092"], cwd=SERVERS,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return p, 9092

def start_node(scale):
    p = subprocess.Popen([NODE, os.path.join(SERVERS, "node_router.js"),
                          os.path.join(CORPUS, f"n{scale}", "routes.txt"), "9094"],
                         cwd=SERVERS, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return p, 9094

def start_php(scale):
    out = os.path.abspath(os.path.join(SERVERS, f"php_router_n{scale}.php"))
    subprocess.run([PY, os.path.abspath(os.path.join(SERVERS, "gen_php.py")),
                    os.path.join(CORPUS, f"n{scale}", "routes.txt"), out],
                   check=True, capture_output=True)
    p = subprocess.Popen([PHP, "-d", "opcache.enable_cli=1",
                          "-S", "127.0.0.1:9093", out], cwd=SERVERS,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return p, 9093

def start_fastapi(scale):
    env = dict(os.environ)
    env["ROUTES"] = os.path.join(CORPUS, f"n{scale}", "routes.txt")
    p = subprocess.Popen([PY, "-m", "uvicorn", "fastapi_router:app",
                          "--host", "127.0.0.1", "--port", "9096", "--workers", "1",
                          "--log-level", "warning"],
                         cwd=SERVERS, env=env,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return p, 9096

SERVER_LIST = [
    ("xs(C/TCC)", start_xs),
    ("Go1.22", start_go),
    ("Node20", start_node),
    ("PHP8.3", start_php),
    ("FastAPI", start_fastapi),
]

def main():
    results = {}
    for scale in SCALES:
        for name, starter in SERVER_LIST:
            print(f"== {name} N={scale} ...", flush=True)
            try:
                p, port = starter(scale)
                if not wait_port(port, 120):
                    print(f"   {name} N={scale} START TIMEOUT")
                    results.setdefault(name, {})[scale] = None
                    kill(p.pid)
                    time.sleep(1)
                    continue
                time.sleep(0.5)
                vals = run_loadgen(port, os.path.join(CORPUS, f"n{scale}", "paths.txt"))
                kill(p.pid)
                time.sleep(1)
                results.setdefault(name, {})[scale] = vals
                if vals and all(v for v in vals):
                    med = statistics.median(v[0] for v in vals)
                    print(f"   {name} N={scale}: {med:.1f} us/req", flush=True)
                else:
                    print(f"   {name} N={scale}: FAILED {vals}", flush=True)
            except Exception as e:
                print(f"   {name} N={scale} ERROR {e}", flush=True)
                results.setdefault(name, {})[scale] = None
    with open(os.path.join(ROOT, "results.json"), "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=1)
    print(json.dumps(results, ensure_ascii=False, indent=1))

if __name__ == "__main__":
    main()
