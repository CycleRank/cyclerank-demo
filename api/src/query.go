package main

import (
    "errors"
    "flag"
    "fmt"
    "io"
    "log"
    "os"
    "os/exec"
    "path"
    "strings"
    "time"

    "github.com/google/uuid"
    "gopkg.in/ini.v1"
)

var (
    bin = flag.String("bin", "./bin", "Binaries folder")

    // ErrQueryNotFound represents a failed query lookup by id
    ErrQueryNotFound = errors.New("query not found")
)

// Query represents a single query and matches the parameters of either: Cyclerank, PageRank, Single Source Personalized PageRank
// The endpoints are responsible of sorting out the mandatory parameters.
// The endpoints allow to avoid having a query call with parameters not accepted by an algorithm (example: ./pr -s 0 -k 3)
type Query struct {
    Algorithm        int // value used internally to determine the chosen algorithm
    File             string
    Query_to_analyze string // SubgraphGenerator attribute
    Maxnodes         int64  // SubgraphGenerator attribute
    Source           string // Cyclerank, Ssppr, Cheirp, 2drp attribute
    MaxLoop          uint64 // Cyclerank attribute
    Debug            bool
    Verbose          bool
    Scoring_function string  // Cyclerank attribute
    Alpha            float64 // Pr, Ssppr, Cheir, Cheirp, 2dr, 2drp attribute
}

// parse the query's parameters given a correct query file name
func Parse(initfile string) Query {

    cfg, err := ini.Load(initfile)
    if err != nil {
        log.Fatalf("Error opening .ini file in Parse() function: %s", err)
    }

    querySection := cfg.Section("query")

    query := Query{}
    query.Algorithm, _ = querySection.Key("algorithm_int").Int()
    query.File = strings.TrimPrefix(querySection.Key("file").String(), *repository)

    switch query.Algorithm {
    case CycleRank:
        query.MaxLoop, _ = querySection.Key("maxloop").Uint64()
        fallthrough
    case SingleSourcePersonalizedPageRank, SingleSource2dRank, SingleSourceCheiRank:
        query.Source = querySection.Key("source").String()
    case SubgraphGenerator:
        query.Query_to_analyze = querySection.Key("query").String()
    }

    if querySection.HasKey("debug") {
        query.Debug = true
    }
    if querySection.HasKey("verbose") {
        query.Verbose = true
    }
    if querySection.HasKey("alpha") {
        query.Alpha, _ = querySection.Key("alpha").Float64()
    }
    if querySection.HasKey("scoring_function") {
        query.Scoring_function = querySection.Key("scoring_function").String()
    }
    if querySection.HasKey("maxnodes") {
        query.Maxnodes, _ = querySection.Key("maxnodes").Int64()
    }

    return query
}

func PrettyFmt(q *Query) map[string]string {
    mappedQuery := make(map[string]string)

    mappedQuery["algorithm"] = Algorithms_str_full[q.Algorithm]
    mappedQuery["file"] = q.File

    switch q.Algorithm {
    case CycleRank:
        mappedQuery["maxloop"] = fmt.Sprint(q.MaxLoop)
        fallthrough
    case SingleSourcePersonalizedPageRank, SingleSource2dRank, SingleSourceCheiRank:
        mappedQuery["source"] = q.Source
    case SubgraphGenerator:
        mappedQuery["query_to_analyze"] = q.Query_to_analyze
    }

    if q.Debug {
        mappedQuery["debug"] = "true"
    }
    if q.Verbose {
        mappedQuery["verbose"] = "true"
    }
    if q.Alpha != -1.0 && q.Algorithm != CycleRank && q.Algorithm != SubgraphGenerator {
        mappedQuery["alpha"] = fmt.Sprint(q.Alpha)
    }
    if len(q.Scoring_function) > 0 {
        mappedQuery["scoring_function"] = q.Scoring_function
    }
    if q.Maxnodes != -1 && q.Algorithm == SubgraphGenerator {
        mappedQuery["maxnodes"] = fmt.Sprint(q.Maxnodes)
    }

    return mappedQuery
}

func (q Query) String() string {
    m := PrettyFmt(&q)
    keysAndValues := make([]string, 0, len(m))
    for k := range m {
        keysAndValues = append(keysAndValues, k+": "+m[k])
    }
    formattedQuery := "[" + strings.Join(keysAndValues, ", ") + "]"

    return formattedQuery
}

