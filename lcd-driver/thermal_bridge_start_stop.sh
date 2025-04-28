#!/bin/sh
#
# Thermal LCD Bridge daemon start-stop script
#

PID_FILE="/var/run/thermal_bridge_pid"
DAEMON_PATH="./thermal_lcd_bridge.sh"

case "$1" in
    start)
        echo "Starting thermal lcd bridge daemon"
        start-stop-daemon -S -p "$PID_FILE" -m -x "$DAEMON_PATH"
        ;;
    stop)
        echo "Stopping thermal lcd bridge daemon"
        if [ -f "$PID_FILE" ]; then
            PID=$(cat "$PID_FILE")
            kill "$PID"
            rm "$PID_FILE"
        fi
        ;;
    *)
        echo "Usage $0 {start|stop}"
        exit 1
        ;;
esac

exit 0 