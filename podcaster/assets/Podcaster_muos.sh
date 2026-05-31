#!/bin/sh

# HELP: Tiny Podcaster
# ICON: flip
# GRID: Tiny Podcaster

. /opt/muos/script/var/func.sh

APP_BIN="podcaster_client"
SETUP_APP "$APP_BIN" ""

# -----------------------------------------------------------------------------
# You do everything else within this area here for your application

PODCASTER_DIR="/run/muos/storage/application/Podcaster"
DB_DIR="/run/muos/storage/save/podcaster"

cd "$PODCASTER_DIR" || exit

if pgrep "podcasterd" > /dev/null; then
    echo "Already running"
else
    nohup ./bin/podcasterd $DB_DIR &> server.log &
fi

./bin/podcaster_client &> client.log