#!/bin/bash
# shellcheck disable=SC2128
SOURCED=false && [ "$0" = "$BASH_SOURCE" ] || SOURCED=true

if ! $SOURCED; then
  set -euo pipefail
  IFS=$'\n\t'
fi

# echo "download_data: $download_data"
# echo "download_output: $download_output"

function flatten() {
  find "$1" -mindepth 2 -type f -exec mv --backup=numbered -t "$1" {} +
  find "$1" -mindepth 1 -type d -empty -delete
}

# create data directory
mkdir -pv data

# shellcheck disable=SC2154
if $download_data; then
  (
    cd data || ( ( echo 'Error. Can not access data directory' >&2 ) && exit 1 )
    wget --no-verbose https://cricca.disi.unitn.it/datasets/cyclerank/docker/data.tar.zst.sha512sum \
      && wget --no-verbose https://cricca.disi.unitn.it/datasets/cyclerank/docker/data.tar.zst \
      && sha512sum -c data.tar.zst.sha512sum \
      && rm -f data.tar.zst.sha512sum

    unzstd -v data.tar.zst \
        && tar -v -x -f data.tar \
        && rm -f data.tar.zst data.tar

    # make sure that we don't have any subfolder
    flatten .
  )
else
  ( echo "Skipping downloading datasets..." >&2 )
  sleep 1
fi

# create output directory
mkdir -pv output

# shellcheck disable=SC2154
if $download_output; then
  (
    cd output || ( ( echo 'Error. Can not access data directory'  >&2 ) && exit 1 )
    wget --no-verbose https://cricca.disi.unitn.it/datasets/cyclerank/docker/output.tar.zst.sha512sum \
      && wget --no-verbose https://cricca.disi.unitn.it/datasets/cyclerank/docker/output.tar.zst \
      && sha512sum -c output.tar.zst.sha512sum \
      && rm -f output.tar.zst.sha512sum

    unzstd -v output.tar.zst \
        && tar -v -x -f output.tar \
        && rm -f output.tar.zst output.tar

    # make sure that we don't have any subfolder
    flatten .
  )
else
  ( echo "Skipping downloading output data..." >&2 )
  sleep 1
fi

exit 0
