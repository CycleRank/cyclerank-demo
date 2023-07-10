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

echo "=== Basic example"
echo "This example computes a trivial query"
echo "and retrieves the resulting ouput without polling"
echo ""

qid=$(curl -s \
  --data "file=wikipedia/en/2005-03-01.csv&source=2561&maxloop=3&verbose=1" \
  "$BASE_URL/v0/start" \
  | jq '.id')

echo "Query id: $qid"

echo "Wait 10 seconds before polling the output"
sleep 10

echo "Result output:"
curl -s \
  "$BASE_URL/v0/output?id=$qid" \
  | jq .

exit 0
