#!/bin/sh
#
# i2c-lcd-driver start-stop-daemon script
#

PID_FILE="/var/run/thermal_lcd_bridge.pid"


case "$1" in
    start)
        echo "Loading i2c-lcd-driver module"
        /sbin/modprobe i2c-lcd-driver
        if [ $? -ne 0 ]; then
            echo "Error: Failed to load i2c-lcd-driver module"
            exit 1
        fi
        start-stop-daemon -S --exec /etc/init.d/thermal_lcd_bridge.sh --start --pidfile $PID_FILE
        ;;
    stop)
        echo "Unloading i2c-lcd-driver module"
        if [ -f $PID_FILE ]; then
            PID=$(cat $PID_FILE)
            kill $PID
            rm $PID_FILE
        fi
        /sbin/rmmod i2c-lcd-driver
        ;;
    restart)
        $0 stop
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac

exit 0 