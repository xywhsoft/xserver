


import socket
import struct



# 单例模式，Socket 对象
client = None



# 连接到服务器
def connect(ip, port):
	global client
	# 关闭旧连接
	if client != None:
		client.close()
		client = None
	# 连接到服务器
	client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	client.connect((ip, port))



# 关闭连接
def close():
	global client
	if client != None:
		client.close()
		client = None



# 发送消息
'''
	xtp + 版本				4字节
	总包大小					4字节
	cmd命令长度				2字节
	参数数量					2字节
	请求体长度				4字节
	param list：
		参数名称长度			2字节
		参数数据长度			2字节
	cmd命令
	param list：
		参数名称
		参数数据
	请求体
'''
def send(cmd, arg, body):
	global client
	PackSize = 16
	# 处理参数列表
	iParamCount = len(arg)
	sParamInfo = b""
	sParamData = b""
	iParamInfoSize = 4 * iParamCount
	iParamDataSize = 0
	for item in arg.items():
		sKey = item[0].encode('utf-8')
		sVal = item[1].encode('utf-8')
		iKeyLen = len(sKey)
		iValLen = len(sVal)
		if iKeyLen > 65535:
			iKeyLen = 65535
			sKey = sKey[:65535]
		if iValLen > 65535:
			iValLen = 65535
			sVal = sVal[:65535]
		sParamInfo += struct.pack("HH", iKeyLen, iValLen)
		sParamData += struct.pack(f"{iKeyLen}s{iValLen}s", sKey, sVal)
		iParamDataSize += iKeyLen + iValLen
		PackSize += 4 + iKeyLen + iValLen
	# 处理封包
	sCmd = cmd.encode('utf-8')
	iCmdLen = len(sCmd)
	sBody = None
	iBodyLen = None
	if isinstance(body, bytes):
		sBody = body
		iBodyLen = len(sBody)
	else:
		sBody = body.encode('utf-8')
		iBodyLen = len(sBody)
	PackSize += iCmdLen + iBodyLen
	sPack = struct.pack(f"cccbLHHL{iParamInfoSize}s{iCmdLen}s{iParamDataSize}s{iBodyLen}s", b'x', b't', b'p', 1, PackSize, iCmdLen, iParamCount, iBodyLen, sParamInfo, sCmd, sParamData, sBody)
	client.send(sPack)



if __name__ == "__main__":
	connect("127.0.0.1", 1216)
	send("log.add", {"session": "12345678901234567890123456789012", "text": "日志内容"}, "body域的内容")
	close()


