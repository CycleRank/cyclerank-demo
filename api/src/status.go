package main

import (
	"log"
	"os"
	"sync"
	"io"

	"github.com/google/uuid"
)

var status *StatusRecorder

// InitStatusRecorder initialize status recording system.
func InitStatusRecorder() {
	status = &StatusRecorder{queries: map[uuid.UUID]RunningQuery{}}
}

// RunningQuery tracks the pid associated with a running query
type RunningQuery struct {
	Query string
	Pid   int
}

// StatusRecorder keeps track of everything is happening
type StatusRecorder struct {
	queries map[uuid.UUID]RunningQuery
	mutex   sync.Mutex
}

// RegisterQuery inform the status recorder about a running query.
// id: the uuid of the query
//
//	q: the string representation for the query.
func (s *StatusRecorder) RegisterQuery(id uuid.UUID, q Query, proc *os.Process) {
	s.mutex.Lock()
	defer s.mutex.Unlock()
	if rep, exists := s.queries[id]; exists {
		log.Fatalf("[uuid collision?] Different queries registered with same id %s:  (old: %s), (new: %s).", id, rep.Query, q)
	}
	s.queries[id] = RunningQuery{
		Query: q.String(),
		Pid:   proc.Pid,
	}
}

// MarkCompleted marks a query as completed by removing its entry from the status.
func (s *StatusRecorder) MarkCompleted(id uuid.UUID) {
	s.mutex.Lock()
	defer s.mutex.Unlock()
	_, found := s.queries[id]
	if !found {
		log.Fatal("Seems like a process completed a second time -_-.")
	}
	delete(s.queries, id)
}

// GetPid maps a query id to the corresponding running process pid.
func (s *StatusRecorder) GetPid(id uuid.UUID) (int, error) {
	s.mutex.Lock()
	defer s.mutex.Unlock()
	rq, exists := s.queries[id]
	if !exists {
		return 0, ErrQueryNotFound
	}
	return rq.Pid, nil
}

func (s *StatusRecorder) Dump(o io.Writer) {
	s.mutex.Lock()
	defer s.mutex.Unlock()
	o.Write(MustMarshal(s.queries))
}
