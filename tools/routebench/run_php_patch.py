"""补跑 PHP 各档并合并进 results.json"""
import json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_bench as rb
import statistics, subprocess, time

r = json.load(open(os.path.join(rb.ROOT, 'results.json'), encoding='utf-8'))
for scale in rb.SCALES:
    print('== PHP8.3 N=%d' % scale, flush=True)
    p, port = rb.start_php(scale)
    if not rb.wait_port(port, 60):
        print('timeout'); r['PHP8.3'][str(scale)] = None; rb.kill(p.pid); continue
    time.sleep(0.5)
    vals = rb.run_loadgen(port, os.path.join(rb.CORPUS, f'n{scale}', 'paths.txt'))
    rb.kill(p.pid); time.sleep(1)
    r['PHP8.3'][str(scale)] = vals
    if vals and all(x for x in vals):
        print('   %.1f us/req' % statistics.median(x[0] for x in vals))
json.dump(r, open(os.path.join(rb.ROOT, 'results.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print('merged')
