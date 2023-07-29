package main

import (
	"bufio"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"

	"github.com/google/uuid"
	"gopkg.in/ini.v1"
)

// RegisterAPI register api endpoints for version v0
func RegisterAPI() {
	// algorithms
	http.HandleFunc("/v0/cyclerank", HTTPJSON(HTTPCyclerank))
	http.HandleFunc("/v0/pr", HTTPJSON(HTTPPr))
	http.HandleFunc("/v0/ssppr", HTTPJSON(HTTPSsppr))
	http.HandleFunc("/v0/2dr", HTTPJSON(HTTP2dr))
	http.HandleFunc("/v0/2drp", HTTPJSON(HTTP2drp))
	http.HandleFunc("/v0/cheir", HTTPJSON(HTTPCheir))
	http.HandleFunc("/v0/cheirp", HTTPJSON(HTTPCheirp))
	http.HandleFunc("/v0/subgraph", HTTPJSON(HTTPSubgraph))

	// utilities
	http.HandleFunc("/v0/output", HTTPJSON(HTTPOutput))
	http.HandleFunc("/v0/outputSubgraph", HTTPJSON(HTTPOutputSubgraph))
	http.HandleFunc("/v0/log", HTTPLog)
	http.HandleFunc("/v0/kill", HTTPJSON(HTTPKill))
	http.HandleFunc("/v0/status", HTTPJSON(HTTPStatus))
	// usage: curl -v -X POST -F "file=@/local/path/to/hello.txt" http://localhost:8080/v0/upload
	http.HandleFunc("/v0/upload", HTTPJSON(HTTPUploadFile))
	http.HandleFunc("/v0/datasets", HTTPJSON(HTTPDatasets))
	http.HandleFunc("/v0/available", HTTPJSON(HTTPAvailable))

	// documentation
	fs := http.FileServer(http.Dir("./swaggerui"))
	http.Handle("/docs/", http.StripPrefix("/docs/", fs))
}

// HTTPJSON wraps an http.HandleFunc and sets the Content-Type header for JSON
func HTTPJSON(f func(http.ResponseWriter, *http.Request)) func(http.ResponseWriter, *http.Request) {
	return func(res http.ResponseWriter, req *http.Request) {
		// TODO: remove once the page is served by this binary
		//       (easier like this for dev)
		res.Header().Set("Access-Control-Allow-Origin", "*")
		res.Header().Set("Content-Type", "application/json")
		res.Header().Set("Cache-Control", "no-store, no-cache, max-age=0, must-revalidate, proxy-revalidate")
		f(res, req)
	}
}

// MustMarshal marhal json or Fatal on errors.
func MustMarshal(data interface{}) []byte {
	ret, err := json.Marshal(data)
	if err != nil {
		log.Fatal(err)
	}
	return ret
}

// HTTPStatus handler for status endpoint
func HTTPStatus(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodGet {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	status.Dump(res)
}

// HTTPKill handler for kill endpoint
func HTTPKill(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	if err := req.ParseForm(); err != nil {
		log.Printf("Error parsing Kill POST: %s", err)
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	if !FormIsSet(req.PostForm, "id") {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"id": ErrRequiredField.Error()}))
		return
	}

	id, err := uuid.Parse(req.PostForm["id"][0])
	if err != nil {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"id": "must be a valid uuid"}))
		return
	}

	if err := Kill(id, status); err == ErrQueryNotFound {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"id": err.Error()}))
	} else if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		// TODO: In v1 just log and return 500.
		res.Write(MustMarshal(map[string]string{"error": err.Error()}))
	} else {
		res.WriteHeader(http.StatusOK)
	}
}

// QueryForm is the singleton instance for Subgraphgenerator's Query structure.
var QueryForm_Subgraph = Form{
	"query_to_analyze": []ValidationFunction{FieldRequired, FieldIsUuid},
	"job_id":           []ValidationFunction{FieldOptional, FieldIsUuid},
	"maxnodes":         []ValidationFunction{FieldOptional, FieldInt},
	"debug":            []ValidationFunction{FieldOptional, FieldBoolean},
	"verbose":          []ValidationFunction{FieldOptional, FieldBoolean},
}

