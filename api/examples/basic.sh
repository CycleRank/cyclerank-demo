#!/bin/bash

set -euo pipefail
IFS=$'\t\n'

#################### helpers
function check_file() {
  local myfile="$1"

  if [[ ! -e "$myfile" ]]; then
    (>&2 echo "$myfile is not a valid file.")
    bkplog "Error: $myfile is not a valid file."
    exit 1
  fi

}
####################

################### help
function short_usage() {
  (>&2 echo \
"Usage:
  $(basename "$0") [options] <input>")
}

function usage() {
  (>&2 short_usage )
  (>&2 echo \
"

Test CycleRank API.

Arguments:
  input           Username of the new user to add.

Options:
  -b BASE_URL     Base URL in the form http://<ip>:<port> of the API.
  -h              Show this help and exits.

Example:
  # Make a request with file 'toy/toy0.in.txt'
  $(basename "$0") toy/toy0.in.txt
")
}

help_flag=false
base_url='http://127.0.0.1:8080'
while getopts ":b:h" opt; do
  case $opt in
    b)
      base_url=$OPTARG
      ;;

    h)
      help_flag=true
      ;;
    \?)
      (>&2 echo "Error. Invalid option: -$OPTARG")
      exit 1
      ;;
    :)
      (>&2 echo "Error.Option -$OPTARG requires an argument.")
      exit 1
      ;;
  esac
done

if $help_flag; then
  usage
  exit 0
fi
#################### end: help

NUMARGS=1

# mixing getopts with positional parameters possible?
# https://stackoverflow.com/q/11742996/2377454
numopt="$#"

# parameter not specified
if (( numopt-OPTIND < NUMARGS-1 )) ; then
  (>&2 echo "Error: parameter <input> is required.")
  short_usage
  exit 1
fi

# unrecognized extra parameters not specified
if (( numopt-OPTIND > NUMARGS-1 )) ; then
  (>&2 echo "Error: unrecognized extra parameters specified.")
  short_usage
  exit 1
fi

input="${*:$OPTIND:1}"

echo "=== Basic example"
echo "This example computes a trivial query"
echo "and retrieves the resulting ouput without polling"
echo ""

qid=$(curl -s \
           --data "file=${input}&source=0&maxloop=3&debug=1" \
             "${base_url}/v0/start" \
  | jq '.id')

echo "Query id: $qid"

echo "Result output:"
curl -s "${base_url}/v0/output?id=$qid" | jq .

exit 0
