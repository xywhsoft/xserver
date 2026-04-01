#!/bin/sh

set -eu

HTTP_COUNT="${1:-12}"
WS_COUNT="${2:-4}"
XTP_COUNT="${3:-4}"
CUSTOM_COUNT="${4:-4}"
WAIT_SECS="${5:-10}"
PORT=8085
WS_PORT=8081
XTP_PORT=9096
CUSTOM_PORT=9098
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
RELEASE_DIR="$REPO_ROOT/release"
TOOL_DIR="$REPO_ROOT/tools"
BODY_FILE="$TOOL_DIR/.xs_memdebug_body.tmp"
OS_NAME=$(uname -s 2>/dev/null || echo unknown)
RUN_TAG=$$
TEMP_ROOT="${TMPDIR:-/tmp}/xs_memdebug_check_$RUN_TAG"

case "$OS_NAME" in
	MINGW*|MSYS*|CYGWIN*|Windows_NT)
		powershell -ExecutionPolicy Bypass -File "$TOOL_DIR/xs_memdebug_check.ps1" "$HTTP_COUNT" "$WS_COUNT" "$XTP_COUNT" "$CUSTOM_COUNT" "$((WAIT_SECS * 1000))"
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
		"$TOOL_DIR"/ws_smoke_client.memdebug.* \
		"$TOOL_DIR"/xtp_pressure_client.memdebug.* \
		"$TOOL_DIR"/custom_smoke_client.memdebug.* \
		"$BODY_FILE"
}

proc_cleanup_servers() {
	pkill -f "/$XSDBG_BIN" >/dev/null 2>&1 || true
	pkill -x "$XSDBG_BIN" >/dev/null 2>&1 || true
	sleep 0.5
}

proc_get_hash_text() {
	s_path="$1"

	if [ ! -f "$s_path" ]; then
		printf "(missing)"
		return
	fi

	sha256sum "$s_path" | awk '{ print $1 }'
}

proc_fetch_status_method() {
	s_method="$1"
	s_url="$2"

	if [ "$s_method" = "GET" ]; then
		curl -s --max-time 5 -o "$BODY_FILE" -w "%{http_code}" "$s_url"
		return
	fi

	curl -s --max-time 5 -X "$s_method" -o "$BODY_FILE" -w "%{http_code}" "$s_url"
}

proc_fetch_body() {
	if [ ! -f "$BODY_FILE" ]; then
		return
	fi

	cat "$BODY_FILE"
}

