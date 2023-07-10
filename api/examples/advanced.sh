#!/bin/bash

set -euo pipefail
IFS=$'\t\n'

echo "=== Advanced example"
echo "This example issues a long running query,"
echo "uses polling to check its progress status for 5 seconds,"
echo "and then kills it."
echo ""

qid=$(curl -s \
  --data "file=toy/cliques/clique-256.txt&source=0&maxloop=4" \
  "http://127.0.0.1:8080/v0/start" \
  | jq '.id')

echo "Query id: $qid"

for _ in {1..5}; do
  # Wait
  sleep 1

  # Poll
  echo -n "Running query pid: "
  curl -s \
    "http://127.0.0.1:8080/v0/status" \
    | jq ".${qid}.Pid"
done

echo -n "Killing the query: "
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
  | jq ".$qid"

echo -n "Via pgrep: "
pgrep -x LoopRank
echo ""