// HTTPSubgraph parses the subgraph of a dataset, given a query id to set the collection of nodes to use
func HTTPSubgraph(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	if err := req.ParseForm(); err != nil {
		log.Printf("Error parsing Subgraph POST: %s", err)
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	if errors := QueryForm_Subgraph.Validate(req.PostForm); len(errors) != 0 {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(errors))
		return
	}

	analyzeid, err := uuid.Parse(req.Form["query_to_analyze"][0])
	if err != nil {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"query_to_analyze": "must be a valid uuid"}))
		return
	}

	input_query, initfile, _, err := FindQueryFilesByID(analyzeid)
	if err == ErrQueryNotFound {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"id": err.Error()}))
		return
	} else if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Did not find output by id %s: %s", analyzeid, err)
		return
	}

	input_dataset, err := FindDatasetWithInitFile(initfile)
	if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Couldn't open .ini file %s: %s", initfile, err)
		return
	}

	q := Query{
		Algorithm:        SubgraphGenerator,
		File:             input_dataset,
		Query_to_analyze: input_query,
	}

	if FormIsSet(req.PostForm, "debug") {
		q.Debug = MustParseBool(req.PostForm["debug"][0])
	}

	if FormIsSet(req.PostForm, "verbose") {
		q.Verbose = MustParseBool(req.PostForm["verbose"][0])
	}

	if FormIsSet(req.PostForm, "maxnodes") {
		if req.PostForm["maxnodes"][0] == "" {
			q.Maxnodes = -1
		} else {
			q.Maxnodes = MustParseInt(req.PostForm["maxnodes"][0], 10, 64)
		}
	}

	if !FormIsSet(req.PostForm, "maxnodes") {
		q.Maxnodes = -1
	}

	var job_id uuid.UUID
	if FormIsSet(req.PostForm, "job_id") {
		job_id, _ = uuid.Parse(req.PostForm["job_id"][0])
	} else {
		job_id = uuid.Nil
	}

	id := uuid.New()
	fOut, eOut, fLog, eLog, fIni, eIni := MakeOutputFiles(id, job_id)
	if eOut != nil || eLog != nil || eIni != nil {
		log.Printf("Error allocating output files %q: (%s, %s, %s)", q, eOut, eLog, eIni)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	fIni.Close()
	cfg, err := ini.Load(fIni.Name())
	if err != nil {
		log.Printf("Fail to read ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	if err := SetupIniFile(q, cfg, fIni.Name()); err != nil {
		log.Printf("Fail to write ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, SubgraphGenerator)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

// QueryForm is the singleton instance for Cyclerank's Query structure.
var QueryForm_Cyclerank = Form{
	"file":             []ValidationFunction{FieldRequired, FieldSafePath, FieldFileExists},
	"source":           []ValidationFunction{FieldRequired},
	"maxloop":          []ValidationFunction{FieldRequired, FieldStrictlyPositiveInt},
	"job_id":           []ValidationFunction{FieldOptional, FieldIsUuid},
	"debug":            []ValidationFunction{FieldOptional, FieldBoolean},
	"verbose":          []ValidationFunction{FieldOptional, FieldBoolean},
	"scoring_function": []ValidationFunction{FieldOptional, FieldCorrectScoringFunction},
	"top_results":      []ValidationFunction{FieldOptional, FieldStrictlyPositiveInt},
}

// HTTPCyclerank is the handler for the cyclerank endpoint
func HTTPCyclerank(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	if err := req.ParseForm(); err != nil {
		log.Printf("Error parsing Cyclerank POST: %s", err)
		res.WriteHeader(http.StatusBadRequest)
		return
	}

	if errors := QueryForm_Cyclerank.Validate(req.PostForm); len(errors) != 0 {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(errors))
		return
	}

	q := Query{
		Algorithm: CycleRank,
		File:      SafeFilePath(*repository, req.PostForm["file"][0]),
		Source:    req.PostForm["source"][0],
		MaxLoop:   MustParseUint(req.PostForm["maxloop"][0], 10, 64),
	}

	if FormIsSet(req.PostForm, "debug") {
		q.Debug = MustParseBool(req.PostForm["debug"][0])
	}

	if FormIsSet(req.PostForm, "verbose") {
		q.Verbose = MustParseBool(req.PostForm["verbose"][0])
	}

	if FormIsSet(req.PostForm, "scoring_function") {
		q.Scoring_function = req.PostForm["scoring_function"][0]
	}

	if FormIsSet(req.PostForm, "top_results") {
		q.Top_results = MustParseUint(req.PostForm["top_results"][0], 10, 64)
	}

	var job_id uuid.UUID
	if FormIsSet(req.PostForm, "job_id") {
		job_id, _ = uuid.Parse(req.PostForm["job_id"][0])
	} else {
		job_id = uuid.Nil
	}

	id := uuid.New()
	fOut, eOut, fLog, eLog, fIni, eIni := MakeOutputFiles(id, job_id)
	if eOut != nil || eLog != nil || eIni != nil {
		log.Printf("Error allocating output files %q: (%s, %s, %s)", q, eOut, eLog, eIni)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	fIni.Close()
	cfg, err := ini.Load(fIni.Name())
	if err != nil {
		log.Printf("Fail to read ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	if err := SetupIniFile(q, cfg, fIni.Name()); err != nil {
		log.Printf("Fail to write ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}
	// TODO: add missing "time_end" at the end of the query's completion
	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, CycleRank)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

// common function body shared by pr based algorithms (pr, cheir, 2dr)
func PrBasedAlgorithmsCommonBody(res http.ResponseWriter, req *http.Request, algorithm int) (uuid.UUID, Query, *os.File, *os.File, error) {
	var ErrInterruptAlgorithm = errors.New("error inside Pr common body")
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	if err := req.ParseForm(); err != nil {
		log.Printf("Error parsing Pr/Cheir/2dr POST: %s", err)
		res.WriteHeader(http.StatusBadRequest)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	if errors := QueryForm_Pr.Validate(req.PostForm); len(errors) != 0 {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(errors))
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	q := Query{
		Algorithm: algorithm,
		File:      SafeFilePath(*repository, req.PostForm["file"][0]),
	}

	if FormIsSet(req.PostForm, "alpha") {
		q.Alpha = MustParseFloat(req.PostForm["alpha"][0], 64)
	}

	if FormIsSet(req.PostForm, "debug") {
		q.Debug = MustParseBool(req.PostForm["debug"][0])
	}

	if FormIsSet(req.PostForm, "verbose") {
		q.Verbose = MustParseBool(req.PostForm["verbose"][0])
	}

	if FormIsSet(req.PostForm, "top_results") {
		q.Top_results = MustParseUint(req.PostForm["top_results"][0], 10, 64)
	}

	var job_id uuid.UUID
	if FormIsSet(req.PostForm, "job_id") {
		job_id, _ = uuid.Parse(req.PostForm["job_id"][0])
	} else {
		job_id = uuid.Nil
	}

	id := uuid.New()
	fOut, eOut, fLog, eLog, fIni, eIni := MakeOutputFiles(id, job_id)
	if eOut != nil || eLog != nil || eIni != nil {
		log.Printf("Error allocating output files %q: (%s, %s, %s)", q, eOut, eLog, eIni)
		res.WriteHeader(http.StatusInternalServerError)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	fIni.Close()
	cfg, err := ini.Load(fIni.Name())
	if err != nil {
		log.Printf("Fail to read ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	if err := SetupIniFile(q, cfg, fIni.Name()); err != nil {
		log.Printf("Fail to write ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}
	return id, q, fOut, fLog, nil
}

// QueryForm_Pr is the singleton instance for Pr's Query structure.
var QueryForm_Pr = Form{
	"file":    		[]ValidationFunction{FieldRequired, FieldSafePath, FieldFileExists},
	"alpha":   		[]ValidationFunction{FieldOptional, FieldFloatBetweenZeroAndOne},
	"job_id":  		[]ValidationFunction{FieldOptional, FieldIsUuid},
	"debug":   		[]ValidationFunction{FieldOptional, FieldBoolean},
	"verbose": 		[]ValidationFunction{FieldOptional, FieldBoolean},
	"top_results":  []ValidationFunction{FieldOptional, FieldStrictlyPositiveInt},
}

// HTTPPr is the handler for the PageRank endpoint
func HTTPPr(res http.ResponseWriter, req *http.Request) {
	id, q, fOut, fLog, err := PrBasedAlgorithmsCommonBody(res, req, PageRank)

	if err != nil {
		return
	}

	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, PageRank)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

// HTTP2dr is the handler for the 2dRank endpoint
func HTTP2dr(res http.ResponseWriter, req *http.Request) {
	id, q, fOut, fLog, err := PrBasedAlgorithmsCommonBody(res, req, Rank2d)

	if err != nil {
		return
	}

	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, Rank2d)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

// HTTPCheir is the handler for the CheiRank endpoint
func HTTPCheir(res http.ResponseWriter, req *http.Request) {
	id, q, fOut, fLog, err := PrBasedAlgorithmsCommonBody(res, req, CheiRank)

	if err != nil {
		return
	}

	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, CheiRank)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

// common function body shared by ssppr based algorithms (ssppr, cheirp, 2drp)
func SspprBasedAlgorithmsCommonBody(res http.ResponseWriter, req *http.Request, algorithm int) (uuid.UUID, Query, *os.File, *os.File, error) {
	var ErrInterruptAlgorithm = errors.New("error inside Ssppr common body")
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	if err := req.ParseForm(); err != nil {
		log.Printf("Error parsing Ssppr/Cheirp/2drp POST: %s", err)
		res.WriteHeader(http.StatusBadRequest)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	if errors := QueryForm_Ssppr.Validate(req.PostForm); len(errors) != 0 {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(errors))
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	q := Query{
		Algorithm: algorithm,
		File:      SafeFilePath(*repository, req.PostForm["file"][0]),
		Source:    req.PostForm["source"][0],
	}

	if FormIsSet(req.PostForm, "alpha") {
		q.Alpha = MustParseFloat(req.PostForm["alpha"][0], 64)
	}

	if !FormIsSet(req.PostForm, "alpha") {
		// internal value to signal query.go that there was no alpha in input
		q.Alpha = -1.0
	}

	if FormIsSet(req.PostForm, "debug") {
		q.Debug = MustParseBool(req.PostForm["debug"][0])
	}

	if FormIsSet(req.PostForm, "verbose") {
		q.Verbose = MustParseBool(req.PostForm["verbose"][0])
	}

	if FormIsSet(req.PostForm, "top_results") {
		q.Top_results = MustParseUint(req.PostForm["top_results"][0], 10, 64)
	}

	var job_id uuid.UUID
	if FormIsSet(req.PostForm, "job_id") {
		job_id, _ = uuid.Parse(req.PostForm["job_id"][0])
	} else {
		job_id = uuid.Nil
	}

	id := uuid.New()
	fOut, eOut, fLog, eLog, fIni, eIni := MakeOutputFiles(id, job_id)
	if eOut != nil || eLog != nil || eIni != nil {
		log.Printf("Error allocating output files %q: (%s, %s, %s)", q, eOut, eLog, eIni)
		res.WriteHeader(http.StatusInternalServerError)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	fIni.Close()
	cfg, err := ini.Load(fIni.Name())
	if err != nil {
		log.Printf("Fail to read ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}

	if err := SetupIniFile(q, cfg, fIni.Name()); err != nil {
		log.Printf("Fail to write ini file: %v", err)
		res.WriteHeader(http.StatusInternalServerError)
		return uuid.Nil, Query{}, nil, nil, ErrInterruptAlgorithm
	}
	return id, q, fOut, fLog, nil
}

// QueryForm_Ssppr is the singleton instance for Ssppr's Query structure.
var QueryForm_Ssppr = Form{
	"file":    	   []ValidationFunction{FieldRequired, FieldSafePath, FieldFileExists},
	"source":  	   []ValidationFunction{FieldRequired},
	"alpha":   	   []ValidationFunction{FieldOptional, FieldFloatBetweenZeroAndOne},
	"job_id":  	   []ValidationFunction{FieldOptional, FieldIsUuid},
	"debug":   	   []ValidationFunction{FieldOptional, FieldBoolean},
	"verbose": 	   []ValidationFunction{FieldOptional, FieldBoolean},
	"top_results": []ValidationFunction{FieldOptional, FieldStrictlyPositiveInt},
}

// HTTPSsppr is the handler for the Single Source Personalized PageRank endpoint
func HTTPSsppr(res http.ResponseWriter, req *http.Request) {
	id, q, fOut, fLog, err := SspprBasedAlgorithmsCommonBody(res, req, SingleSourcePersonalizedPageRank)

	if err != nil {
		return
	}

	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, SingleSourcePersonalizedPageRank)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

// HTTP2drp is the handler for the Single Source 2dRank endpoint
func HTTP2drp(res http.ResponseWriter, req *http.Request) {
	id, q, fOut, fLog, err := SspprBasedAlgorithmsCommonBody(res, req, SingleSource2dRank)

	if err != nil {
		return
	}

	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, SingleSource2dRank)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

// HTTPCheirp is the handler for the Single Source CheiRank endpoint
func HTTPCheirp(res http.ResponseWriter, req *http.Request) {
	id, q, fOut, fLog, err := SspprBasedAlgorithmsCommonBody(res, req, SingleSourceCheiRank)

	if err != nil {
		return
	}

	o := bufio.NewWriter(fOut)
	l := bufio.NewWriter(fLog)
	Start(id, q, o, l, func() {
		o.Flush()
		l.Flush()
		fOut.Close()
		fLog.Close()
	}, status, SingleSourceCheiRank)

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(map[string]string{"id": id.String()}))
}

func OutputAndLogCommonBody(res http.ResponseWriter, req *http.Request) (uuid.UUID, int64, string, string, string, error) {
	var ErrInterruptOutputAndLog = errors.New("error inside Output/Log common body")
	if req.Method != http.MethodGet {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return uuid.Nil, 0, "", "", "", ErrInterruptOutputAndLog
	}

	if err := req.ParseForm(); err != nil {
		log.Printf("Error parsing Output GET: %v", err)
		res.WriteHeader(http.StatusBadRequest)
		return uuid.Nil, 0, "", "", "", ErrInterruptOutputAndLog
	}

	if !FormIsSet(req.Form, "id") {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"id": ErrRequiredField.Error()}))
		return uuid.Nil, 0, "", "", "", ErrInterruptOutputAndLog
	}

	var limit int64
	if FormIsSet(req.Form, "limit") {
		limit = MustParseInt(req.Form["limit"][0], 10, 64)
	}

	id, err := uuid.Parse(req.Form["id"][0])
	if err != nil {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"id": "must be a valid uuid"}))
		return uuid.Nil, 0, "", "", "", ErrInterruptOutputAndLog
	}

	outfile, initfile, logfile, err := FindQueryFilesByID(id)
	if err == ErrQueryNotFound {
		res.WriteHeader(http.StatusUnprocessableEntity)
		res.Write(MustMarshal(map[string]string{"id": err.Error()}))
		return uuid.Nil, 0, "", "", "", ErrInterruptOutputAndLog
	} else if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Did not find output by id %s: %s", id, err)
		return uuid.Nil, 0, "", "", "", ErrInterruptOutputAndLog
	}

	return id, limit, outfile, initfile, logfile, nil
}

type TaskOutput struct {
	JobID   string            `json:"jobid"`
	Task    uuid.UUID         `json:"task"`
	Name    string            `json:"name"`
	Query   map[string]string `json:"params"`
	Ranking QueryResults      `json:"ranking"`
}

// HTTPOutput reads back the output of a task
func HTTPOutput(res http.ResponseWriter, req *http.Request) {
	id, limit, outfile, initfile, _, err := OutputAndLogCommonBody(res, req)

	if err != nil {
		return
	}

	f, err := os.Open(outfile)
	if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Cannot parse output for id %s (%s): %s", id, outfile, err)
		return
	}
	defer f.Close()

	q := Parse(initfile)
	// formattedQuery := Parse(initfile).String()
	name := filepath.Base(f.Name())

	to := TaskOutput{
		JobID: strings.TrimRight(strings.Split(name, "_")[1], ".csv"),
		Task:  id,
		Name:  name,
		Query: PrettyFmt(&q),
	}

	if results, err := ParseOutput(bufio.NewReader(f), limit); err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Cannot parse output for id %s (%s): %s", id, outfile, err)
		return
	} else if len(results) > 0 {
		to.Ranking = results
	}

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(to))
}

// HTTPLog reads back the log of a task
func HTTPLog(res http.ResponseWriter, req *http.Request) {
	id, _, _, _, logfile, err := OutputAndLogCommonBody(res, req)

	if err != nil {
		return
	}

	f, err := os.Open(logfile)
	if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Cannot parse log for id %s (%s): %s", id, logfile, err)
		return
	}
	defer f.Close()

	lines, err := ioutil.ReadAll(f)
	if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Cannot read logfile for id %s (%s): %s", id, logfile, err)
		return
	}

	//log.Printf("Request to parse log of id %s received.", id)

	res.WriteHeader(http.StatusOK)
	res.Header().Set("Content-Type", "text/plain")
	res.Write(lines)
}

type TaskOutputSubgraph struct {
	Task     uuid.UUID         `json:"task"`
	Name     string            `json:"name"`
	QueryID  string            `json:"query_task"`
	Maxnodes string            `json:"maxnodes"`
	Query    map[string]string `json:"params"`
	Nodes    QueryResults      `json:"nodes"`
	Edges    SubgraphEdges     `json:"edges"`
	Clusters map[uint64]string `json:"clusters"`
}

// HTTPOutputSubgraph reads back the output of a subgraph parsing task
func HTTPOutputSubgraph(res http.ResponseWriter, req *http.Request) {
	id, _, outfile, initfile, _, err := OutputAndLogCommonBody(res, req)

	if err != nil {
		return
	}

	f, err := os.Open(outfile)
	if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Cannot parse output for id %s (%s): %s", id, outfile, err)
		return
	}
	defer f.Close()

	// I want to parse queryID and maxnodes from the filepath
	q := Parse(initfile)
	queryID := q.Query_to_analyze
	maxnodes := q.Maxnodes
	to := TaskOutputSubgraph{
		Task:     id,
		Name:     filepath.Base(f.Name()),
		QueryID:  queryID,
		Maxnodes: fmt.Sprintf("%v", maxnodes),
		Query:    PrettyFmt(&q),
	}

	if nodes, edges, clusters, err := ParseOutputSubgraph(bufio.NewReader(f)); err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Cannot parse output for id %s (%s): %s", id, outfile, err)
		return
	} else if len(nodes) > 0 {
		to.Nodes = nodes
		to.Clusters = clusters
		if len(edges) > 0 {
			to.Edges = edges
		}
	}
	
	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(to))
}

