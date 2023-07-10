package main

import (
	"log"
	"path"
	"strconv"
)

// an enum is used to enumerate the existing algorithms
const (
	CycleRank                        int = 0
	PageRank                         int = 1
	SingleSourcePersonalizedPageRank int = 2
	Rank2d                           int = 3
	SingleSource2dRank               int = 4
	CheiRank                         int = 5
	SingleSourceCheiRank             int = 6
	SubgraphGenerator                int = 7
)

// var Algorithms_str = []string{"cr", "pr", "ssppr", "2dr", "2drp", "cheir", "cheirp", "subgr"}
// var Algorithms_str_full = []string{"CycleRank", "PageRank", "SingleSourcePersonalizedPageRank", "Rank2d", "SingleSource2dRank", "CheiRank", "SingleSourceCheiRank", "SubgraphGenerator"}
var Algorithms_str_full = []string{
	"Cyclerank",
	"PageRank",
	"Single Source Personalized PageRank",
	"2dRank",
	"Single Source 2dRank",
	"CheiRank",
	"Single Source CheiRank",
	"Subgraph Generator",
}

var Bins = []string{
	"cyclerank",
	"pr",
	"ssppr",
	"2dr",
	"2drp",
	"cheir",
	"cheirp",
	"subgraphGenerator",
}

// SafeFilePath derives a path for the root and rel_path,
// making sure it is contained under the root itself.
// Root can be absolute or relative, while rel_path must be relative.
func SafeFilePath(root, rel_path string) string {
	// FIXME: there is nothing safe here ... strip '..' and '.' instead.
	// something of /.  must disappear  /...    ....txt   ./. ../..  to avoid getting out of the outputDir folder

	// return path.Join(root, path.Base(rel_path) path.Join(string(os.PathSeparator), rel_path))
	return path.Join(root, path.Base(rel_path))
}

func MustParseUint(value string, base, bitSize int) uint64 {
	v, err := strconv.ParseUint(value, base, bitSize)
	if err != nil {
		log.Fatal(err)
	}
	return v
}

func MustParseInt(value string, base, bitSize int) int64 {
	v, err := strconv.ParseInt(value, base, bitSize)
	if err != nil {
		log.Fatal(err)
	}
	return v
}

func MustParseBool(value string) bool {
	v, err := strconv.ParseBool(value)
	if err != nil {
		log.Fatal(err)
	}
	return v
}

func MustParseFloat(value string, bitSize int) float64 {
	v, err := strconv.ParseFloat(value, bitSize)
	if err != nil {
		log.Fatal(err)
	}
	return v
}
