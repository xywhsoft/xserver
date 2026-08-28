import socket
import struct
import threading


def proc_recv_all(obj_conn, i_need):
	i_recv = b""
	while len(i_recv) < i_need:
		obj_buf = obj_conn.recv(i_need - len(i_recv))
		if not obj_buf:
			raise RuntimeError("recv closed")
		i_recv += obj_buf
	return i_recv


def proc_parse_packet(obj_data):
	_, _, i_msg_id, _, _, i_cmd_size, i_param_count, i_body_size, _ = struct.unpack("<4sIQHHHHIi", obj_data[:32])
	i_pos = 32
	arr_info = []
	for _ in range(i_param_count):
		arr_info.append(struct.unpack("<HH", obj_data[i_pos:i_pos + 4]))
		i_pos += 4
	s_cmd = obj_data[i_pos:i_pos + i_cmd_size].decode()
	i_pos += i_cmd_size
	tbl_param = {}
	for i_key_size, i_val_size in arr_info:
		s_key = obj_data[i_pos:i_pos + i_key_size].decode()
		i_pos += i_key_size
		s_val = obj_data[i_pos:i_pos + i_val_size].decode()
		i_pos += i_val_size
		tbl_param[s_key] = s_val
	obj_body = obj_data[i_pos:i_pos + i_body_size]
	return i_msg_id, s_cmd, tbl_param, obj_body


def proc_build_packet(i_msg_id, s_cmd, tbl_param, obj_body, i_status=0, i_msg_type=2, i_flags=0):
	if isinstance(obj_body, str):
		obj_body = obj_body.encode()
	obj_cmd = s_cmd.encode()
	arr_info = []
	obj_param_data = b""
	for s_key, s_val in tbl_param.items():
		obj_key = str(s_key).encode()
		obj_val = str(s_val).encode()
		arr_info.append(struct.pack("<HH", len(obj_key), len(obj_val)))
		obj_param_data += obj_key + obj_val
	i_pack_size = 32 + len(arr_info) * 4 + len(obj_cmd) + len(obj_param_data) + len(obj_body)
	obj_head = struct.pack("<4sIQHHHHIi", b"xtp\x02", i_pack_size, i_msg_id, i_flags, i_msg_type, len(obj_cmd), len(arr_info), len(obj_body), i_status)
	return obj_head + b"".join(arr_info) + obj_cmd + obj_param_data + obj_body


def proc_handle(obj_conn):
	try:
		obj_head = proc_recv_all(obj_conn, 32)
		_, i_pack_size, _, _, _, _, _, _, _ = struct.unpack("<4sIQHHHHIi", obj_head)
		obj_body = proc_recv_all(obj_conn, i_pack_size - 32)
		i_msg_id, s_cmd, tbl_param, _ = proc_parse_packet(obj_head + obj_body)
		if s_cmd == "demo.fail":
			obj_resp = proc_build_packet(i_msg_id, "xtp.error", {"result": "error"}, "{\"result\":\"error\",\"message\":\"remote fail\"}", i_status=409)
			obj_conn.sendall(obj_resp)
			return
		if s_cmd == "demo.json.reply":
			obj_resp = proc_build_packet(
				i_msg_id,
				"demo.json.reply",
				{"result": "ok", "kind": "json"},
				"{\"result\":\"ok\",\"message\":\"hello json xtp\",\"tag\":\"%s\"}" % tbl_param.get("tag", "")
			)
			obj_conn.sendall(obj_resp)
			return
		if s_cmd == "demo.simple.reply":
			obj_resp = proc_build_packet(i_msg_id, "demo.simple.reply", {"result": "ok"}, "hello simple xtp")
		else:
			obj_resp = proc_build_packet(i_msg_id, "demo.python", {"result": "ok", "echo": tbl_param.get("tag", "")}, "hello from temp xtp")
		obj_conn.sendall(obj_resp)
	finally:
		obj_conn.close()


if __name__ == "__main__":
	obj_srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	obj_srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	obj_srv.bind(("127.0.0.1", 9109))
	obj_srv.listen(8)
	print("temp xtp ready", flush=True)
	while True:
		obj_conn, _ = obj_srv.accept()
		threading.Thread(target=proc_handle, args=(obj_conn,), daemon=True).start()
