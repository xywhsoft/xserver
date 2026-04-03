#!/usr/bin/env python3
import argparse
import json
import socket
import sys
import threading


def recv_loop(obj_sock):
	while True:
		try:
			obj_data, tbl_addr = obj_sock.recvfrom(4096)
		except OSError as obj_err:
			print('recv error:', obj_err)
			return
		print('<<', tbl_addr, obj_data.decode('utf-8', errors='replace').strip())


def main():
	obj_parser = argparse.ArgumentParser(description='simple udp chat demo client')
	obj_parser.add_argument('--host', default='127.0.0.1')
	obj_parser.add_argument('--port', default=18493, type=int)
	obj_parser.add_argument('--name', required=True)
	obj_args = obj_parser.parse_args()

	with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as obj_sock:
		obj_sock.bind(('0.0.0.0', 0))
		obj_thread = threading.Thread(target=recv_loop, args=(obj_sock,), daemon=True)
		obj_thread.start()
		print('local bind =', obj_sock.getsockname())
		print('input: target message')
		print('example: bob hello udp')
		print('type /quit to exit')
		while True:
			try:
				s_line = input('>> ').strip()
			except EOFError:
				break
			if s_line == '/quit':
				break
			if ' ' not in s_line:
				print('format error, use: target message')
				continue
			s_target, s_message = s_line.split(' ', 1)
			obj_payload = json.dumps({
				'name': obj_args.name,
				'to': s_target.strip(),
				'text': s_message.strip()
			}, ensure_ascii=False).encode('utf-8')
			obj_sock.sendto(obj_payload, (obj_args.host, obj_args.port))

	return 0


if __name__ == '__main__':
	sys.exit(main())