// Args generates the command line arguments for the various algorithm binaries.
//
//  lpa: The query to execute.
func Args(lpa Query) []string {
    args := []string{"--file", lpa.File}

    switch lpa.Algorithm {
    case CycleRank:
        args = append(args, "--maxloop", fmt.Sprint(lpa.MaxLoop))
        fallthrough
    case SingleSourcePersonalizedPageRank, SingleSource2dRank, SingleSourceCheiRank:
        args = append(args, "--source", lpa.Source)
    case SubgraphGenerator:
        args = append(args, "--query", lpa.Query_to_analyze)
    }

    if lpa.Debug {
        args = append(args, "--debug")
    }
    if lpa.Verbose {
        args = append(args, "--verbose")
    }
    if lpa.Alpha != -1.0 && lpa.Algorithm != CycleRank && lpa.Algorithm != SubgraphGenerator {
        args = append(args, "--alpha", fmt.Sprint(lpa.Alpha))
    }
    if len(lpa.Scoring_function) > 0 {
        args = append(args, "--scoring_function", lpa.Scoring_function)
    }
    if lpa.Maxnodes != -1 && lpa.Algorithm == SubgraphGenerator {
        args = append(args, "--maxnodes", fmt.Sprintf("%d", lpa.Maxnodes))
    }
    return args
}

// MakeOutputFiles creates and opens output files for stdout, stderr.
//
//  Usually used to setup stdout, stderr redirect for query execution.
func MakeOutputFiles(id uuid.UUID, job_id uuid.UUID) (*os.File, error, *os.File, error, *os.File, error) {
    p := path.Join(*outputDir, id.String())
    var p_withJobId string
    if job_id == uuid.Nil {
        p_withJobId = p
    } else {
        p_withJobId = path.Join(*outputDir, strings.Join([]string{
            id.String(),
            job_id.String()},
            "_"))
    }
    fOut, errOut := os.Create(strings.Join([]string{p_withJobId, "csv"}, "."))
    fLog, errLog := os.Create(strings.Join([]string{p_withJobId, "log"}, "."))
    fIni, errIni := os.Create(strings.Join([]string{p_withJobId, "ini"}, "."))
    return fOut, errOut, fLog, errLog, fIni, errIni
}

// SetupIniFile writes into the query's .ini file with the query's parameters
func SetupIniFile(q Query, cfg *ini.File, fIniName string) error {
    querySection, err := cfg.NewSection("query")
    if err != nil {
        return err
    }
    completionSection, err := cfg.NewSection("completion")
    if err != nil {
        return err
    }

    querySection.NewKey("algorithm", Algorithms_str_full[q.Algorithm])
    querySection.NewKey("algorithm_int", fmt.Sprint(q.Algorithm))
    queryargs := Args(q)
    for index, element := range queryargs {
        // FIXME: this is weak to sources with the likes of "--something". This is a problem of Args()
        if strings.HasPrefix(element, "--") && element != "--debug" && element != "--verbose" {
            querySection.NewKey(strings.TrimPrefix(element, "--"), queryargs[index+1])
        } else if element == "--debug" || element == "--verbose" {
            querySection.NewKey(strings.TrimPrefix(element, "--"), "true")
        }
    }

    completionSection.NewKey("time_start", time.Now().UTC().Format("20060102150405"))
    cfg.SaveTo(fIniName)

    return nil
}

// Start allocates resources, like output file, and start async query.
func Start(id uuid.UUID, q Query, fOut io.Writer, fLog io.Writer, callback func(), s *StatusRecorder, algorithm int) {
    // When killed by SIGTERM queries will be cleaned up and terminated
    // When killed by SIGKILL queries will continue and their output
    //  can be read by other instances, but status get lost.
    // TODO: Check what happens to redirects.

    cmd := exec.Command(path.Join(*bin, Bins[algorithm]), Args(q)...)
    cmd.Stdout = fOut
    cmd.Stderr = fLog

    err := cmd.Start()
    if err != nil {
        callback()
        log.Fatalf("Error starting query (%s): %s", q, err)
    }

    s.RegisterQuery(id, q, cmd.Process)

    go func() {
        if err := cmd.Wait(); err != nil {
            log.Printf("Query %s exited with error: %s", id, err)
        }
        callback()
        s.MarkCompleted(id)
    }()
}

// Kill given a query id, kills the corrisponding process.
func Kill(id uuid.UUID, s *StatusRecorder) error {
    if pid, err := s.GetPid(id); err != nil {
        return ErrQueryNotFound
    } else if p, err := os.FindProcess(pid); err != nil {
        return ErrQueryNotFound
    } else {
        return p.Kill()
    }
    // TODO: double check that this implies the relase of cmd.Wait()
}
