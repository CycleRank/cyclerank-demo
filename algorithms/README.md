# Cyclerank

## Files

```
/src             Actual source code
/libs            Vendored libraries
/examples        Example datasets
/utils           Utilities
```

## Binary generation prerequisites
In order to be able to compile the various binaries, installation of the igraph (0.10.1) is needed. 
Follow the tutorial here - https://igraph.org/c/html/0.10.1/igraph-Installation.html

## Building the algorithms' binaries
```
make all                Build all the binaries
make cyclerank          Build cyclerank's binary
make looprank_old       Build looprank's binary (old cyclerank version)
make pr                 Build pr's binary
make ssppr              Build ssppr's binary
make 2dr                Build 2dr's binary
make 2drp               Build 2drp's binary
make cheir              Build cheir's binary
make cheirp             Build cheirp's binary
make subgraphGenerator  Build subgraphGenerator's binary
```

## Usage
```
bin/cyclerank -h
Usage:
  bin/cyclerank [OPTION...]

  -f, --file FILE               Input file.
  -v, --verbose                 Enable logging at verbose level.
  -d, --debug                   Enable logging at debug level.
  -h, --help                    Show help message and exit.
  -k, --maxloop K               Set max loop length (K).
  -l, --log LOG_FILE            Logfile [default: stderr].
  -o, --output OUTPUT_FILE      Output file [default: stdout].
  -s, --source S                Set source node (S).
  -p, --print_cycles            Print the cycles [default: prints the
                                scores].
  -e, --scoring_function SCORING_FUNCTION
                                Scoring function {exp, lin, quad, log}
                                [default: exp].
  -t, --top-results TOP_RESULTS
                                Print the top-t results. [default: prints top
                                1000 results].
----------------------------------------------------------------------------------
bin/pr -h
  bin/pr [OPTION...]

  -a, --alpha ALPHA             Damping factor (alpha).
  -f, --file FILE               Input file.
  -v, --verbose                 Enable logging at verbose level.
  -d, --debug                   Enable logging at debug level.
  -h, --help                    Show help message and exit.
  -l, --log LOG_FILE            Logfile [default: stderr].
  -o, --output OUTPUT_FILE      Output file.
  -t, --top-results TOP_RESULTS
                                Print the top-t results. [default: prints top
                                1000 results].
----------------------------------------------------------------------------------
bin/ssppr -h
Usage:
  bin/ssppr [OPTION...]

  -a, --alpha ALPHA             Damping factor (alpha).
  -f, --file FILE               Input file.
  -v, --verbose                 Enable logging at verbose level.
  -d, --debug                   Enable logging at debug level.
  -h, --help                    Show help message and exit.
  -l, --log LOG_FILE            Logfile [default: stderr].
  -o, --output OUTPUT_FILE      Output file.
  -s, --source S                Set source node (S).
  -t, --top-results TOP_RESULTS
                                Print the top-t results. [default: prints top
                                1000 results].
----------------------------------------------------------------------------------
bin/2dr -h
Usage:
  bin/2dr [OPTION...]

  -a, --alpha ALPHA         Damping factor (alpha).
  -f, --file FILE           Input file.
  -v, --verbose             Enable logging at verbose level.
  -d, --debug               Enable logging at debug level.
  -h, --help                Show help message and exit.
  -l, --log LOG_FILE        Logfile [default: stderr].
  -o, --output OUTPUT_FILE  Output file.
  -t, --top-results TOP_RESULTS
                                Print the top-t results. [default: prints top
                                1000 results].
----------------------------------------------------------------------------------
bin/2drp -h
Usage:
  bin/2drp [OPTION...]

  -a, --alpha ALPHA         Damping factor (alpha).
  -f, --file FILE           Input file.
  -v, --verbose             Enable logging at verbose level.
  -d, --debug               Enable logging at debug level.
  -h, --help                Show help message and exit.
  -l, --log LOG_FILE        Logfile [default: stderr].
  -o, --output OUTPUT_FILE  Output file.
  -s, --source S            Set source node (S).
  -t, --top-results TOP_RESULTS
                                Print the top-t results. [default: prints top
                                1000 results].
----------------------------------------------------------------------------------
bin/cheir -h
Usage:
  bin/cheir [OPTION...]

  -a, --alpha ALPHA         Damping factor (alpha).
  -f, --file FILE           Input file.
  -v, --verbose             Enable logging at verbose level.
  -d, --debug               Enable logging at debug level.
  -h, --help                Show help message and exit.
  -l, --log LOG_FILE        Logfile [default: stderr].
  -o, --output OUTPUT_FILE  Output file.
  -t, --top-results TOP_RESULTS
                                Print the top-t results. [default: prints top
                                1000 results].
----------------------------------------------------------------------------------
bin/cheirp -h
Usage:
  bin/cheirp [OPTION...]

  -a, --alpha ALPHA         Damping factor (alpha).
  -f, --file FILE           Input file.
  -v, --verbose             Enable logging at verbose level.
  -d, --debug               Enable logging at debug level.
  -h, --help                Show help message and exit.
  -l, --log LOG_FILE        Logfile [default: stderr].
  -o, --output OUTPUT_FILE  Output file.
  -s, --source S            Set source node (S).
  -t, --top-results TOP_RESULTS
                                Print the top-t results. [default: prints top
                                1000 results].
----------------------------------------------------------------------------------
bin/subgraphGenerator -h
Usage:
  bin/subgraphGenerator [OPTION...]

  -f, --file FILE           Input dataset file.
  -q, --query FILE          Input query file.
  -n, --maxnodes N          Set max number of nodes of the subgraph.
  -v, --verbose             Enable logging at verbose level.
  -d, --debug               Enable logging at debug level.
  -h, --help                Show help message and exit.
  -l, --log LOG_FILE        Logfile [default: stderr].
  -o, --output OUTPUT_FILE  Output file [default: stdout].
```

## Testing Locally

To test locally:

```
$ bin/cyclerank -f examples/toy0/toy0.in.txt -o output.txt -s 0 -k 3
$ bin/pr -f examples/toy0/toy0.in.txt -o output.txt -a 0.85
$ bin/ssppr -f examples/toy0/toy0.in.txt -o output.txt -s 0 -k 3 -a 0.85
$ bin/2dr -f examples/toy0/toy0.in.txt -o output.txt -a 0.85
$ bin/cheir -f examples/toy0/toy0.in.txt -o output.txt -a 0.85
$ bin/cheirp -f examples/toy0/toy0.in.txt -o output.txt -s 0 -a 0.85
$ bin/subgraphGenerator -f examples/toy0/toy0.in.txt -q examples/toy0/pr/toy0.out-cr-s0-k3.txt -o output.txt -n 1000
```
