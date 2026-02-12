#!/bin/sh
# fake pkg config for android since you cant link to system dependencies on it
if [ "$1" = "--version" ]; then
    echo "0.29.2"
elif [ "$1" = "--cflags" ]; then
    echo ""
elif [ "$1" = "--libs" ]; then
    echo ""
elif [ "$1" = "--modversion" ]; then
    echo "3.0.0"
fi
exit 0
