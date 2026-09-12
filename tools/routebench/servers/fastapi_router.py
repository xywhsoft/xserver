# fastapi_router — FastAPI 基准路由（启动时读取语料注册路由）
import os, sys
from fastapi import FastAPI
from fastapi.responses import Response

routes_file = os.environ.get("ROUTES", "")
app = FastAPI()

@app.get("/__bench_ok")
def ok_handler():
    return Response(content=b"ok", media_type="text/plain")

ok = ok_handler

for line in open(routes_file, encoding="utf-8"):
    line = line.strip()
    if not line:
        continue
    typ, path = line.split(maxsplit=1)
    app.add_api_route(path, ok, methods=["GET"], name=path.replace("/", "_")[:60])
