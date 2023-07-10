package main

import (
	"bufio"
	"errors"
	"io"
	"io/ioutil"
	"log"
	"path"
	"sort"
	"strconv"
	"strings"

	"github.com/google/uuid"
	"gopkg.in/ini.v1"
)

// QueryResults represents a whole parsed output file
type QueryResults []QueryResult

// QueryResult represent a single entry of an output file
type QueryResult struct {
	ID    uint64  `json:"id"`
	Score float64 `json:"score"`
	Label string  `json:"label,omitempty"`
}

// SubgraphEdges represents a subgraph file's parsed edges
type SubgraphEdges []SubgraphEdge

// QueryResult represent a single entry of an output file
type SubgraphEdge struct {
	From uint64 `json:"from"`
	To   uint64 `json:"to"`
}

var (
	// ErrMalformedOutput is raise when a malformed output line is encountered.
	// It may happen when the LoopRank process is killed or crashes.
	ErrMalformedOutput = errors.New("malformed output line")
)

// FindQueryFilesByID finds the output, init and log files' paths from the provided query id.
func FindQueryFilesByID(id uuid.UUID) (string, string, string, error) {
	ls, err := ioutil.ReadDir(*outputDir)
	if err != nil {
		return "", "", "", err
	}

	idStr := id.String()
	// binary search that returns the closest result
	idx := sort.Search(len(ls), func(i int) bool { return ls[i].Name() >= idStr })
	if idx >= len(ls) || idx+1 >= len(ls) || idx+2 >= len(ls) || !strings.HasPrefix(ls[idx].Name(), idStr) || !strings.HasPrefix(ls[idx+1].Name(), idStr) || !strings.HasPrefix(ls[idx+2].Name(), idStr) {
		return "", "", "", ErrQueryNotFound
	}

	return path.Join(*outputDir, ls[idx].Name()), path.Join(*outputDir, ls[idx+1].Name()), path.Join(*outputDir, ls[idx+2].Name()), nil
}

func FindDatasetWithInitFile(initfile string) (string, error) {
	cfg, err := ini.Load(initfile)
	if err != nil {
		log.Fatalf("Error opening .ini file in FindDatasetWithInitFile() function: %s", err)
	}

	return cfg.Section("query").Key("file").String(), nil
}

// ParseOutput reads the content of a file output in a QueryResults
func ParseOutput(data *bufio.Reader, limit int64) (QueryResults, error) {
	results := QueryResults{}
	for {
		if (limit > 0 && len(results) >= int(limit)) {
			break
		}
		line, err := data.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				break
			}
			return nil, err
		}

		line = strings.Trim(line, "\n ")

		tokens := strings.Split(line, "\t")
		var qs = QueryResult{}
		qs.ID, err = strconv.ParseUint(tokens[0], 10, 64)
		if err != nil {
			return nil, ErrMalformedOutput
		}
		// TODO: add MalformedOutput check for this string. Maybe if the Label is empty?
		qs.Label = tokens[1]
		qs.Score, err = strconv.ParseFloat(tokens[2], 64)
		if err != nil {
			return nil, ErrMalformedOutput
		}

		results = append(results, qs)
	}
	return results, nil
}

// ParseOutput reads the content of a file output in a QueryResults
func ParseOutputSubgraph(data *bufio.Reader) (QueryResults, SubgraphEdges, map[uint64]string, error) {
	nodes := QueryResults{}
	edges := SubgraphEdges{}
	clusters := make(map[uint64]string)
	for {
		line, err := data.ReadString('\n')
		if err != nil {
			// io.EOF is not an error, even before reaching the edges: the subgraph may be edgeless
			// FIXME: this may not be true anymore, since we also append node clusters
			if err == io.EOF {
				break
			}
			return nil, nil, nil, err
		}

		line = strings.Trim(line, "\n ")
		if line == "---" {
			// nodes have ended
			break
		}

		tokens := strings.Split(line, "\t")
		var qs = QueryResult{}
		qs.ID, err = strconv.ParseUint(tokens[0], 10, 64)
		if err != nil {
			return nil, nil, nil, ErrMalformedOutput
		}
		// TODO: add MalformedOutput check for this string. Maybe if the Label is empty?
		qs.Label = tokens[1]
		qs.Score, err = strconv.ParseFloat(tokens[2], 64)
		if err != nil {
			return nil, nil, nil, ErrMalformedOutput
		}

		nodes = append(nodes, qs)
	}
	for {
		line, err := data.ReadString('\n')
		if err != nil {
			// FIXME: check if EOF is an error if clusters are included
			if err == io.EOF {
				break
			}
			return nil, nil, nil, err
		}

		line = strings.Trim(line, "\n ")
		if line == "---" {
			// edges have ended
			break
		}

		tokens := strings.Split(line, "\t")
		var se = SubgraphEdge{}

		se.From, err = strconv.ParseUint(tokens[0], 10, 64)
		if err != nil {
			return nil, nil, nil, ErrMalformedOutput
		}
		se.To, err = strconv.ParseUint(tokens[1], 10, 64)
		if err != nil {
			return nil, nil, nil, ErrMalformedOutput
		}

		edges = append(edges, se)
	}
	for {
		line, err := data.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				break
			}
			return nil, nil, nil, err
		}

		line = strings.Trim(line, "\n ")

		tokens := strings.Split(line, "\t")

		nodeID, err := strconv.ParseUint(tokens[0], 10, 64)
		if err != nil {
			return nil, nil, nil, ErrMalformedOutput
		}

		clusters[nodeID] = tokens[1]
		// log.Printf("Cluster %d: %s", nodeID, clusters[nodeID])
	}
	return nodes, edges, clusters, nil
}
