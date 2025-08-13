#!/usr/bin/env sh

cd "$(dirname "$0")" || exit

if pgrep "podcasterd" > /dev/null; then
    echo "podcasterd already running"
else
    DB_DIR=$HOME/.local/share/krupkat/podcaster
    mkdir -p "$DB_DIR"
    nohup ./usr/bin/podcasterd "$DB_DIR" &
fi

./usr/bin/podcaster_client
