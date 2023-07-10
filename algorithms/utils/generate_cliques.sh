#!/usr/bin/env bash

# declare an array called array and define 3 vales
cliques=( 3 4 5 6 8 10 12 16 32 64 128 256 1024 2048 4096 8192 16384 \
		  32768 )
for i in "${cliques[@]}"; do
	echo -n "Generating ${i}-clique..."
	./clique_generator.py "${i}" -s 0 -l 5 -o "../clique5-${i}.txt"
	echo ' done!'
done
