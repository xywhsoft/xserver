#!/bin/sh

set -eu

HTTP_COUNT="${1:-50}"
WS_COUNT="${2:-20}"
XTP_COUNT="${3:-20}"
CUSTOM_COUNT="${4:-20}"
WAIT_SECS="${5:-10}"
PORT=8085
WS_PORT=8081
XTP_PORT=9096
CUSTOM_PORT=9098
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
RELEASE_DIR="$REPO_ROOT/release"
TOOL_DIR="$REPO_ROOT/tools"
BODY_FILE="$TOOL_DIR/.xs_pressure_body.tmp"
HEADER_FILE="$TOOL_DIR/.xs_pressure_header.tmp"
OS_NAME=$(uname -s 2>/dev/null || echo unknown)
RUN_TAG=$$

case "$OS_NAME" in
	MINGW*|MSYS*|CYGWIN*|Windows_NT)
		powershell -ExecutionPolicy Bypass -File "$TOOL_DIR/xs_pressure_baseline.ps1" "$HTTP_COUNT" "$WS_COUNT" "$XTP_COUNT" "$CUSTOM_COUNT" "$((WAIT_SECS * 1000))"
		exit $?
		;;
esac

XSDBG_BIN="xsdbg"
CLIENT_EXT=""
CLIENT_LIBS=""
WS_CLIENT_EXE=""
XTP_CLIENT_EXE=""
CUSTOM_CLIENT_EXE=""

proc_cleanup_generated_files() {
	rm -f \
		"$TOOL_DIR"/ws_smoke_client.pressure.* \
		"$TOOL_DIR"/xtp_smoke_client.pressure.* \
		"$TOOL_DIR"/xtp_pressure_client.pressure.* \
		"$TOOL_DIR"/custom_smoke_client.pressure.* \
		"$BODY_FILE" \
		"$HEADER_FILE"
}

proc_fetch_status_method() {
	s_method="$1"
	s_url="$2"

	if [ "$s_method" = "HEAD" ]; then
		curl -s -I -D "$HEADER_FILE" -o "$BODY_FILE" -w "%{http_code}" "$s_url"
		return
	fi

	curl -s -X "$s_method" -D "$HEADER_FILE" -o "$BODY_FILE" -w "%{http_code}" "$s_url"
}

proc_fetch_body() {
	if [ ! -f "$BODY_FILE" ]; then
		return
	fi

	cat "$BODY_FILE"
}

proc_wait_ready() {
	i=0
	while [ "$i" -lt $((WAIT_SECS * 5)) ]; do
		if [ "$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/status_json" || true)" = "200" ]; then
			return 0
		fi
		i=$((i + 1))
		sleep 0.2
	done
	return 1
}

proc_wait_not_ready() {
	i=0
	while [ "$i" -lt $((WAIT_SECS * 5)) ]; do
		if [ "$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/status_json" || true)" != "200" ]; then
			return 0
		fi
		i=$((i + 1))
		sleep 0.2
	done
	return 1
}

proc_start_server() {
	s_config="$1"
	(
		cd "$RELEASE_DIR"
		"./$XSDBG_BIN" "$s_config" >/dev/null 2>&1 &
		echo $!
	)
}

proc_stop_server() {
	i_pid="${1:-}"

	if [ -n "$i_pid" ]; then
		kill "$i_pid" >/dev/null 2>&1 || true
	fi

	sleep 0.5
	proc_wait_not_ready >/dev/null 2>&1 || true
}

proc_build_c_client() {
	s_source="$1"
	s_output="$2"
	s_path="$TOOL_DIR/$s_output.pressure.$RUN_TAG$CLIENT_EXT"

	gcc "$TOOL_DIR/$s_source" -O2 -s $CLIENT_LIBS -o "$s_path"
	echo "$s_path"
}

proc_run_client_retry() {
	s_client_exe="$1"
	i_max_try="$2"
	shift 2

	i_try=0
	s_out=""
	while [ "$i_try" -lt "$i_max_try" ]; do
		if s_out=$("$s_client_exe" "$@" 2>&1); then
			printf "%s" "$s_out"
			return 0
		fi

		i_try=$((i_try + 1))
		if [ "$i_try" -lt "$i_max_try" ]; then
			sleep 0.2
		fi
	done

	printf "%s" "$s_out"
	return 1
}

