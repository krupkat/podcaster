#!/bin/sh
# HELP: Tiny Podcaster
# ICON: flip

. /opt/muos/script/var/func.sh

echo app >/tmp/act_go

PODCASTER_DIR="/mnt/mmc/MUOS/application/.podcaster"
DB_DIR="/mnt/sdcard/MUOS/save/podcaster"

cd "$PODCASTER_DIR" || exit

if pgrep "podcaster" > /dev/null; then
    echo "Already running"
else
    nohup ./podcaster $DB_DIR &> server.log &
fi

SET_VAR "system" "foreground_process" "hello_muos"

./hello_muos &> client.log