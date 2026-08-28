#!/usr/bin/env python3
import argparse
import json
import socket
import sys
import threading


def recv_loop(obj_sock):
	while True:
		try:
			obj_data = obj_sock.recv(4096)
		except OSError as obj_err:
			print('recv error:', obj_err)
			return
		if not obj_data:
			print('server closed')
			return
		print('<<', obj_data.decode('utf-8', errors='replace').strip())


def main():
	obj_parser = argparse.ArgumentParser(description='simple tcp peer demo client')
	obj_parser.add_argument('--host', default='127.0.0.1')
	obj_parser.add_argument('--port', default=18392, type=int)
	obj_parser.add_argument('--name', required=True)
	obj_args = obj_parser.parse_args()

	with socket.create_connection((obj_args.host, obj_args.port), timeout=5) as obj_sock:
		obj_thread = threading.Thread(target=recv_loop, args=(obj_sock,), daemon=True)
		obj_thread.start()
		print('connected to', obj_args.host, obj_args.port)
		print('input text and press enter, type /quit to exit')
		while True:
			try:
				s_text = input('>> ').strip()
			except EOFError:
				break
			if s_text == '/quit':
				break
			obj_payload = json.dumps({
				'name': obj_args.name,
				'text': s_text
			}, ensure_ascii=False) + '\n'
			obj_sock.sendall(obj_payload.encode('utf-8'))

	return 0


if __name__ == '__main__':
	sys.exit(main())
