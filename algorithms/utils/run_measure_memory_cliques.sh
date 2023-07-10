#!/usr/bin/env bash

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

arr=( '06' '07' '08' '09' '10' '11' '12' '16' '32' '64' '70' '72' '78' )
for idx in ${arr[@]}; do
  afile=$(realpath "$script_dir/../data/toy/cliques5/clique5-${idx}.txt")
  abase="$(basename "$afile")"

  echo "Running with input $abase"
  aname="${abase%.*}"

  ./pageloop_back_map -f "$afile" &> /dev/null &
  plpid="$!"
  psrecord "$plpid" \
          --interval 1 \
          --include-children \
          --log "${aname}.psrecord.log" \
            2>/dev/null

done

exit 0

