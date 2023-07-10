#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <queue>
#include <stack>
#include <list>
#include <map>
#include <limits>
#include <climits>
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <string>

extern "C" {
   #include <igraph.h>
}

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

using namespace std;
namespace spd = spdlog;
namespace opts = cxxopts;

// ***************************************************************************
struct nodo{
  bool active;
  bool blocked;
  int dist;
  vector<int> adj;
  list<int> B;

  nodo(){
    active = true;
    blocked = false;
    dist = -1;
    B.clear();
  }
};


// *************************************************************************
// global variables
shared_ptr<spd::logger> console;

// *************************************************************************
// helper functions

// helper function to remove multiple chars from string https://stackoverflow.com/questions/5891610/how-to-remove-certain-characters-from-a-string-in-c
bool IsNewlineOrTab(char c)
{
    switch(c)
    {
    case '\t':
    case '\r':
    case '\n':
        return true;
    default:
        return false;
    }
}

// count the number of parameters in the first line of the file
// https://stackoverflow.com/a/34665370/2377454
int count_parameters(ifstream& in) {

  char tmp_c, next_c;
  int firstline_count = 0;
  string line;

  // read first line of file
  getline(in, line);

  //stringstream ss(line);

  // read numbers from the line
  tmp_c = line[0];
  for (int i = 1; i < (int)line.length(); i++) {
    //console->debug("tmp_c: {}", tmp_c);
    next_c = line[i];
    if (tmp_c == ' ' && (next_c != ' ' || next_c != '\n')) {
      ++firstline_count;
    }
    tmp_c = next_c;
  }

  //we count every space between the first input line's data, without counting double spaces and spaces after the last input (e.g. evey space before \n)
  //as such, the number of parameters of the first line of the file equals number of spaces counted + 1.
  ++firstline_count;

  console->debug("firstline_count: {}", firstline_count);

  return firstline_count;
}
// ********** end: helper functions


// *************************************************************************

