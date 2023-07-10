#!/bin/bash

# shellcheck disable=SC2128
SOURCED=false && [ "$0" = "$BASH_SOURCE" ] || SOURCED=true

if ! $SOURCED; then
  set -euo pipefail
  IFS=$'\n\t'
fi

usage() {
  (>&2 echo "Usage:")
  (>&2 echo "  basic.sh BASE_URL")
}

if [ "$#" -ne 1 ]; then
    echo "Error. Parameter BASE_URL required."
    usage
    exit 1
fi

BASE_URL="$1"

echo "=== Advanced example"
echo "This example issues a long running query,"
echo "uses polling to check its progress status for 5 seconds,"
echo "and then kills it."
echo ""

qid=$(curl -s \
  --data "file=wikipedia/en/2018-03-01.csv&source=9167&maxloop=6" \
  "$BASE_URL/v0/start" \
  | jq '.id')

echo "Query id: $qid"

for _ in {1..5}; do
  # Wait
  sleep 1

  # Poll
  echo -n "Running query pid: "
  curl -s \
    "$BASE_URL/v0/status" \
    | jq ".${qid}.Pid"
done

echo -n "Killing the query: "
curl -s --data "id=$qid" \
  "$BASE_URL/v0/kill" \
  -w '%{http_code}'
echo ""


echo ""
echo "Waiting 2 seconds before checking for query kill:"
sleep 2

set +e
echo -n "At Status page: "
curl -s \
  "$BASE_URL/v0/status" \
  | jq ".$qid"

exit 0
