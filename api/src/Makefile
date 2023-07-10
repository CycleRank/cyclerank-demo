.PHONY: default help clean

.DEFAULT_GOAL := help

LFLAGS=-pthread $$(pkg-config --libs igraph arpack) -lgomp
CFLAGS=-Ilibs $$(pkg-config --cflags igraph)

detected_OS := Unknown
ifeq ($(OS),Windows_NT)    						# Windows
	detected_OS := Windows
else
	detected_OS := $(shell uname)	# same as "uname -s"
endif


all: cyclerank-api								## Generates all

clean:											## Remove generated binary
	rm -f cyclerank-api

cyclerank-api:									## Generates the cyclerank-api binary
	go build .

help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		sort | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
