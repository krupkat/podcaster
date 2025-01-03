#!/usr/bin/env bash

set -e

VERSION=$(git describe --tags --always --dirty)
IMAGE_TAG="tiny-podcaster:latest"
TMP_DIR=$(mktemp -d)
APP_DIR=$TMP_DIR/mnt/mmc/MUOS/application
mkdir -p $APP_DIR

# Build the image and copy the files out
docker build -t $IMAGE_TAG -f docker/Dockerfile .
DOCKER_ID=$(docker create $IMAGE_TAG)
docker cp $DOCKER_ID:/install/. $APP_DIR
docker rm $DOCKER_ID

( cd $TMP_DIR && zip -r export.zip . )

mkdir -p export
mv $TMP_DIR/export.zip export/tiny-podcaster-$VERSION.zip
rm -rf $TMP_DIR