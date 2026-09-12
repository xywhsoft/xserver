<?php
// php_router — PHP 内建服务器 + 静态数组 + 动态前缀数组（本文件按档生成，opcache 常驻）
$STATIC = array('/s/res000000/detail'=>1,'/s/res000001/detail'=>1,'/s/res000002/detail'=>1,'/s/res000003/detail'=>1,'/s/res000004/detail'=>1);
$DYN = array('/d/res000000/','/d/res000001/','/d/res000002/','/d/res000003/','/d/res000004/');
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
