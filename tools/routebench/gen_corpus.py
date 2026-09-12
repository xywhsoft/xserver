"""生成路由基准语料：每档 N 条 = N/2 静态 + N/2 动态，输出统一路由表与请求路径表"""
import sys, os

def gen(scale, outdir):
    os.makedirs(outdir, exist_ok=True)
    routes = []   # (type, pattern)
    paths = []    # 具体请求路径（均匀命中全部路由）
    half = scale // 2
    for i in range(half):
        routes.append(("S", f"/s/res{i:06d}/detail"))
        paths.append(f"/s/res{i:06d}/detail")
    for i in range(half):
        routes.append(("D", f"/d/res{i:06d}/{{id}}"))
        paths.append(f"/d/res{i:06d}/{1000000 + i}")
    with open(f"{outdir}/routes.txt", "w", newline=chr(10)) as f:
        for t, p in routes:
            f.write(f"{t} {p}\n")
    with open(f"{outdir}/paths.txt", "w", newline=chr(10)) as f:
        for p in paths:
            f.write(p + "\n")
    print(f"scale={scale} routes={len(routes)} paths={len(paths)} -> {outdir}")

if __name__ == "__main__":
    for scale in [int(x) for x in sys.argv[1].split(",")]:
        gen(scale, sys.argv[2] + f"/n{scale}")