int main(int argc, char* argv[]) {

  // *************************************************************************
  // parse command-line options
  opts::Options* options;
  string input_file="input.txt";
  string output_file="";
  string log_file="";
  double alpha=0.85;
  bool verbose = false;
  bool debug = false;
  bool help = false;
  bool directed = true;

  try {
    options = new cxxopts::Options(argv[0]);

    options->add_options()
      ("a,alpha", "Damping factor (alpha).",
       cxxopts::value<double>(alpha),
       "ALPHA"
       )
      ("f,file", "Input file.",
       cxxopts::value<string>(input_file),
       "FILE"
       )
      ("v,verbose", "Enable logging at verbose level.",
       cxxopts::value(verbose))
      ("d,debug", "Enable logging at debug level.",
       cxxopts::value(debug))
      ("h,help", "Show help message and exit.",
       cxxopts::value(help))
      ("l,log", "Logfile [default: stderr].",
       cxxopts::value<string>(log_file),
       "LOG_FILE"
       )
      ("o,output", "Output file.",
       cxxopts::value<string>(output_file),
       "OUTPUT_FILE"
       );

    auto arguments = options->parse(argc, argv);
  } catch (const cxxopts::OptionException& e) {
    cerr << "Error parsing options: " << e.what() << endl;
    exit (EXIT_FAILURE);
  }

  // if help option is activated, print help and exit.
  if(help) {
    cout << options->help({""}) << endl;
    exit(0);
  }

  if(alpha <= 0) {
    cerr << "Error: the damping factor specified with -a (alpha) must be" \
         << "positive." << endl;
    exit (EXIT_FAILURE);
  }
  // ********** end: parse command-line options

  // *************************************************************************
  // initialize logger
  try {
    if (log_file.empty()) {
      // by default, log using the standard console (outputs on stderr)
      console = spd::stderr_color_mt("console");
      console->debug("log file is empty");
    } else {
      //basic_logger appends inside the log file. For clarity, we delete the previous information.
      console = spd::basic_logger_mt("console", log_file);
      console->debug("log file is not empty: {}", input_file);
    }
  }
  // exceptions thrown upon failed logger init
  catch (const spd::spdlog_ex& ex) {
    cerr << "Log init failed: " << ex.what() << endl;
    return 1;
  }
  // ********** end: logger

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
  console->debug("alpha: {}", alpha);
  // ********** end: start logging

  // *************************************************************************
  // start algorithm
  unsigned int N = 0, M = 0;
  vector<nodo> grafo;

  //map that associates old nodes to the labels given by the user
  map<int,string> old2label;

  // *************************************************************************
  // read input
  {
    ifstream in(input_file);
    int nparam = 0;

    if(in.fail()){
      cerr << "Error! Could not open file: " << input_file << endl;
      exit(EXIT_FAILURE);
    }

    nparam = count_parameters(in);
    in.close();

    in.open(input_file);
    if(nparam == 4) {
      // pr does not need 4 parameters (S and K), but adding this edge case allows one to use a single file for all algorithms
      string tmpS = "";
      unsigned int tmpK = 0;
      in >> N >> M >> tmpS >> tmpK;
    } else if(nparam == 2) {
      in >> N >> M;
    } else {
      cerr << "Error! Error while reading file (" << input_file \
          << "), unexpected number of parameters" << endl;
      exit(EXIT_FAILURE);
    }


    assert( (N > 0 && M > 0) \
            && "N and M must be positive." );

    console->info("N: {}", N);
    console->info("M: {}", M);

    console->debug("reading graph...");
    grafo.resize(N);
    for(unsigned int j=0; j<M; j++) {
      int s, t;
      in >> s >> t;

      // check that we are not inserting duplicates
      if (find(grafo[s].adj.begin(), \
               grafo[s].adj.end(), \
               t) == grafo[s].adj.end()) {
        grafo[s].adj.push_back(t);
      }
    }
    console->debug("--> read graph");

    console->debug("reading labels...");

    unsigned int mapped_node = -1;
    string label;
    
    while(in >> mapped_node) {
      // trim whitespace before label
      getline(in >> std::ws, label);
      label.erase(std::remove_if(label.begin(), label.end(), &IsNewlineOrTab), label.end());
      console->debug("mapped node: {}", mapped_node);
      console->debug("label: {}", label);

      assert( (mapped_node < N) \
              && "the node to be labeled must be within 0 and N-1." );
      
      //TODO: use a constant here
      assert( (label.size() <= 256) \
              && "the label's size must be less or equal than 256 characters." );

      console->debug("Mapped node is: {}", mapped_node);
      assert( (!label.empty()) \
              && "the label must not be empty." );
      old2label.insert(pair<int,string>(mapped_node, label));
    }
    
    console->debug("--> read labels");

    in.close();
  }
  // ********** end: read input

  /* *************************************************************************
  * Pagerank from igraph
  *   https://igraph.org/c/doc/igraph-Structural.html#igraph_pagerank
  *
  * int igraph_pagerank(
  *   const igraph_t *graph,
  *   igraph_pagerank_algo_t algo,
  *   igraph_vector_int_t *vector,
  *   igraph_real_t *value, const igraph_vs_t vids,
  *   igraph_bool_t directed, igraph_real_t damping, 
  *   const igraph_vector_t *weights, void *options
  *   );
  * *************************************************************************/
  console->info("Pagerank (alpha={})", alpha);

  igraph_t igrafo;
  igraph_vector_int_t iedges;
  igraph_real_t pr_alpha(alpha);

  console->debug("Calculating the Pagerank on the graph: ");
  console->debug("  * on the input graph");

  console->debug("  * on the directed graph");

  // count number of edges
  unsigned int num_edges = 0;
  for(unsigned int i=0; i<grafo.size(); i++) {
    num_edges += grafo[i].adj.size();
  }

  console->debug("num_edges: {0}", num_edges);

  // initialize vertices vector 2*num_edges
  igraph_vector_int_init(&iedges, 2*num_edges);

  int ec = 0;
  for(unsigned int i=0; i<grafo.size(); i++) {
    for (int v: grafo[i].adj) {
      VECTOR(iedges)[ec]=i;
      VECTOR(iedges)[ec+1]=v;

      ec = ec + 2;
    }
  }

  // we get the size of the graph and the we clear
  unsigned int num_nodes = grafo.size();
  grafo.clear();

  // int igraph_create(igraph_t *graph, const igraph_vector_int_t *edges,
  //   igraph_integer_t n, igraph_bool_t directed);
  igraph_create(&igrafo, &iedges, num_nodes, directed);

  igraph_vector_t prscore;

  // init result vector
  igraph_vector_init(&prscore,  0);

  /*
  * int igraph_pagerank(
  *   const igraph_t *graph,
  *   igraph_pagerank_algo_t algo,
  *   igraph_vector_t *vector,
  *   igraph_real_t *value,
  *   const igraph_vs_t vids,
  *   igraph_bool_t directed, 
  *   igraph_real_t damping, 
  *   const igraph_vector_t *weights,
  *   void *options);
  *
  *
  * graph:    The graph object.
  * algo:     The PageRank implementation to use {IGRAPH_PAGERANK_ALGO_POWER,
              IGRAPH_PAGERANK_ALGO_ARPACK, IGRAPH_PAGERANK_ALGO_PRPACK}.
  *           IGRAPH_PAGERANK_ALGO_PRPACK is the recommended implementation
  *           http://igraph.org/c/doc/igraph-Structural.html#igraph_pagerank_algo_t
  * vector:   Pointer to an initialized vector, the result is stored here.
  *           It is resized as needed.
  * value:    Pointer to a real variable, the eigenvalue corresponding to the
              PageRank vector is stored here. It should be always exactly one.
  * vids:     The vertex ids for which the PageRank is returned.
  * directed: Boolean, whether to consider the directedness of the edges.
              This is ignored for undirected graphs.
  * damping:  The damping factor ("d" in the original paper)
  * weights:  Optional edge weights, it is either a null pointer, then the
              edges are not weighted, or a vector of the same length as the
              number of edges.
  * options:  Options to the algorithms.
  *
  */

  int ret = -1;
  ret=igraph_pagerank(
     &igrafo,                         // const igraph_t *graph
     IGRAPH_PAGERANK_ALGO_PRPACK,     // igraph_pagerank_algo_t algo
     &prscore,                        // igraph_vector_t *vector
     0,                               // igraph_real_t *value
     igraph_vss_all(),                // const igraph_vs_t vids
     directed,                        // igraph_bool_t directed
     pr_alpha,                        // igraph_real_t damping
     0,                               // const igraph_vector_t *weights,
     0                                // void *options
     );
  console->debug("PR ret: {0:d}", ret);

  igraph_destroy(&igrafo);

  FILE *outfp = stdout;
  if (!output_file.empty()) {
    outfp = fopen(output_file.c_str(), "w+");
  }

  map<float, int, greater<float>> orderedResults;

  for (unsigned int i=0; i<num_nodes; i++) {
    if (VECTOR(prscore)[i] != 0.0) {
      orderedResults.insert(pair<float, int>(VECTOR(prscore)[i], i));
    }
  }

  for (const auto& node: orderedResults) {
    int i = node.second;
    if (old2label.find(i) == old2label.end()) {
      //node not found in label system
      fprintf(outfp, "%d\t%d\t%.10e\n", i, i, VECTOR(prscore)[i]);
    }
    else {
      //node is present in label system
      fprintf(outfp, "%d\t%s\t%.10e\n", i, old2label[i].c_str(), VECTOR(prscore)[i]);
    }
  }

  // for (unsigned int i=0; i<num_nodes; i++) {
  //   if (VECTOR(prscore)[i] != 0.0) {
  //     if (old2label.find(i) == old2label.end()) {
  //       //node not found in label system
  //       fprintf(outfp, "%d\t%d\t%.10f\n", i, i, VECTOR(prscore)[i]);
  //     }
  //     else {
  //       //node is present in label system
  //       fprintf(outfp, "%d\t%s\t%.10f\n", i, old2label[i].c_str(), VECTOR(prscore)[i]);
  //     }
  //   }
  // }

  console->info("Log stop!");
  exit (EXIT_SUCCESS);
}
