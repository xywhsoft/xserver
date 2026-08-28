#!/usr/bin/env python3
import argparse
import socket
import struct
import sys


def recv_exact(obj_sock, i_need):
	obj_buf = bytearray()

	while len(obj_buf) < i_need:
		obj_chunk = obj_sock.recv(i_need - len(obj_buf))
		if not obj_chunk:
			raise ConnectionError('socket closed before reading enough bytes')
		obj_buf.extend(obj_chunk)

	return bytes(obj_buf)


def build_packet(i_msg_type, i_msg_id, s_cmd, tbl_params, s_body, i_status=0, i_flags=0):
	obj_cmd = s_cmd.encode('utf-8')
	obj_body = s_body.encode('utf-8')
	arr_param_info = []
	arr_param_data = []

	for s_key, s_value in tbl_params.items():
		obj_key = s_key.encode('utf-8')
		obj_value = s_value.encode('utf-8')
		arr_param_info.append(struct.pack('<HH', len(obj_key), len(obj_value)))
		arr_param_data.append(obj_key + obj_value)

	obj_param_info = b''.join(arr_param_info)
	obj_param_data = b''.join(arr_param_data)
	i_pack_size = 32 + len(obj_param_info) + len(obj_cmd) + len(obj_param_data) + len(obj_body)
	obj_head = struct.pack(
		'<4sIQHHHHIi',
		b'xtp\x02',
		i_pack_size,
		i_msg_id,
		i_flags,
		i_msg_type,
		len(obj_cmd),
		len(tbl_params),
		len(obj_body),
		i_status
	)
	return obj_head + obj_param_info + obj_cmd + obj_param_data + obj_body


def parse_packet(obj_packet):
	obj_head = obj_packet[:32]
	obj_magic, i_pack_size, i_msg_id, i_flags, i_msg_type, i_cmd_size, i_param_count, i_body_size, i_status = struct.unpack(
		'<4sIQHHHHIi',
		obj_head
	)

	if obj_magic != b'xtp\x02':
		raise ValueError('invalid xtp header magic')
	if i_pack_size != len(obj_packet):
		raise ValueError('invalid xtp packet size')

	i_offset = 32
	arr_param_info = []
	for _ in range(i_param_count):
		i_key_size, i_val_size = struct.unpack('<HH', obj_packet[i_offset:i_offset + 4])
		arr_param_info.append((i_key_size, i_val_size))
		i_offset += 4

	obj_cmd = obj_packet[i_offset:i_offset + i_cmd_size]
	i_offset += i_cmd_size
	tbl_params = {}
	for i_key_size, i_val_size in arr_param_info:
		obj_key = obj_packet[i_offset:i_offset + i_key_size]
		i_offset += i_key_size
		obj_val = obj_packet[i_offset:i_offset + i_val_size]
		i_offset += i_val_size
		tbl_params[obj_key.decode('utf-8')] = obj_val.decode('utf-8')

	obj_body = obj_packet[i_offset:i_offset + i_body_size]
	return {
		'msg_id': i_msg_id,
		'flags': i_flags,
		'msg_type': i_msg_type,
		'cmd': obj_cmd.decode('utf-8'),
		'params': tbl_params,
		'status': i_status,
		'body': obj_body.decode('utf-8')
	}


def main():
	obj_parser = argparse.ArgumentParser(description='push one log message to xserver xtp demo')
	obj_parser.add_argument('--host', default='127.0.0.1')
	obj_parser.add_argument('--port', default=18294, type=int)
	obj_parser.add_argument('--level', default='INFO')
	obj_parser.add_argument('--source', default='python-cli')
	obj_parser.add_argument('--message', required=True)
	obj_args = obj_parser.parse_args()

	obj_packet = build_packet(
		1,
		1,
		'log.push',
		{
			'level': obj_args.level,
			'source': obj_args.source
		},
		obj_args.message
	)

	with socket.create_connection((obj_args.host, obj_args.port), timeout=5) as obj_sock:
		obj_sock.sendall(obj_packet)
		obj_head = recv_exact(obj_sock, 32)
		_, i_pack_size, _, _, _, _, _, _, _ = struct.unpack('<4sIQHHHHIi', obj_head)
		obj_rest = recv_exact(obj_sock, i_pack_size - 32)
		tbl_resp = parse_packet(obj_head + obj_rest)

	print('status =', tbl_resp['status'])
	print('cmd =', tbl_resp['cmd'])
	print('params =', tbl_resp['params'])
	print('body =', tbl_resp['body'])
	return 0


if __name__ == '__main__':
	sys.exit(main())
