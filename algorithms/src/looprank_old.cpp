#include <algorithm>
#include <cassert>
#include <limits>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <stack>
#include <limits>
#include <climits>
#include <stdlib.h> /* exit, EXIT_FAILURE */
#include <utility>
#include <vector>

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

using namespace std;
namespace spd  = spdlog;
namespace opts = cxxopts;

// ***************************************************************************
struct nodo {
  bool        active;
  bool        instack;
  bool        blocked;
  int         dist;
  int         index;
  int         low;
  double      score;
  vector<int> adj;
  list<int>   B;

  nodo() {
    active  = true;
    instack = false;
    blocked = false;
    dist    = -1;
    index   = -1;
    low     = INT_MAX;
    score   = 0.0;
    B.clear();
  }
};

// *************************************************************************
// global variables
shared_ptr<spd::logger> console;

vector<stack<int>> cycles;
stack<int>         tarjan_st;
int                counter = 0;

stack<int> circuits_st;

// *************************************************************************
// helper functions
void print_g(vector<nodo> &g) {
  for (unsigned int i = 0; i < g.size(); i++) {
    if (g[i].active) {
      for (int v : g[i].adj) {
        console->debug("{0:d} -> {1:d}", i, v);
      }
    }
  }

  return;
}

void print_stack(stack<int> s) {
  printf("(print_stack) stack size: %d\n", (int)s.size());
  while (!s.empty()) {
    printf("%d", s.top());
    s.pop();
  }
  printf("\n--- (print_stack) ---\n");
}

void print_circuit(stack<int> s, vector<int> &new2old) {
  vector<int> tmp;

  printf("--> cycle: ");
  while (!s.empty()) {
    int el    = s.top();
    int oldel = new2old[el];
    tmp.push_back(oldel);
    s.pop();
  }

  for (int j = tmp.size() - 1; j >= 0; j--) {
    printf("%d-", tmp[j]);
  }
  int last = tmp[tmp.size() - 1];
  printf("%d\n", last);
}

/*
void print_cycles() {
  for (auto& st: cycles) {
    print_circuit(st);
  }
}
*/

// count the number of parameters in the first line of the file
// https://stackoverflow.com/a/34665370/2377454
int count_parameters(ifstream &in) {

  int    tmp_n;
  int    firstline_count = 0;
  string line;

  // read first line of file
  getline(in, line);

  stringstream ss(line);

  // read numbers from the line
  while (ss >> tmp_n) {
    ++firstline_count;
  }

  console->debug("firstline_count: {}", firstline_count);

  return firstline_count;
}

// ********** end: helper functions

// *************************************************************************
void destroy_nodes(vector<nodo> &g, vector<bool> &destroy) {

  for (unsigned int i = 0; i < g.size(); i++) {
    if (destroy[i]) {
      g[i].active = false;
      g[i].adj.clear();
    } else {
      for (unsigned int j = 0; j < g[i].adj.size(); j++) {
        int v = g[i].adj[j];
        if (destroy[v]) {
          g[i].adj.erase(g[i].adj.begin() + j);
        }
      }
    }
  }
}

void bfs(int source, int K, vector<nodo> &g) {

  if (!g[source].active) {
    return;
  }

  for (nodo &n : g) {
    n.dist = -1;
  }

  g[source].dist = 0;

  queue<int> q;
  q.push(source);
  int cur;
  while (!q.empty()) {
    cur = q.front();
    q.pop();

    // if g[cur].dist == K-1 we can stop
    if (g[cur].dist > K - 2) {
      continue;
    }

    for (int v : g[cur].adj) {
      if ((g[v].dist == -1) and (g[v].active)) {

        // neighbor not yet visited, set distance
        g[v].dist = g[cur].dist + 1;
        q.push(v);
      }
    }
  }
}

void unblock(int u, vector<nodo> &g) {
  // printf("      ----> unblock(%d)\n", u);
  g[u].blocked = false;
  // printf("        ----> g[%d].blocked: %s\n", u, g[u].blocked ? "true" :
  // "false");

  while (!g[u].B.empty()) {
    int w = g[u].B.front();
    g[u].B.pop_front();

    if (g[w].blocked) {
      unblock(w, g);
    }
  }
}

