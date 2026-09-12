<?php
// php_router — PHP 内建服务器 + 静态数组 + 动态前缀数组（本文件按档生成，opcache 常驻）
$STATIC = array(__STATIC__);
$DYN = array(__DYN__);
$p = $_SERVER['REQUEST_URI'];
$dynHit = false;
foreach ($DYN as $pre) {
    if (strncmp($p, $pre, strlen($pre)) === 0) { $dynHit = true; break; }
}
if (isset($STATIC[$p]) || $dynHit) {
    header('Content-Type: text/plain');
    header('Content-Length: 2');
    echo 'ok';
} else {
    http_response_code(404);
}
