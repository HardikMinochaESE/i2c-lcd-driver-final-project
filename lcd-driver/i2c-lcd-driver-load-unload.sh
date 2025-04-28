#!/bin/sh
#
# i2c-lcd-driver start-stop-daemon script
#

case "$1" in
    start)
        echo "Loading i2c-lcd-driver module"
        /sbin/modprobe i2c-lcd-driver
        if [ $? -ne 0 ]; then
            echo "Error: Failed to load i2c-lcd-driver module"
            exit 1
        fi
        start-stop-daemon -S -n thermal_lcd_bridge -a /etc/init.d/thermal_lcd_bridge.sh 
        ;;
    stop)
        echo "Unloading i2c-lcd-driver module"
        /sbin/rmmod i2c-lcd-driver
        start-stop-daemon -K -n thermal_lcd_bridge
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