proc_fetch_json_text() {
	s_url="$1"
	i_status=$(proc_fetch_status_method "GET" "$s_url" || true)
	s_body=$(proc_fetch_body 2>/dev/null || true)

	if [ "$i_status" != "200" ] || [ -z "$s_body" ]; then
		printf "fetch json failed: %s status=%s\n" "$s_url" "$i_status" >&2
		return 1
	fi

	printf "%s" "$s_body"
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

proc_wait_file() {
	s_path="$1"
	i=0
	while [ "$i" -lt $((WAIT_SECS * 5)) ]; do
		if [ -f "$s_path" ]; then
			return 0
		fi
		i=$((i + 1))
		sleep 0.2
	done
	return 1
}

proc_build_c_client() {
	s_source="$1"
	s_output="$2"
	s_path="$TOOL_DIR/$s_output.memdebug.$RUN_TAG$CLIENT_EXT"

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

proc_prepare_run_dir() {
	s_case="$1"
	s_path="$TEMP_ROOT/$s_case"

	rm -rf "$s_path"
	mkdir -p "$s_path"
	printf "%s" "$s_path"
}

proc_start_server() {
	s_run_dir="$1"
	s_config="$2"
	s_log="$3"
	(
		cd "$s_run_dir"
		"$RELEASE_DIR/$XSDBG_BIN" "$RELEASE_DIR/$s_config" >"$s_log" 2>&1 &
		echo $!
	)
}

proc_stop_server() {
	i_pid="$1"

	kill -TERM "$i_pid" >/dev/null 2>&1 || true

	i=0
	while [ "$i" -lt $((WAIT_SECS * 5)) ]; do
		if ! kill -0 "$i_pid" >/dev/null 2>&1; then
			proc_wait_not_ready >/dev/null 2>&1 || true
			return 0
		fi
		i=$((i + 1))
		sleep 0.2
	done

	printf "graceful stop timeout pid=%s\n" "$i_pid" >&2
	return 1
}

proc_require_mem_debug_enabled() {
	s_case="$1"
	s_status=$(proc_fetch_json_text "http://127.0.0.1:$PORT/__xs/status_json" || true)

	if ! printf "%s" "$s_status" | grep -F '"mem_debug":true' >/dev/null 2>&1; then
		printf "%s mem_debug expected true\n" "$s_case" >&2
		return 1
	fi

	return 0
}

proc_report_key_value() {
	s_path="$1"
	s_key="$2"
	s_value=$(sed -n "s/.*\"$s_key\":[[:space:]]*\\([-0-9][0-9]*\\).*/\\1/p" "$s_path" | head -n 1)
	printf "%s" "${s_value:-}"
}

proc_case_baseline_value() {
	s_case="$1"
	s_key="$2"

	case "$s_key" in
		live_alloc_count)
			printf "4"
			return
			;;
		live_alloc_bytes)
			printf "128"
			return
			;;
		live_object_count|invalid_free_count|double_free_count|wrong_allocator_free_count|object_double_destroy_count|overflow_count|underflow_count)
			printf "0"
			return
			;;
	esac

	case "$s_case" in
		http)
			case "$s_key" in
				foreign_live_count) printf "900" ;;
				foreign_live_bytes) printf "43200" ;;
				*) printf -- "-1" ;;
			esac
			;;
		ws)
			case "$s_key" in
				foreign_live_count) printf "840" ;;
				foreign_live_bytes) printf "40320" ;;
				*) printf -- "-1" ;;
			esac
			;;
		xtp|custom)
			case "$s_key" in
				foreign_live_count) printf "840" ;;
				foreign_live_bytes) printf "40320" ;;
				*) printf -- "-1" ;;
			esac
			;;
		*)
			printf -- "-1"
			;;
	esac
}

proc_report_list_count() {
	s_path="$1"
	s_key="$2"
	s_json=$(tr -d '\r\n' < "$s_path")
	s_block=$(printf "%s" "$s_json" | sed -n "s/.*\"$s_key\":\\[\\(.*\\)\\][[:space:]]*,[[:space:]]*\"site_stats\".*/\\1/p" | head -n 1)

	if [ -z "$s_block" ]; then
		s_block=$(printf "%s" "$s_json" | sed -n "s/.*\"$s_key\":\\[\\(.*\\)\\][[:space:]]*}.*/\\1/p" | head -n 1)
	fi
	if [ -z "$s_block" ]; then
		printf "0"
		return
	fi
	if [ "$s_key" = "live_allocations" ]; then
		printf "%s" "$s_block" | grep -o '"type":"LEAK"' | wc -l | tr -d ' '
		return
	fi

	printf "%s" "$s_block" | grep -o '"type":"POOL_LEAK"' | wc -l | tr -d ' '
}

proc_assert_report_non_regression() {
	s_case="$1"
	s_report="$2"

	for s_key in \
		live_alloc_count \
		live_alloc_bytes \
		foreign_live_count \
		foreign_live_bytes \
		live_object_count \
		invalid_free_count \
		double_free_count \
		wrong_allocator_free_count \
		object_double_destroy_count \
		overflow_count \
		underflow_count
	do
		i_actual=$(proc_report_key_value "$s_report" "$s_key")
		i_baseline=$(proc_case_baseline_value "$s_case" "$s_key")
		if [ -z "$i_actual" ] || [ "$i_actual" -gt "$i_baseline" ]; then
			printf "%s report regression key=%s baseline=%s actual=%s\n" "$s_case" "$s_key" "$i_baseline" "${i_actual:-missing}" >&2
			return 1
		fi
	done

	i_live_count=$(proc_report_key_value "$s_report" "live_alloc_count")
	i_live_list=$(proc_report_list_count "$s_report" "live_allocations")
	i_foreign_count=$(proc_report_key_value "$s_report" "foreign_live_count")
	i_foreign_list=$(proc_report_list_count "$s_report" "foreign_live_allocations")

	if [ "$i_live_count" != "$i_live_list" ] || [ "$i_foreign_count" != "$i_foreign_list" ]; then
		printf "%s report list count mismatch\n" "$s_case" >&2
		return 1
	fi

	return 0
}

