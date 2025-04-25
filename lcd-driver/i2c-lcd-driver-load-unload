#!/bin/sh
#
# i2c-lcd-driver start-stop-daemon script
#

case "$1" in
    start)
        echo "Loading i2c-lcd-driver module"
        /sbin/modprobe i2c-lcd-driver
        ;;
    stop)
        echo "Unloading i2c-lcd-driver module"
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