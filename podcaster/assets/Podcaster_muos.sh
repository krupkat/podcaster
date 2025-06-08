#!/bin/sh
# HELP: Tiny Podcaster
# ICON: flip
# GRID: Tiny Podcaster

. /opt/muos/script/var/func.sh

echo app >/tmp/act_go

PODCASTER_DIR="$(GET_VAR "device" "storage/rom/mount")/MUOS/application/Podcaster"
DB_DIR="/run/muos/storage/save/podcaster"

cd "$PODCASTER_DIR" || exit

if pgrep "podcasterd" > /dev/null; then
    echo "Already running"
else
    nohup ./podcasterd $DB_DIR &> server.log &
fi

SET_VAR "system" "foreground_process" "podcaster_client"

./podcaster_client &> client.log