proc_print_case_summary() {
	s_case="$1"
	s_config="$2"
	i_requests="$3"
	s_log="$4"
	s_report="$5"

	printf "[%s]\n" "$s_case"
	printf "config=%s\n" "$s_config"
	printf "requests=%s\n" "$i_requests"
	printf "log=%s\n" "$s_log"
	printf "report=%s\n" "$s_report"
	printf "live_alloc_count=%s\n" "$(proc_report_key_value "$s_report" "live_alloc_count")"
	printf "live_alloc_bytes=%s\n" "$(proc_report_key_value "$s_report" "live_alloc_bytes")"
	printf "foreign_live_count=%s\n" "$(proc_report_key_value "$s_report" "foreign_live_count")"
	printf "foreign_live_bytes=%s\n" "$(proc_report_key_value "$s_report" "foreign_live_bytes")"
	printf "invalid_free_count=%s\n" "$(proc_report_key_value "$s_report" "invalid_free_count")"
	printf "double_free_count=%s\n" "$(proc_report_key_value "$s_report" "double_free_count")"
	printf "overflow_count=%s\n" "$(proc_report_key_value "$s_report" "overflow_count")"
	printf "underflow_count=%s\n" "$(proc_report_key_value "$s_report" "underflow_count")"
}

proc_run_http_case() {
	s_run_dir=$(proc_prepare_run_dir "http")
	s_log="$s_run_dir/http.log"
	i_pid=$(proc_start_server "$s_run_dir" "xs_manage_test.json" "$s_log")

	if ! proc_wait_ready; then
		printf "FAIL http ready timeout\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi
	if ! proc_require_mem_debug_enabled "http"; then
		proc_stop_server "$i_pid" || true
		return 1
	fi

	i=1
	while [ "$i" -le "$HTTP_COUNT" ]; do
		i_status=$(proc_fetch_status_method "GET" "http://127.0.0.1:$PORT/json?run=$RUN_TAG&seq=$i" || true)
		s_body=$(proc_fetch_body 2>/dev/null || true)
		if [ "$i_status" != "200" ] || ! printf "%s" "$s_body" | grep -F '"path":"/json"' >/dev/null 2>&1; then
			printf "FAIL http seq=%s\n" "$i" >&2
			proc_stop_server "$i_pid" || true
			return 1
		fi
		i=$((i + 1))
	done

	s_metric=$(proc_fetch_json_text "http://127.0.0.1:$PORT/__xs/http_metrics_json" || true)
	i_app_count=$(proc_json_get_int "$s_metric" "http_app_req_count")
	if [ -z "$i_app_count" ] || [ "$i_app_count" -ne "$HTTP_COUNT" ]; then
		printf "FAIL http_app_req_count expected=%s got=%s\n" "$HTTP_COUNT" "${i_app_count:-missing}" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi

	proc_stop_server "$i_pid"
	s_report="$s_run_dir/xrt_mem_report_auto.json"
	if ! proc_wait_file "$s_report"; then
		printf "FAIL http mem report missing\n" >&2
		return 1
	fi
	proc_assert_report_non_regression "http" "$s_report"
	proc_print_case_summary "http" "xs_manage_test.json" "$HTTP_COUNT" "$s_log" "$s_report"
}

