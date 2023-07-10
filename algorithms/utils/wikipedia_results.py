#!/usr/bin/env python3

import sys
import csv
import re
import argparse
import pathlib
import itertools

# Regex:
#  score(<pageid>):<spaces><score>
#
# where:
#   - <pageid> is an integer number
#   - <score> is a real number that can be written using the scientific
#     notation
REGEX_SCORE = r'score\(([0-9]+)\):\s+([0-9]+\.?[0-9]*e?-?[0-9]*)'
regex_score = re.compile(REGEX_SCORE)


def process_line(line):
    match = regex_score.match(line)

    title = None
    score = None
    if match:
        pageid = int(match.group(1))
        score = float(match.group(2))
        title = snapshot[pageid]
    else:
        print('Error: could not process line: {}'.format(line),
              file=sys.stderr)

    # if match fails this is (None, None)
    return title, score


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Map wikipedia page ids to page titles.')

    parser.add_argument('-s', '--snapshot',
                        type=pathlib.Path,
                        required=True,
                        help='Wikipedia snapshot with the id-title mapping.'
                        )
    parser.add_argument('--sort',
                        choices=['score','title'],
                        help='Sort results in ascending order with respect '
                             'to the given column. Choose between '
                             '{score, title}.'
                        )
    parser.add_argument('-r', '--reverse',
                        action='store_true',
                        help='Sort results in desceding order. If --sort is '
                             'not given this option is effectively ignored.'
                        )


    args = parser.parse_args()

    snapshotname = args.snapshot
    outline = 'score({title}):\t{score}\n'
    with snapshotname.open('r') as snapshotfile:
        reader = csv.reader(snapshotfile, delimiter='\t')
        snapshot = dict((int(l[0]), l[1]) for l in reader)

    all_outlines = []
    try:
        for line in sys.stdin:
            title, score = process_line(line)

            if title:
                if args.sort:
                    all_outlines.append((title, score))
                else:
                    sys.stdout.write(outline.format(title=title,
                                                    score=repr(score)
                                                    )
                                     )
    except IOError as err:
        if err.errno == errno.EPIPE:
            pass

    if args.sort:
        sortkey = args.sort
        if sortkey == 'score':
            sorted_lines = sorted(all_outlines,
                                  key=lambda tup: tup[1],
                                  reverse=args.reverse
                                  )
        else:
            # sortkey == 'title'
            sorted_lines = sorted(all_outlines,
                                  key=lambda tup: tup[0],
                                  reverse=args.reverse
                                  )


        try:
            for title, score in sorted_lines:
                if title:
                    sys.stdout.write(outline.format(title=title,
                                                    score=repr(score)
                                                    )
                                     )
        except IOError as err:
            if err.errno == errno.EPIPE:
                pass

    exit(0)
