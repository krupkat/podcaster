#!/bin/sh

APP_DIR="$(dirname "$0")/.podcaster"
DB_DIR="$APP_DIR/state"

cd "$APP_DIR"

if pgrep "podcasterd" > /dev/null; then
    echo "Already running"
else
    nohup ./podcasterd $DB_DIR &> server.log &
fi

./podcaster_client &> client.log