proc_run_ws_case() {
	s_run_dir=$(proc_prepare_run_dir "ws")
	s_log="$s_run_dir/ws.log"

	if [ -z "$WS_CLIENT_EXE" ]; then
		WS_CLIENT_EXE=$(proc_build_c_client "ws_smoke_client.c" "ws_smoke_client")
	fi

	i_pid=$(proc_start_server "$s_run_dir" "xs_manage_ws_test.json" "$s_log")
	if ! proc_wait_ready; then
		printf "FAIL ws ready timeout\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi
	if ! proc_require_mem_debug_enabled "ws"; then
		proc_stop_server "$i_pid" || true
		return 1
	fi

	i=1
	while [ "$i" -le "$WS_COUNT" ]; do
		s_text="memdebug-ws-$i"
		if ! s_out=$(proc_run_client_retry "$WS_CLIENT_EXE" 10 "127.0.0.1" "$WS_PORT" "$s_text"); then
			printf "FAIL ws client seq=%s\n" "$i" >&2
			proc_stop_server "$i_pid" || true
			return 1
		fi
		if ! printf "%s" "$s_out" | grep -F "ws demo" >/dev/null 2>&1 || \
			! printf "%s" "$s_out" | grep -F "text=$s_text" >/dev/null 2>&1 || \
			! printf "%s" "$s_out" | grep -F "protocol=xs-demo" >/dev/null 2>&1; then
			printf "FAIL ws body seq=%s\n" "$i" >&2
			proc_stop_server "$i_pid" || true
			return 1
		fi
		i=$((i + 1))
	done

	s_metric=$(proc_fetch_json_text "http://127.0.0.1:$PORT/__xs/ws_metrics_json" || true)
	i_text=$(proc_json_get_int "$s_metric" "ws_text_count")
	i_conn=$(proc_json_get_int "$s_metric" "ws_conn_current")
	if [ -z "$i_text" ] || [ "$i_text" -lt "$WS_COUNT" ] || [ -z "$i_conn" ] || [ "$i_conn" -ne 0 ]; then
		printf "FAIL ws metrics mismatch\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi

	proc_stop_server "$i_pid"
	s_report="$s_run_dir/xrt_mem_report_auto.json"
	if ! proc_wait_file "$s_report"; then
		printf "FAIL ws mem report missing\n" >&2
		return 1
	fi
	proc_assert_report_non_regression "ws" "$s_report"
	proc_print_case_summary "ws" "xs_manage_ws_test.json" "$WS_COUNT" "$s_log" "$s_report"
}

proc_run_xtp_case() {
	s_run_dir=$(proc_prepare_run_dir "xtp")
	s_log="$s_run_dir/xtp.log"

	if [ -z "$XTP_CLIENT_EXE" ]; then
		XTP_CLIENT_EXE=$(proc_build_c_client "xtp_pressure_client.c" "xtp_pressure_client")
	fi

	i_pid=$(proc_start_server "$s_run_dir" "xs_manage_xtp_test.json" "$s_log")
	if ! proc_wait_ready; then
		printf "FAIL xtp ready timeout\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi
	if ! proc_require_mem_debug_enabled "xtp"; then
		proc_stop_server "$i_pid" || true
		return 1
	fi

	if ! s_out=$(proc_run_client_retry "$XTP_CLIENT_EXE" 10 "127.0.0.1" "$XTP_PORT" "$XTP_COUNT" "demo.ping" "tag=memdebug-xtp-$RUN_TAG"); then
		printf "FAIL xtp client\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi
	if ! printf "%s" "$s_out" | grep -F "ok_count=$XTP_COUNT" >/dev/null 2>&1 || \
		! printf "%s" "$s_out" | grep -F "last_status=0" >/dev/null 2>&1 || \
		! printf "%s" "$s_out" | grep -F "last_cmd=xtp.reply" >/dev/null 2>&1; then
		printf "FAIL xtp output\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi

	s_metric=$(proc_fetch_json_text "http://127.0.0.1:$PORT/__xs/xtp_metrics_json" || true)
	i_req=$(proc_json_get_int "$s_metric" "xtp_req_count")
	i_conn=$(proc_json_get_int "$s_metric" "xtp_conn_current")
	if [ -z "$i_req" ] || [ "$i_req" -lt "$XTP_COUNT" ] || [ -z "$i_conn" ] || [ "$i_conn" -ne 0 ]; then
		printf "FAIL xtp metrics mismatch\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi

	proc_stop_server "$i_pid"
	s_report="$s_run_dir/xrt_mem_report_auto.json"
	if ! proc_wait_file "$s_report"; then
		printf "FAIL xtp mem report missing\n" >&2
		return 1
	fi
	proc_assert_report_non_regression "xtp" "$s_report"
	proc_print_case_summary "xtp" "xs_manage_xtp_test.json" "$XTP_COUNT" "$s_log" "$s_report"
}

