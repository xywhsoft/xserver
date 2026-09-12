// node_router — Node 原生 http + Map(静态) + 前缀数组(动态) 两级路由
// 用法：node node_router.js <routes.txt> <port>
const http = require('http');
const fs = require('fs');

const [_, __, routesFile, portArg] = process.argv;
const port = parseInt(portArg, 10);
const staticMap = new Map();
const dynMap = new Map();

for (const line of fs.readFileSync(routesFile, 'utf8').split('\n')) {
	const m = line.trim().match(/^([SD]) (.+)$/);
	if (!m) continue;
	if (m[1] === 'S') staticMap.set(m[2], 1);
	else dynMap.set(m[2].replace(/\{id\}.*$/, ''), 1); // 键 = 末参数段前的前缀
}

const server = http.createServer((req, res) => {
	const p = req.url;
	if (staticMap.has(p) || hitDyn(p)) {
		res.writeHead(200, { 'Content-Type': 'text/plain', 'Content-Length': 2 });
		res.end('ok');
	} else {
		res.writeHead(404, { 'Content-Type': 'text/plain' });
		res.end();
	}
});
function hitDyn(p) {
	/* 末段为参数：取最后一个 '/' 之前（含）作键，O(1) 查表 */
	const cut = p.lastIndexOf('/');
	return dynMap.has(p.slice(0, cut + 1));
}
server.listen(port, '127.0.0.1', () => console.log('node ready', port));
