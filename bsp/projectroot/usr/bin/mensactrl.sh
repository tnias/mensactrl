#!/bin/sh

/bin/echo 91 > /sys/class/gpio/export
/bin/echo low > /sys/class/gpio/gpio91/direction
exec /usr/bin/mensactrl /dev/fb0
