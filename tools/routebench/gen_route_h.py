"""从语料 routes.txt 生成 xs routebench 的 route.h"""
import sys
routes_file, out_file = sys.argv[1], sys.argv[2]
lines = [l.split(maxsplit=1) for l in open(routes_file) if l.strip()]
body = []
body.append("// 路由注册清单（由 gen_route_h.py 按语料生成）\n")
body.append("void RouteHTTP_Init()\n{\n")
body.append("\tG_StaticRouteTableHTTP = xrtMapCreate(sizeof(RouteInfoHTTP));\n\n")
for t, p in lines:
    p = p.strip()
    if t == "S":
        body.append('\tAddStaticRouteHTTP("%s", XHTTP_METHOD_GET, Bench_Static);\n' % p)
    else:
        body.append('\tAddDynamicRouteHTTP("%s", XHTTP_METHOD_GET, Bench_Dynamic);\n' % p)
body.append("\n\tRouteHTTP_Compile();\n}\n")
body.append("""
void RouteHTTP_Unit()
{
	if ( G_StaticRouteTableHTTP ) {
		xrtMapDestroy(G_StaticRouteTableHTTP);
		G_StaticRouteTableHTTP = NULL;
	}
	if ( G_DynamicRoutePattern ) {
		xrtPatternRelease(G_DynamicRoutePattern);
		G_DynamicRoutePattern = NULL;
	}
	{
		uint32 i;

		for ( i = 0; i < G_iDynCount; i++ ) {
			xrtFree(G_arrDynPattern[i]);
		}
		G_iDynCount = 0;
	}
}
""")
open(out_file, "w").write("".join(body))
print(f"{out_file}: {len(lines)} routes")