proc_run_custom_case() {
	s_run_dir=$(proc_prepare_run_dir "custom")
	s_log="$s_run_dir/custom.log"

	if [ -z "$CUSTOM_CLIENT_EXE" ]; then
		CUSTOM_CLIENT_EXE=$(proc_build_c_client "custom_smoke_client.c" "custom_smoke_client")
	fi

	i_pid=$(proc_start_server "$s_run_dir" "xs_manage_custom_test.json" "$s_log")
	if ! proc_wait_ready; then
		printf "FAIL custom ready timeout\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi
	if ! proc_require_mem_debug_enabled "custom"; then
		proc_stop_server "$i_pid" || true
		return 1
	fi

	i=1
	while [ "$i" -le "$CUSTOM_COUNT" ]; do
		s_text="memdebug-custom-$i"
		if ! s_out=$(proc_run_client_retry "$CUSTOM_CLIENT_EXE" 10 "127.0.0.1" "$CUSTOM_PORT" "$s_text"); then
			printf "FAIL custom client seq=%s\n" "$i" >&2
			proc_stop_server "$i_pid" || true
			return 1
		fi
		if ! printf "%s" "$s_out" | grep -F "custom demo" >/dev/null 2>&1 || \
			! printf "%s" "$s_out" | grep -F "data=$s_text" >/dev/null 2>&1; then
			printf "FAIL custom body seq=%s\n" "$i" >&2
			proc_stop_server "$i_pid" || true
			return 1
		fi
		i=$((i + 1))
	done

	s_metric=$(proc_fetch_json_text "http://127.0.0.1:$PORT/__xs/custom_metrics_json" || true)
	i_recv=$(proc_json_get_int "$s_metric" "custom_recv_count")
	i_conn=$(proc_json_get_int "$s_metric" "custom_conn_current")
	if [ -z "$i_recv" ] || [ "$i_recv" -lt "$CUSTOM_COUNT" ] || [ -z "$i_conn" ] || [ "$i_conn" -ne 0 ]; then
		printf "FAIL custom metrics mismatch\n" >&2
		proc_stop_server "$i_pid" || true
		return 1
	fi

	proc_stop_server "$i_pid"
	s_report="$s_run_dir/xrt_mem_report_auto.json"
	if ! proc_wait_file "$s_report"; then
		printf "FAIL custom mem report missing\n" >&2
		return 1
	fi
	proc_assert_report_non_regression "custom" "$s_report"
	proc_print_case_summary "custom" "xs_manage_custom_test.json" "$CUSTOM_COUNT" "$s_log" "$s_report"
}

proc_check_cleanup() {
	s_root_hash_before="$1"
	s_release_hash_before="$2"

	if pgrep -x "$XSDBG_BIN" >/dev/null 2>&1; then
		printf "FAIL cleanup_processes remained\n" >&2
		return 1
	fi

	if find "$TOOL_DIR" -maxdepth 1 \( -name 'ws_smoke_client.memdebug.*' -o -name 'xtp_pressure_client.memdebug.*' -o -name 'custom_smoke_client.memdebug.*' \) | grep . >/dev/null 2>&1; then
		printf "FAIL generated helpers remained\n" >&2
		return 1
	fi

	if [ "$(proc_get_hash_text "$REPO_ROOT/xrt_mem_report_auto.json")" != "$s_root_hash_before" ]; then
		printf "FAIL tracked root mem report changed\n" >&2
		return 1
	fi
	if [ "$(proc_get_hash_text "$RELEASE_DIR/xrt_mem_report_auto.json")" != "$s_release_hash_before" ]; then
		printf "FAIL tracked release mem report changed\n" >&2
		return 1
	fi

	return 0
}

trap 'proc_cleanup_servers; proc_cleanup_generated_files' EXIT

ROOT_HASH_BEFORE=$(proc_get_hash_text "$REPO_ROOT/xrt_mem_report_auto.json")
RELEASE_HASH_BEFORE=$(proc_get_hash_text "$RELEASE_DIR/xrt_mem_report_auto.json")
rm -rf "$TEMP_ROOT"
mkdir -p "$TEMP_ROOT"

proc_cleanup_generated_files
proc_cleanup_servers
proc_run_http_case
proc_run_ws_case
proc_run_xtp_case
proc_run_custom_case
proc_cleanup_servers
proc_cleanup_generated_files
proc_check_cleanup "$ROOT_HASH_BEFORE" "$RELEASE_HASH_BEFORE"
printf "[status]\n"
printf "result=ok\n"
