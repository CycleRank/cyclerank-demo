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

echo "=== Timeout example"
echo "This example issues two queries with a timeout (managed client side)."
echo "The first is supposed to fail, the second to succeed"
echo ""

function query {
  echo ""
  echo ""

  f=$1
  s=$2
  k=$3
  t=$4


  qid=$(curl -s \
    --data "file=$f&source=$s&maxloop=$k" \
    "$BASE_URL/v0/start" \
    | jq '.id')
  echo "Query id: $qid"

  for _ in $(seq 1 "$t"); do
    # Wait
    sleep 1

    # Poll
    echo -n "Running query pid: "
    curl -s \
      "$BASE_URL/v0/status" \
      | jq -e ".${qid}.Pid" || (output "$qid" && false) || return 0
  done

  echo -n "[Timeout ${t}s] killing the query: "
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
    | jq ".$qid.Pid"

  echo -n "Via pgrep: "
  pgrep -x LoopRank
  echo ""
}

function output {
  qid=$1
  echo "Query $qid output:"
  curl -s \
    "$BASE_URL/v0/output?id=$qid" \
    | jq .
}

# f s k t
query "wikipedia/en/2018-03-01.csv" 9167 6 5 || true
query "wikipedia/en/2018-03-01.csv" 971683 3 90

exit 0
