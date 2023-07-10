package main

import (
	"flag"
	"log"
	"net/http"
	"path"
	"path/filepath"
)

var (
	addr       = flag.String("addr", ":8080", "[ip]:port")
	repository = flag.String("repository", "data", "Dataset repository path")
	outputDir  = flag.String("output_dir", "output", "Output directory path")
)

func main() {
	flag.Parse()

	repository = EnsureAbsPathOrDie(repository)
	outputDir = EnsureAbsPathOrDie(outputDir)

	log.Printf("Repository %s", *repository)
	log.Printf("Output dir %s", *outputDir)

	InitStatusRecorder()
	RegisterAPI()

	// http.Handle("/", http.FileServer(http.Dir("./ui")))
	log.Printf("Serving requests @ %s", *addr)
	log.Fatal(http.ListenAndServe(*addr, nil))
}

// EnsureAbsPathOrDie makes sure a path is absolute or makes it so.
func EnsureAbsPathOrDie(p *string) *string {
	if path.IsAbs(*p) {
		return p
	}

	np, err := filepath.Abs(*p)
	if err != nil {
		log.Fatalf("Cannot get absolute path for %s", *p)
	}
	return &np
}
