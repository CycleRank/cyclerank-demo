# CycleRank API

## Building the API binary

To build the binary do:
```
cd src/
go build
```

## Usage

```
./cyclerank-api -help
Usage of ./cyclerank-api:
  -addr string
    	[ip]:port (default ":8080")
  -bincyclerank string
        Path to CycleRank binary. (default "bin/cyclerank")
  -binpr string
        Path to PageRank binary. (default "bin/pr")
  -binssppr string
        Path to Single Source PageRank binary. (default "bin/ssppr")
  -bin2dr string
        Path to 2dRank binary. (default "bin/2dr")
  -bin2drp string
        Path to Single Source 2dRank binary. (default "bin/2drp")
  -bincheir string
        Path to CheiRank binary. (default "bin/cheir")
  -bincheirp string
        Path to Single Source CheiRank binary. (default "bin/cheirp")
  -output_dir string
        Output directory path (default "output/")
  -repository string
        Dataset repository path (default "data/")
  -utils string
        Utilities directory path (default "../utils")
```

## Testing Locally

To test locally:

```bash
./cyclerank-api -addr 127.0.0.1:8080
```

