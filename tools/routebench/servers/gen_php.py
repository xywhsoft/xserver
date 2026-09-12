"""生成 PHP 路由文件（数组内嵌，按档）"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
routes_file, out_file = sys.argv[1], sys.argv[2]
static, dyn = [], []
for l in open(routes_file, encoding='utf-8'):
    if not l.strip(): continue
    t, p = l.split(maxsplit=1)
    p = p.strip()
    if t == 'S': static.append(p)
    else: dyn.append(p.replace('{id}', ''))
tpl = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'php_router_tpl.php'), encoding='utf-8').read()
s = ','.join(["'%s'=>1" % p for p in static])
d = "'" + "','".join(dyn) + "'"
open(out_file, 'w', encoding='utf-8').write(tpl.replace('__STATIC__', s).replace('__DYN__', d))
print(out_file, len(static), len(dyn))