int  count_calls = 0;
bool circuit(int v, int S, unsigned int K, vector<nodo> &g) {
  bool flag = false;

  if (!(circuits_st.size() > K - 1)) {
    count_calls++;

    circuits_st.push(v);

    g[v].blocked = true;

    // console->debug("sizeof circuits_st {}", (sizeof circuits_st));

    for (int w : g[v].adj) {
      if (w == S) {
        if (!(circuits_st.size() > K)) {
          cycles.push_back(circuits_st);
        }
        flag = true;
      } else if (!g[w].blocked) {
        if (circuit(w, S, K, g)) {
          flag = true;
        }
      }
    }

    if (flag) {
      unblock(v, g);
    } else {
      for (int w : g[v].adj) {
        auto it = find(g[w].B.begin(), g[w].B.end(), v);
        if (it == g[w].B.end()) {
          g[w].B.push_back(v);
        }
      }
    }

    circuits_st.pop();
  }

  return flag;
}

int main(int argc, char *argv[]) {

  // *************************************************************************
  // initialize logger
  try {
    console = spd::stderr_color_mt("console");
  }
  // exceptions thrown upon failed logger init
  catch (const spd::spdlog_ex &ex) {
    cerr << "Log init failed: " << ex.what() << endl;
    return 1;
  }
  // ********** end: logger

  // *************************************************************************
  // parse command-line options
  opts::Options *options;
  string         input_file  = "input.txt";
  string         output_file = "";
  int            cliS        = -1;
  int            cliK        = -1;
  bool           verbose     = false;
  bool           debug       = false;
  bool           help        = false;

  try {
    options = new cxxopts::Options(argv[0]);

    options->add_options()("f,file", "Input file.",
                           cxxopts::value<std::string>(input_file), "FILE")(
        "v,verbose", "Enable logging at verbose level.",
        cxxopts::value(verbose))("d,debug", "Enable logging at debug level.",
                                 cxxopts::value(debug))(
        "h,help", "Show help message and exit.", cxxopts::value(help))(
        "k,maxloop", "Set max loop length (K).", cxxopts::value(cliK),
        "K")("o,output", "Output file.", cxxopts::value<string>(output_file),
             "OUTPUT_FILE")("s,source", "Set source node (S).",
                            cxxopts::value(cliS), "S");

    auto arguments = options->parse(argc, argv);
  } catch (const cxxopts::OptionException &e) {
    cerr << "error parsing options: " << e.what() << endl;
    exit(EXIT_FAILURE);
  }

  // if help option is activated, print help and exit.
  if (help) {
    cout << options->help({""}) << endl;
    exit(0);
  }
  // ********** end: parse command-line options

  // *************************************************************************
  // start logging
  // set logging level based on option from CLI
  if (debug) {
    spd::set_level(spd::level::debug);
  } else if (verbose) {
    spd::set_level(spd::level::info);
  } else {
    spd::set_level(spd::level::warn);
  }

  console->info("Log start!");
  console->debug("input_file: {}", input_file);
  console->debug("verbose: {}", verbose);
  console->debug("debug: {}", debug);
  // ********** end: start logging

  // *************************************************************************
  // start algorithm
  int          N = -1, M = -1, S = -1, K = -1;
  vector<nodo> grafo;

  map<int, int> old2new;
  vector<int>   new2old;

  int count_destroied = 0;

  // *************************************************************************
  // read input
  {
    ifstream in(input_file);
    int      tmpS   = -1;
    int      tmpK   = -1;
    int      nparam = 0;

    if (in.fail()) {
      cerr << "Error! Could not open file: " << input_file << endl;
      exit(EXIT_FAILURE);
    }

    nparam = count_parameters(in);
    in.close();

    in.open(input_file);
    if (nparam == 4) {
      in >> N >> M >> tmpS >> tmpK;
    } else if (nparam == 2) {
      in >> N >> M;
    } else {
      cerr << "Error! Error while reading file (" << input_file
           << "), unexpected number of parameters" << endl;
      exit(EXIT_FAILURE);
    }

    if (cliS == -1) {
      S = tmpS;
    } else {
      S = cliS;
    }

    if (cliK == -1) {
      K = tmpK;
    } else {
      K = cliK;
    }

    assert((N > 0 && M > 0) && "N and M must be positive.");

    assert((K > 0 && S >= 0) &&
           "K must be positive and S must be non-negative.");

    console->info("N: {}", N);
    console->info("M: {}", M);
    console->info("S: {}", S);
    console->info("K: {}", K);

    console->debug("reading graph...");
    grafo.resize(N);
    for (int i = 0; i < M; i++) {
      int s, t;
      in >> s >> t;

      // check that we are not inserting duplicates
      if (find(grafo[s].adj.begin(), grafo[s].adj.end(), t) ==
          grafo[s].adj.end()) {
        grafo[s].adj.push_back(t);
      }
    }
    console->debug("--> read graph");
    in.close();

    // print_g(grafo);
    // console->debug("---\n");
  }
  // ********** end: read input

  // *************************************************************************
  // Step 1: BFS on g
  {
    console->info("Step 1. BFS");
    vector<bool> destroy(N, false);

    bfs(S, K, grafo);

    for (int i = 0; i < N; i++) {
      // se il nodo si trova distanza maggiore di K-1 o non raggiungibile
      if ((grafo[i].dist == -1) or (grafo[i].dist > K - 1)) {
        destroy[i] = true;
        count_destroied++;
      }
    }

    int remaining = N - count_destroied;
    console->info("nodes: {}", N);
    console->info("destroyed: {}", count_destroied);
    console->info("remaining: {}", remaining);
    new2old.resize(remaining);

    int newindex = -1;
    for (int i = 0; i < N; i++) {
      if (!destroy[i]) {
        newindex++;
        new2old[newindex] = i;
        old2new.insert(pair<int, int>(i, newindex));
        /*
        console->debug("old2new.insert(pair<int,int>({0}, {1}))",
                       i,
                       newindex);
        */
      }
    }

    if (debug) {
      console->debug("index map (1)");
      console->debug("old2new.size() is {}", old2new.size());

      int c = 0;
      for (auto const &pp : old2new) {
        console->debug("{0:d} => {1:d}, {2:d} => {3:d}", pp.first, pp.second, c,
                       new2old[c]);
        c++;
      }
      console->debug("~~~");
    }

    destroy_nodes(grafo, destroy);

    vector<nodo> tmpgrafo;
    tmpgrafo.resize(remaining);

    int newi = -1;
    int newv = -1;

    for (int i = 0; i < N; i++) {
      if (grafo[i].active) {

        if (old2new.find(i) == old2new.end()) {
          // not found
          cerr << "Key " << i << " (i) not found in map old2new" << endl;
          exit(EXIT_FAILURE);
        } else {
          // found
          newi = old2new[i];
        }

        tmpgrafo[newi].dist   = grafo[i].dist;
        tmpgrafo[newi].active = true;
        for (int v : grafo[i].adj) {
          if (old2new.find(v) == old2new.end()) {
            // not found
            console->debug(
                "Key {} (v) not found in map old2new - destroy[{}]: {}", v, v,
                (destroy[v] ? "true" : "false"));
            continue;

          } else {
            // found
            newv = old2new[v];
            console->debug("destroy[{0:d}]: {}", v, destroy[v]);
          }
          tmpgrafo[newi].adj.push_back(newv);
        }
      }
    }

    grafo.clear();
    grafo.swap(tmpgrafo);

    destroy.clear();
  }
  // ********** end: Step 1

  if (debug) {
    // print_g(grafo);
    console->debug("*** end print_g(grafo) ***");
  }

  vector<bool> destroy(grafo.size(), false);
  int          newS = -1;
  if (old2new.find(S) == old2new.end()) {
    // Key S not found
    cerr << "Key " << S << " (S) not found in map old2new" << endl;
    exit(EXIT_FAILURE);
  } else {
    // Key S found
    newS = old2new[S];
  }

  console->info("S: {0}, newS: {1}", S, newS);

  // *************************************************************************
  // Step 2: BFS on g^T
  {
    console->info("Step 2.: BFS on g^T");
    vector<nodo> grafoT;
    grafoT.resize(grafo.size());

    for (unsigned int i = 0; i < grafo.size(); i++) {
      for (int v : grafo[i].adj) {
        grafoT[v].adj.push_back(i);
      }
    }

    if (debug) {
      // print_g(grafoT);
      console->debug("*** end print_g(grafoT) ***");
    }

    bfs(newS, K, grafoT);

    for (unsigned int i = 0; i < grafo.size(); i++) {
      if ((grafo[i].dist == -1) or (grafoT[i].dist == -1) or
          (grafo[i].dist + grafoT[i].dist > K)) {
        // console->debug("destroied node: {0:d}\n", i);
        destroy[i] = true;
        count_destroied++;
      }
    }

    int remaining = N - count_destroied;

    console->info("nodes: {}", N);
    console->info("destroyed: {}", count_destroied);
    console->info("remaining: {}", remaining);

    destroy_nodes(grafo, destroy);
    destroy.clear();
  }
  // ********** end: Step 2

  int           remaining = N - count_destroied;
  map<int, int> tmp_old2new;
  vector<int>   tmp_new2old;
  tmp_new2old.resize(remaining);

  {
    int newindex = -1;
    int oldi     = -1;
    for (unsigned int i = 0; i < grafo.size(); i++) {
      if (!destroy[i]) {
        newindex++;

        oldi = new2old[i];

        console->debug("newindex: {}", newindex);
        console->debug("i: {} - oldi: {}", i, oldi);

        tmp_new2old[newindex] = oldi;
        tmp_old2new.insert(pair<int, int>(oldi, newindex));
        console->debug("tmp_new2old[{0}]: {1}", newindex,
                       tmp_new2old[newindex]);
        console->debug("tmp_old2new.insert(pair<int,int>({0}, {1}))", oldi,
                       newindex);
      }
    }
  }

  if (debug) {
    console->debug("*** tmp maps ***");
    console->debug("tmp_old2new, tmp_new2old");
    console->debug("tmp_old2new.size() is {}", tmp_old2new.size());

    int c = 0;
    for (auto const &pp : tmp_old2new) {
      console->debug("{0:d} => {1:d}, {2:d} => {3:d}", pp.first, pp.second, c,
                     tmp_new2old[c]);
      c++;
    }

    console->debug("*** maps BBB ***");
    console->debug("old2new.size() is {}", old2new.size());
    console->debug("old2new, new2old");
    c = 0;
    for (auto const &pp : old2new) {
      console->debug("{0:d} => {1:d}, {2:d} => {3:d}", pp.first, pp.second,
                     pp.second, new2old[pp.second]);
      c++;
    }
    console->debug("~~~");

    console->debug("*** 1 ***");
  }

  {
    int oldi = -1, tmpnewi = -1;
    int oldv = -1, tmpnewv = -1;

    vector<nodo> tmpgrafo;
    tmpgrafo.resize(remaining);
    for (unsigned int i = 0; i < grafo.size(); i++) {
      if (grafo[i].active) {

        oldi = new2old[i];
        if (tmp_old2new.find(oldi) == tmp_old2new.end()) {
          // not found
          cerr << "Key " << oldi << " (oldi) not found in map old2new" << endl;
          exit(EXIT_FAILURE);
        } else {
          // found
          tmpnewi = tmp_old2new[oldi];
        }
        console->debug("i: {}, oldi: {}, tmpnewi: {}", i, oldi, tmpnewi);

        tmpgrafo[tmpnewi].dist   = grafo[i].dist;
        tmpgrafo[tmpnewi].active = true;
        for (int v : grafo[i].adj) {

          oldv = new2old[v];
          if (tmp_old2new.find(oldv) == tmp_old2new.end()) {
            // not found
            cerr << "Key " << oldv << " (oldv) not found in map old2new"
                 << endl;
            continue;
          } else {
            // found
            tmpnewv = tmp_old2new[oldv];
          }
          console->debug("v: {}, oldv: {}, tmpnewv: {}", v, oldv, tmpnewv);

          tmpgrafo[tmpnewi].adj.push_back(tmpnewv);
        }
      }
    }

    grafo.clear();
    grafo.swap(tmpgrafo);
  }

  new2old.clear();
  old2new.clear();

  tmp_new2old.swap(new2old);
  tmp_old2new.swap(old2new);

  if (debug) {
    console->debug("map indexes");
    console->debug("old2new.size() is {}", old2new.size());

    for (auto const &pp : old2new) {
      console->debug("{0:d} => {1:d}, {2:d} => {3:d}", pp.first, pp.second,
                     pp.second, new2old[pp.second]);
    }
    console->debug("~~~");
  }

  // print_g(grafo);
  console->debug("---");

  newS = old2new[S];
  console->info("S: {0}, newS: {1}", S, newS);
  console->debug("calling circuit()");
  circuit(newS, newS, K, grafo);
  console->debug("count_calls: {}", count_calls);
  console->debug("called circuit()");

  console->debug("***");
  console->debug("count_calls: {0}", count_calls);
  console->debug("# of cycles found: {0}", cycles.size());

  for (auto &c : cycles) {
    int csize = c.size();
    // print_circuit(c, new2old);
    while (!c.empty()) {
      int el = c.top();
      c.pop();

      console->debug("{} <- {}", el, 1.0 / csize);

      grafo[el].score += 1.0 / csize;
    }
  }

  console->debug("---");

  // This is the result display loop.
  FILE *outfp = stdout;
  if (!output_file.empty()) {
    outfp = fopen(output_file.c_str(), "w+");
  }

  for (unsigned int i = 0; i < grafo.size(); i++) {
    if (grafo[i].score != 0.0) {
      int oldi = new2old[i];
      fprintf(outfp, "%d\t%.10f\n", oldi, grafo[i].score);
    }
  }

  console->info("Log stop!");
  exit(EXIT_SUCCESS);
}
