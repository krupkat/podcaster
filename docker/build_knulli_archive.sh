#!/usr/bin/env bash

set -e

VERSION=$(git describe --tags --always --dirty)
IMAGE_TAG="tiny-podcaster:latest"
TMP_DIR=$(mktemp -d)

# Build the image and copy the files out
docker build -t $IMAGE_TAG -f docker/Dockerfile --build-arg CFW=knulli .
DOCKER_ID=$(docker create $IMAGE_TAG)
docker cp $DOCKER_ID:/install/. $TMP_DIR
docker rm $DOCKER_ID

( cd $TMP_DIR && zip -r export.zip . )

mkdir -p export
mv $TMP_DIR/export.zip export/tiny-podcaster-knulli-$VERSION.zip
rm -rf $TMP_DIR