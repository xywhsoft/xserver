"""重跑 Node（修复行尾 + O(1) 动态键后）并合并进 results.json"""
import json, os, sys, statistics, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_bench as rb

r = json.load(open(os.path.join(rb.ROOT, 'results.json'), encoding='utf-8'))
for scale in rb.SCALES:
    print('== Node20 N=%d' % scale, flush=True)
    p, port = rb.start_node(scale)
    if not rb.wait_port(port, 60):
        print('timeout'); r['Node20'][str(scale)] = None; rb.kill(p.pid); continue
    time.sleep(0.5)
    vals = rb.run_loadgen(port, os.path.join(rb.CORPUS, f'n{scale}', 'paths.txt'))
    rb.kill(p.pid); time.sleep(1)
    r['Node20'][str(scale)] = vals
    if vals and all(x for x in vals):
        print('   %.1f us/req' % statistics.median(x[0] for x in vals))
json.dump(r, open(os.path.join(rb.ROOT, 'results.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print('merged')
