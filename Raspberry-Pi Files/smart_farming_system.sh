#!/usr/bin/env bash
set -Eeuo pipefail

# Smart Farming Raspberry Pi system controller
#
# Starts and manages:
#   1. CC1101 receiver
#   2. Sensor-value conversion bridge
#   3. Nutrient HAT inference through predict_and_log.py
#   4. Flask dashboard
#   5. Optional Chromium browser window
#
# Usage:
#   ./smart_farming_system.sh
#   ./smart_farming_system.sh start
#   ./smart_farming_system.sh stop
#   ./smart_farming_system.sh restart
#   ./smart_farming_system.sh status
#   ./smart_farming_system.sh logs
#
# Optional:
#   OPEN_BROWSER=0 ./smart_farming_system.sh start

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RUNTIME_DIR="$SCRIPT_DIR/.runtime"
LOG_DIR="$SCRIPT_DIR/system_logs"

BRIDGE_PID_FILE="$RUNTIME_DIR/bridge.pid"
DASHBOARD_PID_FILE="$RUNTIME_DIR/dashboard.pid"

BRIDGE_LOG="$LOG_DIR/bridge.log"
DASHBOARD_LOG="$LOG_DIR/dashboard.log"
BROWSER_LOG="$LOG_DIR/browser.log"

PYTHON="$SCRIPT_DIR/.venv/bin/python"
BRIDGE_SCRIPT="$SCRIPT_DIR/start_radio_model.sh"
DASHBOARD_APP="$SCRIPT_DIR/dashboard_app.py"
PREDICTOR="$SCRIPT_DIR/predict_and_log.py"
MODEL="$SCRIPT_DIR/nutrient_stress_hat.pkl"

DASHBOARD_HOST="127.0.0.1"
DASHBOARD_PORT="5000"
DASHBOARD_URL="http://${DASHBOARD_HOST}:${DASHBOARD_PORT}"

mkdir -p "$RUNTIME_DIR" "$LOG_DIR"

print_header() {
    echo "======================================================"
    echo " Smart Farming AI System"
    echo " CC1101 + Nutrient HAT + Dashboard"
    echo "======================================================"
}

read_pid() {
    local pid_file="$1"

    if [[ -f "$pid_file" ]]; then
        tr -d '[:space:]' < "$pid_file"
    fi
}

