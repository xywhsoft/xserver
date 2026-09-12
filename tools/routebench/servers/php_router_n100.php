<?php
// php_router — PHP 内建服务器 + 静态数组 + 动态前缀数组（本文件按档生成，opcache 常驻）
$STATIC = array('/s/res000000/detail'=>1,'/s/res000001/detail'=>1,'/s/res000002/detail'=>1,'/s/res000003/detail'=>1,'/s/res000004/detail'=>1,'/s/res000005/detail'=>1,'/s/res000006/detail'=>1,'/s/res000007/detail'=>1,'/s/res000008/detail'=>1,'/s/res000009/detail'=>1,'/s/res000010/detail'=>1,'/s/res000011/detail'=>1,'/s/res000012/detail'=>1,'/s/res000013/detail'=>1,'/s/res000014/detail'=>1,'/s/res000015/detail'=>1,'/s/res000016/detail'=>1,'/s/res000017/detail'=>1,'/s/res000018/detail'=>1,'/s/res000019/detail'=>1,'/s/res000020/detail'=>1,'/s/res000021/detail'=>1,'/s/res000022/detail'=>1,'/s/res000023/detail'=>1,'/s/res000024/detail'=>1,'/s/res000025/detail'=>1,'/s/res000026/detail'=>1,'/s/res000027/detail'=>1,'/s/res000028/detail'=>1,'/s/res000029/detail'=>1,'/s/res000030/detail'=>1,'/s/res000031/detail'=>1,'/s/res000032/detail'=>1,'/s/res000033/detail'=>1,'/s/res000034/detail'=>1,'/s/res000035/detail'=>1,'/s/res000036/detail'=>1,'/s/res000037/detail'=>1,'/s/res000038/detail'=>1,'/s/res000039/detail'=>1,'/s/res000040/detail'=>1,'/s/res000041/detail'=>1,'/s/res000042/detail'=>1,'/s/res000043/detail'=>1,'/s/res000044/detail'=>1,'/s/res000045/detail'=>1,'/s/res000046/detail'=>1,'/s/res000047/detail'=>1,'/s/res000048/detail'=>1,'/s/res000049/detail'=>1);
$DYN = array('/d/res000000/','/d/res000001/','/d/res000002/','/d/res000003/','/d/res000004/','/d/res000005/','/d/res000006/','/d/res000007/','/d/res000008/','/d/res000009/','/d/res000010/','/d/res000011/','/d/res000012/','/d/res000013/','/d/res000014/','/d/res000015/','/d/res000016/','/d/res000017/','/d/res000018/','/d/res000019/','/d/res000020/','/d/res000021/','/d/res000022/','/d/res000023/','/d/res000024/','/d/res000025/','/d/res000026/','/d/res000027/','/d/res000028/','/d/res000029/','/d/res000030/','/d/res000031/','/d/res000032/','/d/res000033/','/d/res000034/','/d/res000035/','/d/res000036/','/d/res000037/','/d/res000038/','/d/res000039/','/d/res000040/','/d/res000041/','/d/res000042/','/d/res000043/','/d/res000044/','/d/res000045/','/d/res000046/','/d/res000047/','/d/res000048/','/d/res000049/');
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
