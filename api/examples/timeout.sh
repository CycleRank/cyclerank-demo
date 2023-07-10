#!/bin/bash

set -euo pipefail
IFS=$'\t\n'

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
    "http://127.0.0.1:8080/v0/start" \
    | jq '.id')
  echo "Query id: $qid"

  for _ in $(seq 1 "$t"); do
    # Wait
    sleep 1

    # Poll
    echo -n "Running query pid: "
    curl -s \
      "http://127.0.0.1:8080/v0/status" \
      | jq -e ".${qid}.Pid" || (output "$qid" && false) || return 0
  done

  echo -n "[Timeout ${t}s] killing the query: "
  curl -s --data "id=$qid" \
    "http://127.0.0.1:8080/v0/kill" \
    -w '%{http_code}'
  echo ""

  echo ""
  echo "Waiting 2 seconds before checking for query kill:"
  sleep 2

  set +e
  echo -n "At Status page: "
  curl -s \
    "http://127.0.0.1:8080/v0/status" \
    | jq ".$qid.Pid"

  echo -n "Via pgrep: "
  pgrep -x LoopRank
  echo ""
}

function output {
  qid=$1
  echo "Query $qid output:"
  curl -s \
    "http://127.0.0.1:8080/v0/output?id=$qid" \
    | jq .
}

# f s k t
query "toy/cliques/clique-256.txt" 0 4 5 || true
query "toy/cliques/clique-128.txt" 0 4 10

