#!/usr/bin/env python3

import sys
import csv
import re
import argparse
import pathlib
import itertools

ALLOWED_FIELDS = set(['source_id',
                      'source_title',
                      'target_id',
                      'target_title'
                      ])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Filter Wikipedia networks from a list of ids.')

    parser.add_argument('GRAPH',
                        type=pathlib.Path,
                        help='Graph file.')
    parser.add_argument('-d', '--delimiter',
                        type=str,
                        default='\t',
                        help="Graph file delimiter [default: '\t'].")
    parser.add_argument('--skip-header',
                        action='store_true',
                        help='Skip graph file header.')

    parser.add_argument('-o', '--output',
                        type=pathlib.Path,
                        help='Output file name [default: stdout].')
    parser.add_argument('--print',
                        type=str,
                        nargs='+',
                        help='Output format.')
    parser.add_argument('--output-delimiter',
                        type=str,
                        default='\t',
                        help="Output file delimiter [default: '\t'].")

    parser.add_argument('--match-titles',
                        action='store_true',
                        help='Match page titles instead of ids.')

    parser.add_argument('-k', '--maxloop',
                        type=int,
                        dest='K',
                        help='Limit cycles to this length (K) '
                             '[default: No limit].')

    parser.add_argument('-f', '--filter',
                        type=pathlib.Path,
                        required=True,
                        help='Filter file (with ids, pageloop cycles or page '
                             'titles).')
    parser.add_argument('--filter-delimiter',
                        type=str,
                        default=' ',
                        help="Filter file delimiter [default: ' '].")

    parser.add_argument('-s', '--snapshot',
                        type=pathlib.Path,
                        required=True,
                        help='Wikipedia snapshot with the id-title mapping.'
                        )
    parser.add_argument('--snapshot-delimiter',
                        type=str,
                        default='\t',
                        help="Wikipedia snapshot delimiter [default: '\t']."
                        )

    parser.add_argument('--map',
                        type=pathlib.Path,
                        help="Map new ids to old ids."
                        )
    parser.add_argument('--map-delimiter',
                        type=str,
                        default=' ',
                        help="Map file delimiter [default: ' ']."
                        )
    parser.add_argument('--mapped-snapshot',
                        action='store_true',
                        help="Use mapped ids for the snapshot file."
                        )


    args = parser.parse_args()

    graphfile = args.GRAPH
    filterfile = args.filter
    snapshotfile = args.snapshot
    mapfile = args.map
    K = args.K
    output = args.output
    output_fields = args.print

    assert (set(output_fields) <= ALLOWED_FIELDS)

    graphfp = graphfile.open('r')
    graph_reader = csv.reader(graphfp, delimiter=args.delimiter)
    if args.skip_header:
        next(reader)

    tofilter = set()
    with filterfile.open('r') as filterfp:
        filter_reader = csv.reader(filterfp, delimiter=args.filter_delimiter)
        for line in filter_reader:
            # if (K is None) or (K and len(line) >= K)
            if not K or len(line) >= K:
                for el in line:
                    if args.match_titles:
                        tofilter.add(el)
                    else:
                        tofilter.add(int(el))

    snapshot = dict()
    with snapshotfile.open('r') as snapfp:
        snap_reader = csv.reader(snapfp, delimiter=args.snapshot_delimiter)
        snapshot = dict([(int(line[0]), line[1])
                         for line in snap_reader])

    mapo2n = dict()
    if mapfile:
        with mapfile.open('r') as mapfp:
            map_reader = csv.reader(mapfp, delimiter=args.map_delimiter)
            smapo2n = dict([(int(line[0]), line[1])
                            for line in map_reader])


    outfile = None
    if output is None:
        outfile = sys.stdout
    else:
        outfile = output.open('w+')

    outwriter = csv.DictWriter(outfile,
                               delimiter=args.output_delimiter,
                               fieldnames=output_fields)
    outwriter.writeheader()

    for line in graph_reader:
        fullout = None
        if len(line) == 2:
            # graph file is just: source, target
            if args.match_titles:
                source_title, target_title = line[0], line[1]
            else:
                source, target = int(line[0]), int(line[1])
                source_title = snapshot[source]
                target_title = snapshot[target]
        elif len(line) == 4:
            # graph file is: source_id, source_title, target_id, target_title
            source, target = int(line[0]), int(line[2])
            source_title = snapshot[source]
            target_title = snapshot[target]

        if args.match_titles:
            if source_title == target_title:
                continue
        else:
            if source == target:
                continue

        if args.match_titles:
            if source_title in tofilter and target_title in tofilter:
                fullout = {'source_title': source_title,
                           'target_title': target_title
                           }
        else:
            if source in tofilter and target in tofilter:
                fullout = {'source_id': source,
                           'source_title': source_title,
                           'target_id': target,
                           'target_title': target_title
                           }

        if fullout:
            out = {key: fullout[key]
                   for key in output_fields}

            outwriter.writerow(out)

    exit(0)