// usage: curl -v -X POST -F "file=@/local/path/to/hello.txt" http://localhost:8080/v0/upload
// HTTPuploadFile allows to upload a file
func HTTPUploadFile(res http.ResponseWriter, req *http.Request) {
	// the FormFile function takes in the POST input id "file"
	if req.Method != http.MethodPost {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	file, header, err := req.FormFile("file")
	if err != nil {
		log.Printf("Error parsing Upload file POST: %s", err)
		res.WriteHeader(http.StatusBadRequest)
		//fmt.Fprintln(res, err)
		return
	}
	defer file.Close()

	// TODO: is this enough as a safety measure? https://medium.com/@owlwalks/dont-parse-everything-from-client-multipart-post-golang-9280d23cd4ad
	err = req.ParseMultipartForm(1 << 30) // 1Gb max size
	if err != nil {
		log.Printf("File size exceeds limits. Max size: 1Gb")
		res.WriteHeader(http.StatusUnprocessableEntity)
		return
	}

	// check if the file name is safe, using FieldSafePathRegexp defined in form.go
	if !FieldSafePathRegexp.Match([]byte(header.Filename)) {
		log.Printf("Unsafe file name")
		res.WriteHeader(http.StatusUnprocessableEntity)
		return
	}

	// check if file already exists: https://stackoverflow.com/questions/12518876/how-to-check-if-a-file-exists-in-go
	var newfilepath = filepath.Join(*repository, header.Filename)

	// for the purpose of checking the file's existence we need to remove the extension, if specified
	possible_filepath := newfilepath
	if FormIsSet(req.PostForm, "extension") {
		possible_filepath = strings.TrimSuffix(possible_filepath, req.PostForm["extension"][0]) + "txt"
	}

	if _, err := os.Stat(possible_filepath); err == nil {
		// path/to/whatever exists
		log.Printf("File already exists. Filepath: %s", possible_filepath)
		res.WriteHeader(http.StatusUnprocessableEntity)
		return

	} else if errors.Is(err, os.ErrNotExist) {
		// path/to/whatever does *not* exist -> do nothing, continue with the execution

	} else {
		// Schrodinger: file may or may not exist. See err for details.

		// Therefore, do *NOT* use !os.IsNotExist(err) to test for file existence

		log.Printf("Unable to determine file existence. Abort. Error: %s", err)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	out, err := os.Create(newfilepath)
	if err != nil {
		log.Printf("Unable to create the file for writing. Check your write access privilege")
		res.WriteHeader(http.StatusUnprocessableEntity)
		//fmt.Fprintf(res, "Unable to create the file for writing. Check your write access privilege")
		return
	}
	defer out.Close()

	// write the content from POST to the file
	_, err = io.Copy(out, file)
	if err != nil {
		res.WriteHeader(http.StatusInternalServerError)
		log.Printf("Internal error while opening new file to copy contents in: %s", err)
		//fmt.Fprintln(res, err)
		return
	}

	description := "No info available for this dataset"
	if FormIsSet(req.PostForm, "description") {
		description = req.PostForm["description"][0]
		if description == "" {
			description = "No info available for this dataset"
		}
	}

	if FormIsSet(req.PostForm, "extension") {
		switch req.PostForm["extension"][0] {
		case "csv":
			ConvertAndCreateIniFile(newfilepath, "csvToTxtConverter.py", "csv", description)
			newfilepath = strings.TrimSuffix(newfilepath, "csv") + "txt"
		case "gml":
			ConvertAndCreateIniFile(newfilepath, "gmlToTxtConverter.py", "gml", description)
			newfilepath = strings.TrimSuffix(newfilepath, "gml") + "txt"
		case "net":
			ConvertAndCreateIniFile(newfilepath, "pajekToTxtConverter.py", "net", description)
			newfilepath = strings.TrimSuffix(newfilepath, "net") + "txt"
		case "txt":
			CreateIniFile(newfilepath, description)
		}
	} else {
		// we assume that the user is uploading a file in asd format
		// log.Printf("Creating ini file for file with name: %s, path: %s", newfilepath, *repository)
		CreateIniFile(newfilepath, description)
	}


	// log.Printf("Uploaded file with name: %s, path: %s", header.Filename, newfilepath)
	log.Printf("Uploaded file with name: %s, path: %s, description: %s, extension: %s", header.Filename, newfilepath, req.PostForm["description"][0], req.PostForm["extension"][0])
	res.WriteHeader(http.StatusOK)
}

type DatasetInfo struct {
	Path string `json:"path"`
	Name string `json:"name"`
	Size string `json:"size"`
	N    string `json:"N"`
	M    string `json:"M"`
	Info string `json:"info"`
}

// HTTPDatasets reads the currently available datasets
func HTTPDatasets(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodGet {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	datasets, err := lstree([]DatasetInfo{}, *repository, true)
	if err != nil {
		log.Printf("%s", err)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(datasets))
}

func lstree(datasets []DatasetInfo, folder string, isHTTPDatasets bool) ([]DatasetInfo, error) {
	files, err := os.ReadDir(folder)
	if err != nil {
		return nil, fmt.Errorf("error listing repository content (%s) %w", folder, err)
	}

	for _, f := range files {
		if f.IsDir() {
			datasets, err = lstree(datasets, filepath.Join(folder, f.Name()), isHTTPDatasets)
			if err != nil {
				return nil, err
			}
			continue
		} else if strings.HasPrefix(f.Name(), ".") {
			// Skip hidden files
			continue
		} else if strings.HasSuffix(f.Name(), ".ini") && isHTTPDatasets {
			// Skip .ini files if we are looking at datassets folder
			continue
		}

		fstat, err := f.Info()
		if err != nil {
			return nil, err
		}
		if fstat.Size() == 0 && isHTTPDatasets {
			// Skip empty files
			continue
		}

		path := filepath.Join(strings.TrimPrefix(folder, *repository), f.Name())

		//if isHTTPDatasets: open dataset's .ini file, take new info: N, M, description
		if isHTTPDatasets {
			// remove .csv or .txt from path
			iniPath := strings.TrimSuffix(filepath.Join(folder, f.Name()), ".csv")
			iniPath = strings.TrimSuffix(iniPath, ".txt")
			iniPath = iniPath + ".ini"
			// check if .ini file exists

			if _, err := os.Stat(iniPath); err == nil {
				cfg, err := ini.Load(iniPath)
				if err != nil {
					log.Printf("Fail to read ini file in lstree: %v", err)
					return nil, err
				}

				datasets = append(datasets, DatasetInfo{
					path,
					cfg.Section("").Key("name").String(),
					cfg.Section("").Key("size").String(),
					cfg.Section("").Key("N").String(),
					cfg.Section("").Key("M").String(),
					cfg.Section("").Key("info").String(),
				})
			} else {
				datasets = append(datasets, DatasetInfo{
					path,
					f.Name(),
					"", "", "", "No info available for this dataset",
				})
			}
		} else {
			datasets = append(datasets, DatasetInfo{
				path,
				f.Name(),
				// data needed only for HTTPDatasets -> Size, N, M, info
				"", "", "", "",
			})
		}

	}
	return datasets, nil
}

type TaskInJob struct {
	Task  uuid.UUID         `json:"task"`
	Name  string            `json:"name"`
	Query map[string]string `json:"params"`
}

// HTTPAvailable returns a list of all completed jobs and their relative tasks
// TODO: control that the tasks ended succesfully
func HTTPAvailable(res http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodGet {
		res.WriteHeader(http.StatusMethodNotAllowed)
		return
	}

	jobs := map[string][]TaskInJob{}
	jobs[uuid.Nil.String()] = []TaskInJob{}

	// take all the files present in *outputDir (even the empty ones, as .log files may be empty)
	queryOutputFiles, err := lstree([]DatasetInfo{}, *outputDir, false)
	if err != nil {
		log.Printf("%s", err)
		res.WriteHeader(http.StatusInternalServerError)
		return
	}

	// TODO: implement history sorted by last modification
	// sort.Slice(queryOutputFiles, func(i, j int) bool {
	// 	stat_i, _ := os.Stat(queryOutputFiles[i].Path)
	// 	stat_j, _ := os.Stat(queryOutputFiles[j].Path)
	// 	return stat_j.ModTime().Before(stat_i.ModTime())
	// })

	var currQueryName, currQueryId, currJobName string
	var currQueryIdAndJob []string
	var currQuery Query
	// pick all queryOutputFiles's .Name's fields
	for i := 0; i < len(queryOutputFiles); i++ {
		if !strings.HasSuffix(queryOutputFiles[i].Path, ".csv") {
			continue
		}
		currQueryName = queryOutputFiles[i].Name
		currQuery = Parse(strings.TrimSuffix(queryOutputFiles[i].Path, ".csv") + ".ini")

		if !strings.Contains(currQueryName, "_") {
			// simply append to the no job category (uuid.Nil.String)
			currQueryId = strings.TrimSuffix(currQueryName, ".csv")
			currUuid, _ := uuid.Parse(currQueryId)

			_, found := status.queries[currUuid]
			if !found {
				jobs[uuid.Nil.String()] = append(jobs[uuid.Nil.String()], TaskInJob{Task: currUuid, Name: currQueryName, Query: PrettyFmt(&currQuery)})
			}
		} else {
			// task is part of a job, extract queryId and jobId
			currQueryIdAndJob = strings.SplitN(currQueryName, "_", 2)
			currQueryId = currQueryIdAndJob[0]
			currJobName = strings.TrimSuffix(currQueryIdAndJob[1], ".csv")

			currUuid, _ := uuid.Parse(currQueryId)
			_, found := status.queries[currUuid]
			if !found {
				_, ok := jobs[currJobName]
				if ok {
					// jobId present in map
					jobs[currJobName] = append(jobs[currJobName], TaskInJob{Task: currUuid, Name: currQueryName, Query: PrettyFmt(&currQuery)})
				} else {
					jobs[currJobName] = []TaskInJob{}
					jobs[currJobName] = append(jobs[currJobName], TaskInJob{Task: currUuid, Name: currQueryName, Query: PrettyFmt(&currQuery)})
				}
			}
		}
	}

	res.WriteHeader(http.StatusOK)
	res.Write(MustMarshal(jobs))
}
