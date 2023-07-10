#!/usr/bin/env python3

import sys
import csv
import argparse
import pathlib
import itertools


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate a clique graph of '
                                                 'the given dimension.')
    parser.add_argument('file',
                        metavar='<file>',
                        type=pathlib.Path,
                        help='input file name.')

    parser.add_argument('-l', '--lenght',
                        type=int,
                        required=True,
                        help='max lenght of the loop [default: K].')

    parser.add_argument('--snapshot',
                        type=pathlib.Path,
                        required=True,
                        help='max lenght of the loop [default: K].')

    parser.add_argument('-o', '--output',
                        type=pathlib.Path,
                        help='output file name [default: stdout].')

    parser.add_argument('-s', '--start',
                        type=int,
                        default=0,
                        help='starting node for pageloop algorithm [default: 0].')

    parser.add_argument('--no-params',
                        dest='print_params',
                        action='store_false',
                        help='Do not print parameters, only number of nodes '
                             'and edges.')

    args = parser.parse_args()

    # import ipdb; ipdb.set_trace()

    file = args.file
    snapshot = args.snapshot
    lenght = args.lenght
    output = args.output
    start = args.start

    outfile = None
    if output is None:
        outfile = sys.stdout
    else:
        outfile = output.open('w+')

    with file.open('r') as infile:
        reader = csv.reader(infile, delimiter='\t')
        edges = [(int(l[0]), int(l[1]))
                for l in reader]
        numedges = len(edges)

    with snapshot.open('r') as snapshotfile:
        reader = csv.reader(snapshotfile, delimiter='\t')
        maxnodes = max([int(l[0]) for l in reader])

    # assertion about input
    assert isinstance(maxnodes, int)
    assert isinstance(lenght, int)
    assert isinstance(start, int)
    assert hasattr(outfile, 'write')    # check that outfile has write method

    numnodes = maxnodes + 1

    if args.print_params:
        outfile.write("{nodes} {edges} {start} {lenght}\n"
              .format(nodes=numnodes, edges=numedges, start=start, lenght=lenght))
    else:
        outfile.write("{nodes} {edges}\n"
              .format(nodes=numnodes, edges=numedges))

    for e1, e2 in edges:
        outfile.write("{} {}\n".format(e1, e2))

    exit(0)
