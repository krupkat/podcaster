#!/usr/bin/env bash

VERSION=$(git describe --tags --always --dirty)
IMAGE_TAG="tiny-podcaster:latest"
TMP_DIR=$(mktemp -d)

# Build the image and copy the files out
docker build -t $IMAGE_TAG -f docker/Dockerfile .
DOCKER_ID=$(docker create $IMAGE_TAG)
docker cp $DOCKER_ID:/build/. $TMP_DIR
docker rm $DOCKER_ID

mkdir -p export
mv $TMP_DIR/podcaster_client-x86_64.AppImage export/tiny-podcaster-client-$VERSION.AppImage
mv $TMP_DIR/podcasterd-x86_64.AppImage export/tiny-podcaster-server-$VERSION.AppImage
rm -rf $TMP_DIR