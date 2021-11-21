#!/bin/bash
DST_ADDR=${1:-192.168.14.50}
DST_PORT=${2:-2021}

echo "Usage: $0 [IP addr] [port]"
echo "Starting UDP console ${DST_ADDR} ${DST_PORT}. Press enter"
saved_tty_settings=$(stty -g)

trap "stty $ saved_tty_settings" EXIT

stty -icanon -echo -icrnl && nc -u 192.168.14.2 55151

