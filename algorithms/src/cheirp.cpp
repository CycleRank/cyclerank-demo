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
#include <set>

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

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
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
void transpose_graph(vector<nodo>& g, vector<nodo>& gT) {
  gT.resize(g.size());

  for(unsigned int i=0; i<g.size(); i++) {
    for (int v: g[i].adj) {
      gT[v].adj.push_back(i);
    }
  }
}

int main(int argc, char* argv[]) {

  // *************************************************************************
  // parse command-line options
  opts::Options* options;
  string input_file="input.txt";
  string output_file="";
  string log_file="";
  string cliS = "";
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
       )
      ("s,source", "Set source node (S).",
       cxxopts::value<string>(cliS),
       "S"
       )
      ;

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
  int S = -1, newS = -1;
  unsigned int N = 0, M = 0;
  vector<nodo> grafo;

  map<int,int> old2new;
  //map that associates old nodes to the labels given by the user
  map<int,string> old2label;
  vector<int> new2old;

  // *************************************************************************
  // read input
  {
    ifstream in(input_file);
    //int tmpS = -1;
    string tmpS = "";
    int nparam = 0;

    if(in.fail()){
      cerr << "Error! Could not open file: " << input_file << endl;
      exit(EXIT_FAILURE);
    }

    nparam = count_parameters(in);
    in.close();

    in.open(input_file);
    if(nparam == 4) {
      // K is not used for single source page rank
      unsigned int tmpK = 0;
      in >> N >> M >> tmpS >> tmpK;
    } else if(nparam == 2) {
      in >> N >> M;
    } else {
      cerr << "Error! Error while reading file (" << input_file \
          << "), unexpected number of parameters" << endl;
      exit(EXIT_FAILURE);
    }

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

    if(cliS == "") {
      //cliS is empty, therefore we must use tmpS (souce node given via input file)
      
      //we must control whether or not the passed source node is a label

      //foundNode is the node mapped to tmpS's value
      int foundNode = 0;
      auto findResult = std::find_if(std::begin(old2label), std::end(old2label), [&](const std::pair<int, string> &pair) {
            return pair.second == tmpS;
      });
      if (findResult != std::end(old2label)) {
            foundNode = findResult->first;
            S = foundNode;
            console->debug("S is a label and it has been found");
      }
      else if (is_number(tmpS) && stoi(tmpS) < (int)N) {
        S = stoi(tmpS);
      }
      else{
        //if tmpS is not a number, then it must have failed the label map check, therefore it is an invalid label
        assert( (is_number(tmpS)) \
                && "label given for source node is invalid." );
        //if tmpS is a number but exceeds N, then it is a node, but its value is not registered in the graph
        assert( (stoi(tmpS) < (int)N) \
                && "S must be less or equal than N." );
      }

    } else {
      //cliS is not empty
      
      //we must control whether or not the passed source node is a label

      //foundNode is the node mapped to cliS's value
      int foundNode = 0;
      auto findResult = std::find_if(std::begin(old2label), std::end(old2label), [&](const std::pair<int, string> &pair) {
            return pair.second == cliS;
      });
      if (findResult != std::end(old2label)) {
            foundNode = findResult->first;
            S = foundNode;
            console->debug("S is a label and it has been found");
      }
      else if (is_number(cliS) && stoi(cliS) < (int)N) {
        S = stoi(cliS);
      }
      else{
        //if cliS is not a number, then it must have failed the label map check, therefore it is an invalid label
        assert( (is_number(cliS)) \
                && "label given for source node is invalid." );
        //if cliS is a number but exceeds N, then it is a node, but its value is not registered in the graph
        assert( (stoi(cliS) < (int)N) \
                && "S must be less or equal than N." );
      }
    }

    console->info("N: {}", N);
    console->info("M: {}", M);
    console->info("S: {}", S);

    assert( (N > 0 && M > 0) \
            && "N and M must be positive." );

    assert( (S >= 0) \
            && "S must be non-negative." );

    in.close();
  }
  // ********** end: read input

  newS = S; // no remap shenanigans happen

  /* *************************************************************************
  * Personalized pagerank from igraph
  *   http://igraph.org/c/doc/
  *       igraph-Structural.html#igraph_personalized_pagerank
  *
  * int igraph_personalized_pagerank(
  *    const igraph_t *graph, 
  *    igraph_pagerank_algo_t algo, igraph_vector_t *vector,
  *    igraph_real_t *value, const igraph_vs_t vids,
  *    igraph_bool_t directed, igraph_real_t damping, 
  *    igraph_vector_t *reset,
  *    const igraph_vector_t *weights,
  *    void *options);
  * *************************************************************************/
  console->info("Personalized CheiRank (alpha={})", alpha);

  igraph_t igrafo;
  igraph_vector_int_t iedges;
  vector<nodo> grafoT;
  vector<nodo> grafoU;
  igraph_real_t pr_alpha(alpha);
  igraph_vector_t cheirpscore, reset;
  unsigned int num_nodes;

    console->debug("Calculating the Personalized CheiRank on the graph: ");
    transpose_graph(grafo, grafoT);
    grafo.swap(grafoT);
  
    // deallocate vector (of nodes)
    grafoT.clear();
    
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
  
    // we get the size of the graph and then we clear the graph, as it is not needed anymore
    num_nodes = grafo.size();
    grafo.clear();
  
    // int igraph_create(igraph_t *graph, const igraph_vector_int_t *edges,
    //   igraph_integer_t n, igraph_bool_t directed);
    igraph_create(&igrafo, &iedges, num_nodes, directed);

    // init result vector
    igraph_vector_init(&cheirpscore, 0);
  
    // reset vector
    igraph_vector_init(&reset, num_nodes);
    igraph_vector_fill(&reset, 0);
  
    /*
    * int igraph_personalized_pagerank(
    *    const igraph_t *graph,
    *    igraph_pagerank_algo_t algo, igraph_vector_t *vector,
    *    igraph_real_t *value, const igraph_vs_t vids,
    *    igraph_bool_t directed, igraph_real_t damping,
    *    igraph_vector_t *reset,
    *    const igraph_vector_t *weights,
    *    void *options);
    *
    * algo: IGRAPH_PAGERANK_ALGO_PRPACK is the recommended implementation
    *       http://igraph.org/c/doc/igraph-Structural.html#igraph_pagerank_algo_t
    */
  
    // jump probability to (remapped) S is 1
    VECTOR(reset)[newS]=1.0;

    int ret = -1;
    ret=igraph_personalized_pagerank(
         &igrafo,                         // const igraph_t *graph
         IGRAPH_PAGERANK_ALGO_PRPACK,     // igraph_pagerank_algo_t algo
         &cheirpscore,                       // igraph_vector_t *vector
         0,                               // igraph_real_t *value
         igraph_vss_all(),                // const igraph_vs_t vids
         directed,                        // igraph_bool_t directed
         pr_alpha,                        // igraph_real_t damping
         &reset,                          // igraph_vector_t *reset
         0,                               // const igraph_vector_t *weights,
         0                                // void *options
         );
    console->debug("CHEIR ret: {0:d}", ret);
    
    igraph_vector_destroy(&reset);
    igraph_destroy(&igrafo);

  FILE *outfp = stdout;
  if (!output_file.empty()) {
    outfp = fopen(output_file.c_str(), "w+");
  }

  map<float, int, greater<float>> orderedResults;

  for (unsigned int i=0; i<num_nodes; i++) {
    if (VECTOR(cheirpscore)[i] != 0.0) {
      orderedResults.insert(pair<float, int>(VECTOR(cheirpscore)[i], i));
    }
  }

  for (const auto& node: orderedResults) {
    int i = node.second;
      if (old2label.find(i) == old2label.end()) {
          //node not found in label system
      fprintf(outfp, "%d\t%d\t%.10e\n", i, i, VECTOR(cheirpscore)[i]);
        }
        else {
          //node is present in label system
      fprintf(outfp, "%d\t%s\t%.10e\n", i, old2label[i].c_str(), VECTOR(cheirpscore)[i]);
    }
  }

  console->info("Log stop!");
  exit (EXIT_SUCCESS);
}
