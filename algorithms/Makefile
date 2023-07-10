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

												## Generates all binaries (CycleRank, PageRank, Single-Source PR, 2d Rank, Sigle-Source 2dR, CheiRank, Single-Source CR)
all: cyclerank pr ssppr	2dr 2drp cheir cheirp subgraphGenerator	

clean:											## Remove generated binary and object files
	rm -f bin/cyclerank bin/pr bin/ssppr bin/2drp bin/2dr bin/cheir bin/cheirp bin/subgraphGenerator bin/looprank_old

cyclerank: src/cyclerank.cpp					## Compile CycleRank

	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/cyclerank.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/cyclerank

looprank_old: src/looprank_old.cpp				## Compile LoopRank (deprecated)

	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/looprank_old.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/looprank_old

pr: src/pr.cpp									## Compile PageRank
	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/pr.cpp \
			${CFLAGS} \
			$(LFLAGS) \
			-o bin/pr

ssppr: src/ssppr.cpp							## Compile Single-Source PageRank
	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/ssppr.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/ssppr

2dr: src/2dr.cpp								## Compile 2dRank

	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/2dr.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/2dr

2drp: src/2drp.cpp								## Compile Single-Source 2dRank

	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/2drp.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/2drp

cheir: src/cheir.cpp							## Compile CheiRank

	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/cheir.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/cheir

cheirp: src/cheirp.cpp							## Compile Single-Source CheiRank

	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/cheirp.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/cheirp

subgraphGenerator: src/subgraphGenerator.cpp	## Compile subgraphGenerator

	${CXX} -Wall -Werror -std=c++11 -O2 -flto -pipe \
			src/subgraphGenerator.cpp \
			${CFLAGS} \
			${LFLAGS} \
			-o bin/subgraphGenerator

help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		sort | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