proc_json_compact() {
	printf "%s" "$1" | tr -d '\r\n'
}

proc_json_get_int() {
	s_json=$(proc_json_compact "$1")
	s_key="$2"
	printf "%s" "$s_json" | sed -n "s/.*\"$s_key\":\\([-0-9][0-9]*\\).*/\\1/p" | head -n 1
}

proc_json_get_string() {
	s_json=$(proc_json_compact "$1")
	s_key="$2"
	printf "%s" "$s_json" | sed -n "s/.*\"$s_key\":\"\\([^\"]*\\)\".*/\\1/p" | head -n 1
}

proc_calc_avg_ms() {
	i_count="$1"
	i_elapsed_ms="$2"
	awk -v c="$i_count" -v e="$i_elapsed_ms" 'BEGIN { if (c <= 0) { printf "0.00" } else { printf "%.2f", e / c } }'
}

proc_calc_rps() {
	i_count="$1"
	i_elapsed_ms="$2"
	awk -v c="$i_count" -v e="$i_elapsed_ms" 'BEGIN { if (e <= 0) { printf "0.00" } else { printf "%.2f", (c * 1000.0) / e } }'
}

proc_now_ms() {
	date +%s%3N
}

proc_print_header() {
	printf "[%s]\n" "$1"
}

proc_run_http_baseline() {
	i_pid=""

	proc_print_header "http"
	if [ "$HTTP_COUNT" -le 0 ]; then
		printf "config=xs_manage_test.json\nskipped=true\nrequests=0\n"
		return 0
	fi

	i_pid=$(proc_start_server "xs_manage_test.json")
	if ! proc_wait_ready; then
		printf "FAIL http ready timeout\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/http_metrics_clear" || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL http_metrics_clear status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_success=0
	i_start=$(proc_now_ms)
	i=1
	while [ "$i" -le "$HTTP_COUNT" ]; do
		i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/json?run=$RUN_TAG&seq=$i" || true)
		s_body=$(proc_fetch_body 2>/dev/null || true)
		if [ "$i_status" != "200" ] || ! printf "%s" "$s_body" | grep -F '"path":"/json"' >/dev/null 2>&1; then
			printf "FAIL http seq=%s status=%s\n" "$i" "$i_status"
			proc_stop_server "$i_pid"
			return 1
		fi
		i_success=$((i_success + 1))
		i=$((i + 1))
	done
	i_end=$(proc_now_ms)
	i_elapsed_ms=$((i_end - i_start))

	sleep 0.2
	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/http_metrics_json" || true)
	s_metric=$(proc_fetch_body 2>/dev/null || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL http_metrics_json status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_app_count=$(proc_json_get_int "$s_metric" "http_app_req_count")
	s_manage_count=$(proc_json_get_int "$s_metric" "http_manage_req_count")
	s_time_avg=$(proc_json_get_int "$s_metric" "http_time_avg_ms")
	s_last_path=$(proc_json_get_string "$s_metric" "http_last_app_path")

	if [ -z "$i_app_count" ] || [ "$i_app_count" -ne "$HTTP_COUNT" ]; then
		printf "FAIL http_app_req_count expected=%s got=%s\n" "$HTTP_COUNT" "${i_app_count:-missing}"
		proc_stop_server "$i_pid"
		return 1
	fi

	printf "config=xs_manage_test.json\n"
	printf "requests=%s\n" "$HTTP_COUNT"
	printf "success=%s\n" "$i_success"
	printf "failures=%s\n" "$((HTTP_COUNT - i_success))"
	printf "elapsed_ms=%s\n" "$i_elapsed_ms"
	printf "avg_ms=%s\n" "$(proc_calc_avg_ms "$i_success" "$i_elapsed_ms")"
	printf "rps=%s\n" "$(proc_calc_rps "$i_success" "$i_elapsed_ms")"
	printf "server_http_app_req_count=%s\n" "$i_app_count"
	printf "server_http_manage_req_count=%s\n" "${s_manage_count:-0}"
	printf "server_http_time_avg_ms=%s\n" "${s_time_avg:-0}"
	printf "server_http_last_app_path=%s\n" "${s_last_path:-}"

	proc_stop_server "$i_pid"
	return 0
}

proc_run_ws_baseline() {
	i_pid=""

	proc_print_header "ws"
	if [ "$WS_COUNT" -le 0 ]; then
		printf "config=xs_manage_ws_test.json\nskipped=true\nrequests=0\n"
		return 0
	fi

	if [ -z "$WS_CLIENT_EXE" ]; then
		WS_CLIENT_EXE=$(proc_build_c_client "ws_smoke_client.c" "ws_smoke_client")
	fi

	i_pid=$(proc_start_server "xs_manage_ws_test.json")
	if ! proc_wait_ready; then
		printf "FAIL ws ready timeout\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/ws_metrics_clear" || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL ws_metrics_clear status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi
	sleep 0.5

	i_success=0
	i_start=$(proc_now_ms)
	i=1
	while [ "$i" -le "$WS_COUNT" ]; do
		s_text="baseline-ws-$i"
		if ! s_out=$(proc_run_client_retry "$WS_CLIENT_EXE" 10 "127.0.0.1" "$WS_PORT" "$s_text"); then
			printf "FAIL ws client seq=%s\n" "$i"
			proc_stop_server "$i_pid"
			return 1
		fi
		if ! printf "%s" "$s_out" | grep -F "ws demo" >/dev/null 2>&1 || \
			! printf "%s" "$s_out" | grep -F "text=$s_text" >/dev/null 2>&1 || \
			! printf "%s" "$s_out" | grep -F "protocol=xs-demo" >/dev/null 2>&1; then
			printf "FAIL ws body seq=%s\n" "$i"
			proc_stop_server "$i_pid"
			return 1
		fi
		i_success=$((i_success + 1))
		i=$((i + 1))
	done
	i_end=$(proc_now_ms)
	i_elapsed_ms=$((i_end - i_start))

	sleep 0.3
	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/ws_metrics_json" || true)
	s_metric=$(proc_fetch_body 2>/dev/null || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL ws_metrics_json status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_open_count=$(proc_json_get_int "$s_metric" "ws_open_count")
	i_close_count=$(proc_json_get_int "$s_metric" "ws_close_count")
	i_text_count=$(proc_json_get_int "$s_metric" "ws_text_count")
	i_conn_current=$(proc_json_get_int "$s_metric" "ws_conn_current")
	s_last_text=$(proc_json_get_string "$s_metric" "ws_last_text")

	if [ -z "$i_open_count" ] || [ "$i_open_count" -lt "$WS_COUNT" ] || \
		[ -z "$i_close_count" ] || [ "$i_close_count" -lt "$WS_COUNT" ] || \
		[ -z "$i_text_count" ] || [ "$i_text_count" -lt "$WS_COUNT" ] || \
		[ -z "$i_conn_current" ] || [ "$i_conn_current" -ne 0 ]; then
		printf "FAIL ws metrics mismatch\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	printf "config=xs_manage_ws_test.json\n"
	printf "requests=%s\n" "$WS_COUNT"
	printf "success=%s\n" "$i_success"
	printf "failures=%s\n" "$((WS_COUNT - i_success))"
	printf "elapsed_ms=%s\n" "$i_elapsed_ms"
	printf "avg_ms=%s\n" "$(proc_calc_avg_ms "$i_success" "$i_elapsed_ms")"
	printf "rps=%s\n" "$(proc_calc_rps "$i_success" "$i_elapsed_ms")"
	printf "server_ws_open_count=%s\n" "$i_open_count"
	printf "server_ws_close_count=%s\n" "$i_close_count"
	printf "server_ws_text_count=%s\n" "$i_text_count"
	printf "server_ws_last_text=%s\n" "${s_last_text:-}"

	proc_stop_server "$i_pid"
	return 0
}

proc_run_xtp_baseline() {
	i_pid=""

	proc_print_header "xtp"
	if [ "$XTP_COUNT" -le 0 ]; then
		printf "config=xs_manage_xtp_test.json\nskipped=true\nrequests=0\n"
		return 0
	fi

	if [ -z "$XTP_CLIENT_EXE" ]; then
		XTP_CLIENT_EXE=$(proc_build_c_client "xtp_pressure_client.c" "xtp_pressure_client")
	fi

	i_pid=$(proc_start_server "xs_manage_xtp_test.json")
	if ! proc_wait_ready; then
		printf "FAIL xtp ready timeout\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/xtp_metrics_clear" || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL xtp_metrics_clear status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi
	sleep 0.5

	i_start=$(proc_now_ms)
	if ! s_out=$(proc_run_client_retry "$XTP_CLIENT_EXE" 10 "127.0.0.1" "$XTP_PORT" "$XTP_COUNT" "demo.ping" "tag=baseline-xtp-$RUN_TAG"); then
		printf "FAIL xtp client\n"
		proc_stop_server "$i_pid"
		return 1
	fi
	i_end=$(proc_now_ms)
	i_elapsed_ms=$((i_end - i_start))

	if ! printf "%s" "$s_out" | grep -F "ok_count=$XTP_COUNT" >/dev/null 2>&1 || \
		! printf "%s" "$s_out" | grep -F "last_status=0" >/dev/null 2>&1 || \
		! printf "%s" "$s_out" | grep -F "last_cmd=xtp.reply" >/dev/null 2>&1; then
		printf "FAIL xtp client output\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_success="$XTP_COUNT"

	sleep 0.3
	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/xtp_metrics_json" || true)
	s_metric=$(proc_fetch_body 2>/dev/null || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL xtp_metrics_json status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_open_count=$(proc_json_get_int "$s_metric" "xtp_open_count")
	i_close_count=$(proc_json_get_int "$s_metric" "xtp_close_count")
	i_msg_count=$(proc_json_get_int "$s_metric" "xtp_msg_count")
	i_req_count=$(proc_json_get_int "$s_metric" "xtp_req_count")
	i_send_count=$(proc_json_get_int "$s_metric" "xtp_send_count")
	i_conn_current=$(proc_json_get_int "$s_metric" "xtp_conn_current")
	s_last_cmd=$(proc_json_get_string "$s_metric" "xtp_last_cmd")

	if [ -z "$i_open_count" ] || [ "$i_open_count" -lt 1 ] || \
		[ -z "$i_close_count" ] || [ "$i_close_count" -lt 1 ] || \
		[ -z "$i_msg_count" ] || [ "$i_msg_count" -lt "$XTP_COUNT" ] || \
		[ -z "$i_req_count" ] || [ "$i_req_count" -lt "$XTP_COUNT" ] || \
		[ -z "$i_send_count" ] || [ "$i_send_count" -lt "$XTP_COUNT" ] || \
		[ -z "$i_conn_current" ] || [ "$i_conn_current" -ne 0 ]; then
		printf "FAIL xtp metrics mismatch\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	printf "config=xs_manage_xtp_test.json\n"
	printf "requests=%s\n" "$XTP_COUNT"
	printf "success=%s\n" "$i_success"
	printf "failures=%s\n" "$((XTP_COUNT - i_success))"
	printf "elapsed_ms=%s\n" "$i_elapsed_ms"
	printf "avg_ms=%s\n" "$(proc_calc_avg_ms "$i_success" "$i_elapsed_ms")"
	printf "rps=%s\n" "$(proc_calc_rps "$i_success" "$i_elapsed_ms")"
	printf "server_xtp_open_count=%s\n" "$i_open_count"
	printf "server_xtp_close_count=%s\n" "$i_close_count"
	printf "server_xtp_msg_count=%s\n" "$i_msg_count"
	printf "server_xtp_req_count=%s\n" "$i_req_count"
	printf "server_xtp_send_count=%s\n" "$i_send_count"
	printf "server_xtp_last_cmd=%s\n" "${s_last_cmd:-}"

	proc_stop_server "$i_pid"
	return 0
}

proc_run_custom_baseline() {
	i_pid=""

	proc_print_header "custom"
	if [ "$CUSTOM_COUNT" -le 0 ]; then
		printf "config=xs_manage_custom_test.json\nskipped=true\nrequests=0\n"
		return 0
	fi

	if [ -z "$CUSTOM_CLIENT_EXE" ]; then
		CUSTOM_CLIENT_EXE=$(proc_build_c_client "custom_smoke_client.c" "custom_smoke_client")
	fi

	i_pid=$(proc_start_server "xs_manage_custom_test.json")
	if ! proc_wait_ready; then
		printf "FAIL custom ready timeout\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/custom_metrics_clear" || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL custom_metrics_clear status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi
	sleep 0.5

	i_success=0
	i_start=$(proc_now_ms)
	i=1
	while [ "$i" -le "$CUSTOM_COUNT" ]; do
		s_text="baseline-custom-$i"
		if ! s_out=$(proc_run_client_retry "$CUSTOM_CLIENT_EXE" 10 "127.0.0.1" "$CUSTOM_PORT" "$s_text"); then
			printf "FAIL custom client seq=%s\n" "$i"
			proc_stop_server "$i_pid"
			return 1
		fi
		if ! printf "%s" "$s_out" | grep -F "custom demo" >/dev/null 2>&1 || \
			! printf "%s" "$s_out" | grep -F "data=$s_text" >/dev/null 2>&1; then
			printf "FAIL custom body seq=%s\n" "$i"
			proc_stop_server "$i_pid"
			return 1
		fi
		i_success=$((i_success + 1))
		i=$((i + 1))
	done
	i_end=$(proc_now_ms)
	i_elapsed_ms=$((i_end - i_start))

	sleep 0.3
	i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/__xs/custom_metrics_json" || true)
	s_metric=$(proc_fetch_body 2>/dev/null || true)
	if [ "$i_status" != "200" ]; then
		printf "FAIL custom_metrics_json status=%s\n" "$i_status"
		proc_stop_server "$i_pid"
		return 1
	fi

	i_open_count=$(proc_json_get_int "$s_metric" "custom_open_count")
	i_close_count=$(proc_json_get_int "$s_metric" "custom_close_count")
	i_recv_count=$(proc_json_get_int "$s_metric" "custom_recv_count")
	i_send_count=$(proc_json_get_int "$s_metric" "custom_send_count")
	i_conn_current=$(proc_json_get_int "$s_metric" "custom_conn_current")
	s_last_text=$(proc_json_get_string "$s_metric" "custom_last_text")

	if [ -z "$i_open_count" ] || [ "$i_open_count" -lt "$CUSTOM_COUNT" ] || \
		[ -z "$i_close_count" ] || [ "$i_close_count" -lt "$CUSTOM_COUNT" ] || \
		[ -z "$i_recv_count" ] || [ "$i_recv_count" -lt "$CUSTOM_COUNT" ] || \
		[ -z "$i_send_count" ] || [ "$i_send_count" -lt "$CUSTOM_COUNT" ] || \
		[ -z "$i_conn_current" ] || [ "$i_conn_current" -ne 0 ]; then
		printf "FAIL custom metrics mismatch\n"
		proc_stop_server "$i_pid"
		return 1
	fi

	printf "config=xs_manage_custom_test.json\n"
	printf "requests=%s\n" "$CUSTOM_COUNT"
	printf "success=%s\n" "$i_success"
	printf "failures=%s\n" "$((CUSTOM_COUNT - i_success))"
	printf "elapsed_ms=%s\n" "$i_elapsed_ms"
	printf "avg_ms=%s\n" "$(proc_calc_avg_ms "$i_success" "$i_elapsed_ms")"
	printf "rps=%s\n" "$(proc_calc_rps "$i_success" "$i_elapsed_ms")"
	printf "server_custom_open_count=%s\n" "$i_open_count"
	printf "server_custom_close_count=%s\n" "$i_close_count"
	printf "server_custom_recv_count=%s\n" "$i_recv_count"
	printf "server_custom_send_count=%s\n" "$i_send_count"
	printf "server_custom_last_text=%s\n" "${s_last_text:-}"

	proc_stop_server "$i_pid"
	return 0
}

proc_cleanup_generated_files

if ! proc_run_http_baseline; then
	printf "[result]\nstatus=fail\n"
	proc_cleanup_generated_files
	exit 1
fi

if ! proc_run_ws_baseline; then
	printf "[result]\nstatus=fail\n"
	proc_cleanup_generated_files
	exit 1
fi

if ! proc_run_xtp_baseline; then
	printf "[result]\nstatus=fail\n"
	proc_cleanup_generated_files
	exit 1
fi

if ! proc_run_custom_baseline; then
	printf "[result]\nstatus=fail\n"
	proc_cleanup_generated_files
	exit 1
fi

printf "[result]\nstatus=ok\n"
proc_cleanup_generated_files
