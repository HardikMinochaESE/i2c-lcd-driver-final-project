#!/bin/sh
#
# Thermal LCD Bridge daemon start-stop script
#

case "$1" in
    start)
        echo "Starting thermal lcd bridge daemon"
        start-stop-daemon -S -n thermal_lcd_bridge_daemon -a /etc/init.d/thermal_lcd_bridge_daemon.sh
        ;;
    stop)
        echo "Stopping thermal lcd bridge daemon"
        start-stop-daemon -K -n thermal_lcd_bridge_daemon
        ;;
    restart)
        $0 stop
        $0 start
        ;;
    *)
        echo "Usage $0 {start|stop}"
        exit 1
        ;;
esac

exit 0 