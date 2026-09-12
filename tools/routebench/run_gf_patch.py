import json, os, sys, statistics, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_bench as rb
r = json.load(open(os.path.join(rb.ROOT, 'results.json'), encoding='utf-8'))
for name, starter in [('Go1.22', rb.start_go), ('FastAPI', rb.start_fastapi)]:
    for scale in [10, 10000]:
        p, port = starter(scale)
        if not rb.wait_port(port, 120):
            print(name, scale, 'timeout'); r[name][str(scale)] = None; rb.kill(p.pid); continue
        time.sleep(0.5)
        vals = rb.run_loadgen(port, os.path.join(rb.CORPUS, f'n{scale}', 'paths.txt'))
        rb.kill(p.pid); time.sleep(1)
        print(name, scale, '%.1f us/req' % statistics.median(x[0] for x in vals) if all(x for x in vals) else 'FAIL')
json.dump(r, open(os.path.join(rb.ROOT, 'results.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