pid_is_running() {
    local pid="${1:-}"

    [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null
}

remove_stale_pid_file() {
    local pid_file="$1"
    local pid

    pid="$(read_pid "$pid_file")"

    if [[ -n "$pid" ]] && ! pid_is_running "$pid"; then
        rm -f "$pid_file"
    fi
}

require_file() {
    local path="$1"
    local description="$2"

    if [[ ! -f "$path" ]]; then
        echo "ERROR: Missing $description:"
        echo "       $path"
        exit 1
    fi
}

validate_project() {
    if [[ ! -x "$PYTHON" ]]; then
        echo "ERROR: Python virtual environment was not found:"
        echo "       $PYTHON"
        echo
        echo "Create or restore the .venv folder first."
        exit 1
    fi

    require_file "$BRIDGE_SCRIPT" "radio/model bridge"
    require_file "$DASHBOARD_APP" "dashboard application"
    require_file "$PREDICTOR" "HAT inference program"
    require_file "$MODEL" "trained nutrient HAT model"
    require_file "$SCRIPT_DIR/RX_Demo_Sensors.cpp" "CC1101 receiver source"
    require_file "$SCRIPT_DIR/cc1100_raspi.cpp" "SpaceTeddy CC1101 driver"
    require_file "$SCRIPT_DIR/cc1100_raspi.h" "SpaceTeddy CC1101 header"

    chmod +x "$BRIDGE_SCRIPT"
}

start_bridge() {
    local current_pid

    remove_stale_pid_file "$BRIDGE_PID_FILE"
    current_pid="$(read_pid "$BRIDGE_PID_FILE")"

    if pid_is_running "$current_pid"; then
        echo "[OK] Radio/model bridge is already running (PID $current_pid)."
        return
    fi

    echo "[START] CC1101 receiver + conversion bridge + HAT inference"

    # start_radio_model.sh invokes sudo for the radio process.
    # sudo -v requests authorization once before launching in the background.
    sudo -v

    : > "$BRIDGE_LOG"

    nohup setsid "$BRIDGE_SCRIPT" \
        </dev/null \
        >>"$BRIDGE_LOG" \
        2>&1 &

    local bridge_pid=$!
    echo "$bridge_pid" > "$BRIDGE_PID_FILE"

    sleep 2

    if ! pid_is_running "$bridge_pid"; then
        echo "ERROR: The radio/model bridge stopped during startup."
        echo "Last bridge log lines:"
        tail -n 30 "$BRIDGE_LOG" || true
        rm -f "$BRIDGE_PID_FILE"
        exit 1
    fi

    echo "[OK] Radio/model bridge running (PID $bridge_pid)."
}

start_dashboard() {
    local current_pid

    remove_stale_pid_file "$DASHBOARD_PID_FILE"
    current_pid="$(read_pid "$DASHBOARD_PID_FILE")"

    if pid_is_running "$current_pid"; then
        echo "[OK] Dashboard is already running (PID $current_pid)."
        return
    fi

    echo "[START] Flask dashboard"
    : > "$DASHBOARD_LOG"

    nohup setsid "$PYTHON" "$DASHBOARD_APP" \
        </dev/null \
        >>"$DASHBOARD_LOG" \
        2>&1 &

    local dashboard_pid=$!
    echo "$dashboard_pid" > "$DASHBOARD_PID_FILE"

    sleep 1

    if ! pid_is_running "$dashboard_pid"; then
        echo "ERROR: The dashboard stopped during startup."
        echo "Last dashboard log lines:"
        tail -n 30 "$DASHBOARD_LOG" || true
        rm -f "$DASHBOARD_PID_FILE"
        stop_bridge
        exit 1
    fi

    echo "[OK] Dashboard running (PID $dashboard_pid)."
}

wait_for_dashboard() {
    echo "[CHECK] Waiting for dashboard at $DASHBOARD_URL"

    for _ in $(seq 1 30); do
        if "$PYTHON" - "$DASHBOARD_URL" >/dev/null 2>&1 <<'PY'
import sys
import urllib.request

with urllib.request.urlopen(sys.argv[1], timeout=1) as response:
    if response.status >= 400:
        raise RuntimeError(response.status)
PY
        then
            echo "[OK] Dashboard is responding."
            return
        fi

        sleep 1
    done

    echo "WARNING: Dashboard process is running but the webpage did not respond."
    echo "Check:"
    echo "  $DASHBOARD_LOG"
}

get_lan_ip() {
    hostname -I 2>/dev/null | awk '{print $1}'
}

open_browser() {
    if [[ "${OPEN_BROWSER:-1}" == "0" ]]; then
        return
    fi

    if [[ -z "${DISPLAY:-}" ]] && [[ -z "${WAYLAND_DISPLAY:-}" ]]; then
        echo "[INFO] No desktop display detected; browser was not opened."
        return
    fi

    echo "[START] Opening dashboard browser"

    if command -v chromium >/dev/null 2>&1; then
        nohup chromium \
            --new-window \
            --start-fullscreen \
            "$DASHBOARD_URL" \
            </dev/null \
            >>"$BROWSER_LOG" \
            2>&1 &
        return
    fi

    if command -v chromium-browser >/dev/null 2>&1; then
        nohup chromium-browser \
            --new-window \
            --start-fullscreen \
            "$DASHBOARD_URL" \
            </dev/null \
            >>"$BROWSER_LOG" \
            2>&1 &
        return
    fi

    if command -v xdg-open >/dev/null 2>&1; then
        nohup xdg-open "$DASHBOARD_URL" \
            </dev/null \
            >>"$BROWSER_LOG" \
            2>&1 &
        return
    fi

    echo "[INFO] No supported browser command was found."
}

stop_process_group() {
    local pid_file="$1"
    local name="$2"
    local use_sudo="${3:-0}"
    local pid

    pid="$(read_pid "$pid_file")"

    if ! pid_is_running "$pid"; then
        echo "[OK] $name is not running."
        rm -f "$pid_file"
        return
    fi

    echo "[STOP] $name (PID $pid)"

    # Each component is launched with setsid, so its PID is also the process
    # group ID. Killing the negative PID stops the complete pipeline.
    if [[ "$use_sudo" == "1" ]]; then
        sudo kill -TERM -- "-$pid" 2>/dev/null || true
    else
        kill -TERM -- "-$pid" 2>/dev/null || true
    fi

    for _ in $(seq 1 10); do
        if ! pid_is_running "$pid"; then
            rm -f "$pid_file"
            echo "[OK] $name stopped."
            return
        fi
        sleep 0.5
    done

    echo "[STOP] Forcing $name to stop."

    if [[ "$use_sudo" == "1" ]]; then
        sudo kill -KILL -- "-$pid" 2>/dev/null || true
    else
        kill -KILL -- "-$pid" 2>/dev/null || true
    fi

    rm -f "$pid_file"
}

stop_bridge() {
    stop_process_group "$BRIDGE_PID_FILE" "Radio/model bridge" 1
}

stop_dashboard() {
    stop_process_group "$DASHBOARD_PID_FILE" "Dashboard" 0
}

start_system() {
    print_header
    validate_project

    start_bridge
    start_dashboard
    wait_for_dashboard
    open_browser

    local lan_ip
    lan_ip="$(get_lan_ip)"

    echo
    echo "SYSTEM READY"
    echo
    echo "Raspberry Pi:"
    echo "  $DASHBOARD_URL"

    if [[ -n "$lan_ip" ]]; then
        echo
        echo "Another computer on the same network:"
        echo "  http://${lan_ip}:${DASHBOARD_PORT}"
    fi

    echo
    echo "Live predictions:"
    echo "  prediction_logs/hat_predictions.csv"
    echo
    echo "Raw and converted radio values:"
    echo "  prediction_logs/raw_and_converted_cc1101_packets.jsonl"
    echo
    echo "Commands:"
    echo "  ./smart_farming_system.sh status"
    echo "  ./smart_farming_system.sh logs"
    echo "  ./smart_farming_system.sh stop"
}

stop_system() {
    print_header
    stop_dashboard
    stop_bridge
    echo
    echo "System stopped."
}

show_status() {
    print_header

    local bridge_pid dashboard_pid
    bridge_pid="$(read_pid "$BRIDGE_PID_FILE")"
    dashboard_pid="$(read_pid "$DASHBOARD_PID_FILE")"

    if pid_is_running "$bridge_pid"; then
        echo "[RUNNING] Radio/model bridge (PID $bridge_pid)"
    else
        echo "[STOPPED] Radio/model bridge"
    fi

    if pid_is_running "$dashboard_pid"; then
        echo "[RUNNING] Dashboard (PID $dashboard_pid)"
    else
        echo "[STOPPED] Dashboard"
    fi

    echo
    echo "Dashboard URL: $DASHBOARD_URL"
}

show_logs() {
    echo "Press Ctrl+C to stop viewing logs."
    echo

    touch "$BRIDGE_LOG" "$DASHBOARD_LOG"

    tail -n 50 -F \
        "$BRIDGE_LOG" \
        "$DASHBOARD_LOG"
}

restart_system() {
    stop_system
    echo
    start_system
}

show_help() {
    cat <<'HELP'
Usage:
  ./smart_farming_system.sh [command]

Commands:
  start     Start CC1101, bridge, HAT inference, dashboard, and browser
  stop      Stop the dashboard and complete radio/model pipeline
  restart   Stop and start the complete system
  status    Show which components are running
  logs      Follow bridge and dashboard logs
  help      Show this help

The default command is start.

Optional:
  OPEN_BROWSER=0 ./smart_farming_system.sh start
HELP
}

command="${1:-start}"

case "$command" in
    start)
        start_system
        ;;
    stop)
        stop_system
        ;;
    restart)
        restart_system
        ;;
    status)
        show_status
        ;;
    logs)
        show_logs
        ;;
    help|-h|--help)
        show_help
        ;;
    *)
        echo "Unknown command: $command"
        echo
        show_help
        exit 1
        ;;
esac
