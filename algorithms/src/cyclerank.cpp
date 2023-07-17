#include <cassert>
#include <fstream>
#include <iomanip>      /* needed for std::setprecision() */
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
#include <string.h>

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
  double score;
  vector<int> adj;
  list<int> B;

  nodo(){
    active = true;
    blocked = false;
    dist = -1;
    score = 0.0;
    B.clear();
  }
};


// *************************************************************************
// global variables
shared_ptr<spd::logger> main_logger;

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


void write_circuit(stack<int> s, vector<int>& new2old, map<int,string>& old2label, ostream& out) {
  vector<int> tmp;

  while (!s.empty()) {
    int el = s.top();
    int oldel = new2old[el];
    tmp.push_back(oldel);
    s.pop();
  }

  for (unsigned k=tmp.size()-1; k>0; k--) {string label = "";
      if (old2label.find(tmp[k]) == old2label.end()) {
        //node not found in label system
        out << tmp[k] << " ";
      }
      else {
        //node is present in label system
        out << old2label[tmp[k]] << " ";
      }
  }

  //repeat the same block for node tmp[0], as we need to print a newline here

  if (old2label.find(tmp[0]) == old2label.end()) {
        //node 0 not found in label system
        out << tmp[0] << endl;
      }
      else {
        //node 0 is present in label system
        out << old2label[tmp[0]] << endl;
  }
}

void compute_scores(vector<nodo>& grafo, stack<int> s, vector<int>& new2old, string& scoring_function) {
  int cycle_size = s.size();
  while (!s.empty()) {
    int el = s.top();

    //TODO: add more scoring functions

    if (scoring_function == "lin") {
      grafo[el].score += 1.0 / cycle_size;
    }
    else if (scoring_function == "exp") {
      grafo[el].score += exp(-1*cycle_size);
    }
    else if (scoring_function == "quad") {
      grafo[el].score += 1.0 / (pow(cycle_size, 2));
    }
    else if (scoring_function == "log") {
      grafo[el].score += 1.0 / (log(cycle_size));
    }

    // main_logger->debug("remapped node {} has score {}", el, grafo[el].score);
    s.pop();
  }
}

//when we finish calling circuit we can write all the computed scores
void write_scores(int source, vector<nodo>& grafo, vector<int>& new2old, map<int,string>& old2label, ostream& out) {
  main_logger->debug("graph has size {}", grafo.size());
  if (grafo[source].score == 0.0) {
    // source node has score 0.0, therefore there are no cycles. Only print the source
    int oldSource = new2old[source];

    if (old2label.find(oldSource) == old2label.end()) {
      out << oldSource << "\t" << oldSource << "\t" << fixed << setprecision(10) << scientific << grafo[source].score << endl;
    }
    else {
      out << oldSource << "\t" << old2label[oldSource] << "\t" << fixed << setprecision(10) << scientific << grafo[source].score << endl;
    }   
  }
  else {

    multimap<float, int, greater<float>> orderedResults;
    for (unsigned int i=0; i<grafo.size(); i++) {
      if (grafo[i].score != 0.0) {
        orderedResults.insert(pair<float, int>(grafo[i].score, i));
      }
    }

    for (const auto& node: orderedResults) {
      int i = node.second;
      int oldi = new2old[i];
      if (old2label.find(i) == old2label.end()) {
        //node not found in label system
        out << oldi << "\t" << oldi << "\t" << fixed << scientific << setprecision(10) << grafo[i].score << endl;
      }
      else {
        //node is present in label system
        out << oldi << "\t" << old2label[oldi] << "\t" << fixed << scientific << setprecision(10) << grafo[i].score << endl;
      }
    }

    // for (unsigned int i = 0; i < grafo.size(); i++) {
    //   if (grafo[i].score != 0.0) {
    //     int oldi = new2old[i];

    //     if (old2label.find(oldi) == old2label.end()) {
    //       //node not found in label system
    //       main_logger->info("printing node {}: score {}", oldi, grafo[i].score);
    //       out << oldi << "\t" << oldi << "\t" << fixed << setprecision(10) << grafo[i].score << endl;
    //     }
    //     else {
    //       //node is present in label system
    //       main_logger->info("printing node {}: score {}", oldi, grafo[i].score);
    //       out << oldi << "\t" << old2label[oldi] << "\t" << fixed << setprecision(10) << grafo[i].score << endl;
    //     }      
    //   }
    // }
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
    //main_logger->debug("tmp_c: {}", tmp_c);
    next_c = line[i];
    if (tmp_c == ' ' && (next_c != ' ' || next_c != '\n')) {
      ++firstline_count;
    }
    tmp_c = next_c;
  }

  //we count every space between the first input line's data, without counting double spaces and spaces after the last input (e.g. evey space before \n)
  //as such, the number of parameters of the first line of the file equals number of spaces counted + 1.
  ++firstline_count;

  main_logger->debug("firstline_count: {}", firstline_count);

  return firstline_count;
}

// get remapped node
int get_remapped_node_or_fail(int s, map<int,int>& map_old2new ) {
  int newS = -1;

  if ( map_old2new.find(s) == map_old2new.end() ) {
    // Key s not found
    cerr << "Key " << s << " not found in map" << endl;
    exit(EXIT_FAILURE);
  } else {
    // Key s found
    newS = map_old2new[s];
    return newS;
  }

}
// ********** end: helper functions


// *************************************************************************
void destroy_nodes(vector<nodo>& g, vector<bool>& destroy) {

  for(unsigned int i=0; i<g.size(); i++) {
    if (destroy[i]) {
      g[i].active = false;
      g[i].adj.clear();
    } else {
      for (unsigned int j=0; j<g[i].adj.size(); j++) {
        int v = g[i].adj[j];
        if (destroy[v]) {
          g[i].adj.erase(g[i].adj.begin()+j);
        }
      }
    }
  }
}


void bfs(int source, unsigned int K, vector<nodo>& g) {

  if (!g[source].active) {
    return;
  }

  for (nodo& n: g) {
    n.dist = -1;
  }

  g[source].dist = 0;

  queue<int> q;
  q.push(source);
  int cur;
  while (!q.empty()){
    cur = q.front();
    q.pop();

    // if g[cur].dist == K-1 we can stop
    if(g[cur].dist > (int) K-2) {
      continue;
    }

    for (int v: g[cur].adj) {
      if ((g[v].dist==-1) and (g[v].active)) {

        // neighbor not yet visited, set distance
        g[v].dist = g[cur].dist + 1;
        q.push(v);
      }
    }
  }
}


void unblock(int u, vector<nodo>& g) {
  // printf("      ----> unblock(%d)\n", u);
  g[u].blocked = false;
  // printf("        ----> g[%d].blocked: %s\n", u, g[u].blocked ? "true" : "false");

  while (!g[u].B.empty()) {
    int w = g[u].B.front();
    g[u].B.pop_front();

    if (g[w].blocked) {
      unblock(w, g);
    }
  }
}

int count_calls=0;
bool circuit(int v, int S, unsigned int K,
             vector<nodo>& g,
             vector<int>& new2old,
             map<int,string>& old2label,
             stack<int>& circuits_st,
             ostream& out,
             bool& print_cycles,
             string& scoring_function) {
  bool flag = false;

  if (!(circuits_st.size() > K-1)) {
    count_calls++;

    circuits_st.push(v);

    g[v].blocked = true;

    for(int w : g[v].adj) {
      if (w == S) {
        if (!(circuits_st.size() > K)) {
          //cycle has been found and is within limits, print it
          //write_circuit prints the cycle we have found.
          if (print_cycles) write_circuit(circuits_st, new2old, old2label, out);
          
          //compute_scores calculates the scores with a given function. We do not print as one node may be part of more cycles.
          else compute_scores(g, circuits_st, new2old, scoring_function);
        }
        flag = true;
      } else if (!g[w].blocked) {
        if (circuit(w, S, K, g, new2old, old2label, circuits_st, out, print_cycles, scoring_function)) {
          flag = true;
        }
      }
    }

    if (flag) {
      unblock(v, g);
    } else {
      for (int w: g[v].adj) {
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


int main(int argc, char* argv[]) {

  // *************************************************************************
  // parse command-line options
  opts::Options* options;
  string input_file="input.txt";
  string log_file="";
  string output_file="";
  string scoring_function="exp";
  string cliS = "";
  int cliK = -1;
  bool verbose = false;
  bool debug = false;
  bool help = false;
  bool print_cycles = false;
  bool print_stdout = false;

  try {
    options = new cxxopts::Options(argv[0]);

    options->add_options()
      ("f,file", "Input file.",
       cxxopts::value<string>(input_file),
       "FILE"
       )
      ("v,verbose", "Enable logging at verbose level.",
       cxxopts::value(verbose)
       )
      ("d,debug", "Enable logging at debug level.",
       cxxopts::value(debug)
       )
      ("h,help", "Show help message and exit.",
       cxxopts::value(help)
       )
      ("k,maxloop", "Set max loop length (K).",
       cxxopts::value(cliK),
       "K"
       )
      ("l,log", "Logfile [default: stderr].",
       cxxopts::value<string>(log_file),
       "LOG_FILE"
       )
      ("o,output", "Output file [default: stdout].",
       cxxopts::value<string>(output_file),
       "OUTPUT_FILE"
       )
      ("s,source", "Set source node (S).",
       cxxopts::value<string>(cliS),
       "S"
       )
      ("p,print_cycles", "Print the cycles [default: prints the scores].",
       cxxopts::value(print_cycles)
       )
      ("e,scoring_function", "Scoring function {exp, lin, quad, log} [default: exp].",
        cxxopts::value<string>(scoring_function),
        "SCORING_FUNCTION"
       )
      ;

    auto arguments = options->parse(argc, argv);

    //if output_file field is not filled, we print the result in stdout, and log everything in an arbitrary log file
    if(output_file.empty()) {
      print_stdout = true;
    }

  } catch (const cxxopts::OptionException& e) {
    cerr << "error parsing options: " << e.what() << endl;
    exit (EXIT_FAILURE);
  }

  // if help option is activated, print help and exit.
  if(help) {
    cout << options->help({""}) << endl;
    exit(0);
  }
  // ********** end: parse command-line options

  // *************************************************************************
  // initialize logger
  try {
    if (log_file.empty()) {
      // by default, log using the standard console (outputs on stderr)
      main_logger = spd::stderr_color_mt("console");
    } else {
      //basic_logger appends inside the log file. For clarity, we delete the previous information.
      main_logger = spd::basic_logger_mt("main_logger", log_file);
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

  main_logger->info("Log start!");
  main_logger->debug("input_file: {}", input_file);
  main_logger->debug("verbose: {}", verbose);
  main_logger->debug("debug: {}", debug);
  // ********** end: start logging

  // *************************************************************************
  // start algorithm
  int S = -1, newS = -1;
  unsigned int N = 0, M = 0, K = 0;
  vector<nodo> grafo;

  map<int,int> old2new;
  //map that associates old nodes to the labels given by the user
  map<int,string> old2label;
  vector<int> new2old;

  int count_destroyed = 0;

  // *************************************************************************
  // read input
  {
    ifstream in(input_file);
    //int tmpS = -1;
    string tmpS = "";
    int tmpK = -1;
    int nparam = 0;

    if(in.fail()){
      cerr << "Error! Could not open file: " << input_file << endl;
      exit(EXIT_FAILURE);
    }

    nparam = count_parameters(in);
    in.close();

    in.open(input_file);
    if(nparam == 4) {
      in >> N >> M >> tmpS >> tmpK;
    } else if(nparam == 2) {
      in >> N >> M;
    } else {
      cerr << "Error! Error while reading file (" << input_file \
          << "), unexpected number of parameters" << endl;
      exit(EXIT_FAILURE);
    }

    main_logger->debug("reading graph...");
    grafo.resize(N);
    //int selfLoopCounter = 0;
    for(unsigned int j=0; j<M; j++) {
      int s, t;
      in >> s >> t;

      // if (s == t) {
      //   cout << "self loops found: " << s << " " << t << endl;
      //   selfLoopCounter++;
      // }
      // else {
      //   cout << s << " " << t << endl;
      // }

      if (s==t) {
        continue;
      }
      
      /* assert( (s != t) \
      //          && "The two ends of an edge must be different nodes." );*/
      

      // check that we are not inserting duplicates
      if (find(grafo[s].adj.begin(), \
               grafo[s].adj.end(), \
               t) == grafo[s].adj.end()) {
        grafo[s].adj.push_back(t);
      }
    }
    main_logger->debug("--> read graph");

    main_logger->debug("reading labels...");

    unsigned int mapped_node = -1;
    string label;
    
    while(in >> mapped_node) {
      // trim whitespace before label
      getline(in >> std::ws, label);
      label.erase(std::remove_if(label.begin(), label.end(), &IsNewlineOrTab), label.end());
      // main_logger->debug("mapped node: {}", mapped_node);
      // main_logger->debug("label: {}", label);

      assert( (mapped_node < N) \
              && "the node to be labeled must be within 0 and N-1." );
      
      //TODO: use a constant here
      assert( (label.size() <= 256) \
              && "the label's size must be less or equal than 256 characters." );

      //main_logger->debug("Mapped node is: {}", mapped_node);
      assert( (!label.empty()) \
              && "the label must not be empty." );
      old2label.insert(pair<int,string>(mapped_node, label));
    }
    
    main_logger->debug("--> read labels");

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
            main_logger->debug("S is a label and it has been found");
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
            main_logger->debug("S is a label and it has been found");
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

    if(cliK == -1) {
      K = (unsigned int) tmpK;
    } else {
      K = (unsigned int) cliK;
    }

    main_logger->info("N: {}", N);
    main_logger->info("M: {}", M);
    main_logger->info("S: {}", S);
    main_logger->info("K: {}", K);
    main_logger->info("R: {}", scoring_function);

    assert( (N > 0 && M > 0) \
            && "N and M must be positive." );

    assert( (K > 0 && S >= 0) \
            && "K must be positive and S must be non-negative." );

    assert( (scoring_function == "exp" || scoring_function == "lin" || scoring_function == "quad" || scoring_function == "log") \
            && "Scoring function must be 'exp' (exponential), 'lin', linear, 'quad', quadratic, 'log', logarithmic." );

    in.close();
  }
  // ********** end: read input


  // *************************************************************************
  // Step 1: BFS on g
  {
    main_logger->info("Step 1. BFS");
    vector<bool> destroy(N, false);

    bfs(S, K, grafo);

    for(unsigned int i=0; i<N; i++) {
      // se il nodo si trova distanza maggiore di K-1 o non raggiungibile
      if((grafo[i].dist == -1) or (grafo[i].dist > (int) K-1)) {
        destroy[i] = true;
        count_destroyed++;
      }
    }

    int remaining = N-count_destroyed;
    main_logger->info("nodes: {}", N);
    main_logger->info("destroyed: {}", count_destroyed);
    main_logger->info("remaining: {}", remaining);
    new2old.resize(remaining);

    int newindex = -1;
    for(unsigned int i=0; i<N; i++) {
      if(!destroy[i]) {
        newindex++;
        new2old[newindex] = i;
        old2new.insert(pair<int,int>(i,newindex));
        /*
        main_logger->debug("old2new.insert(pair<int,int>({0}, {1}))",
                       i,
                       newindex);
        */
      }
    }

    if(debug) {
      main_logger->debug("index map (1)");
      main_logger->debug("old2new.size() is {}", old2new.size());

      int c = 0;
      for (auto const& pp : old2new) {
        main_logger->debug("{0:d} => {1:d}, {2:d} => {3:d}",
                       pp.first,
                       pp.second,
                       c,
                       new2old[c]);
        c++;
      }
      main_logger->debug("~~~");
    }

    destroy_nodes(grafo, destroy);

    vector<nodo> tmpgrafo;
    tmpgrafo.resize(remaining);

    int newi = -1;
    int newv = -1;

    for(unsigned int i=0; i<N; i++) {
      if(grafo[i].active) {
        newi = old2new[i];
        tmpgrafo[newi].dist = grafo[i].dist;
        tmpgrafo[newi].active = true;
        for (int v: grafo[i].adj) {
          if(grafo[v].active) {
            newv = old2new[v];
            tmpgrafo[newi].adj.push_back(newv);
          }
        }
      }
    }

    grafo.clear();
    grafo.swap(tmpgrafo);

    destroy.clear();
  }
  // ********** end: Step 1

  vector<bool> destroy(grafo.size(), false);


  // *************************************************************************
  // get remapped source node (S)
  int remappedS1 = -1;
  remappedS1 = get_remapped_node_or_fail(S, old2new);
  main_logger->info("S: {0}, newS: {1}", S, remappedS1);
  // ********** end: get remapped source node (S)

  // *************************************************************************
  // Step 2: BFS on g^T
  {
    main_logger->info("Step 2.: BFS on g^T");
    vector<nodo> grafoT;
    grafoT.resize(grafo.size());

    for(unsigned int i=0; i<grafo.size(); i++) {
      for (int v: grafo[i].adj) {
        grafoT[v].adj.push_back(i);
      }
    }

    bfs(remappedS1 , K, grafoT);

    for(unsigned int i=0; i<grafo.size(); i++) {
      if((grafo[i].dist == -1) or (grafoT[i].dist == -1) or \
          (grafo[i].dist + grafoT[i].dist > (int) K)) {
        // main_logger->debug("destroyed node: {0:d}\n", i);
        destroy[i] = true;
        count_destroyed++;
      }
    }

    int remaining = N-count_destroyed;

    main_logger->info("nodes: {}", N);
    main_logger->info("destroyed: {}", count_destroyed);
    main_logger->info("remaining: {}", remaining);

    destroy_nodes(grafo, destroy);
  }
  // ********** end: Step 2

  int remaining = N-count_destroyed;
  map<int,int> tmp_old2new;
  vector<int> tmp_new2old;
  tmp_new2old.resize(remaining);

  {
    int newindex = -1;
    int oldi = -1;
    for(unsigned int i=0; i<grafo.size(); i++) {
      if(!destroy[i]) {
        newindex++;

        oldi = new2old[i];

        main_logger->debug("newindex: {}", newindex);
        main_logger->debug("i: {} - oldi: {}", i, oldi);

        tmp_new2old[newindex] = oldi;
        tmp_old2new.insert(pair<int,int>(oldi, newindex));
        main_logger->debug("tmp_new2old[{0}]: {1}",
                       newindex,
                       tmp_new2old[newindex]);
        main_logger->debug("tmp_old2new.insert(pair<int,int>({0}, {1}))",
                       oldi,
                       newindex);
      }
    }
  }

  if(debug) {
    main_logger->debug("*** tmp maps ***");
    main_logger->debug("tmp_old2new, tmp_new2old");
    main_logger->debug("tmp_old2new.size() is {}", tmp_old2new.size());

    int c = 0;
    for (auto const& pp : tmp_old2new) {
      main_logger->debug("{0:d} => {1:d}, {2:d} => {3:d}",
                     pp.first,
                     pp.second,
                     c,
                     tmp_new2old[c]);
      c++;
    }

    main_logger->debug("*** maps BBB ***");
    main_logger->debug("old2new.size() is {}", old2new.size());
    main_logger->debug("old2new, new2old");
    c = 0;
    for (auto const& pp : old2new) {
      main_logger->debug("{0:d} => {1:d}, {2:d} => {3:d}",
                     pp.first,
                     pp.second,
                     pp.second,
                     new2old[pp.second]);
      c++;
    }
    main_logger->debug("~~~");

    main_logger->debug("*** 1 ***");
  }

  {
    int oldi = -1, tmpnewi = -1;
    int oldv = -1, tmpnewv = -1;

    vector<nodo> tmpgrafo;
    tmpgrafo.resize(remaining);
    for(unsigned int i=0; i<grafo.size(); i++) {
      if(grafo[i].active) {

        oldi = new2old[i];
        tmpnewi = tmp_old2new[oldi];
        tmpgrafo[tmpnewi].dist = grafo[i].dist;
        tmpgrafo[tmpnewi].active = true;
        for (int v: grafo[i].adj) {
          if(grafo[v].active) {
            oldv = new2old[v];
            tmpnewv = tmp_old2new[oldv];
            tmpgrafo[tmpnewi].adj.push_back(tmpnewv);
          }
        }
      }
    }

    grafo.clear();
    grafo.swap(tmpgrafo);
    destroy.clear();
  }

  // *************************************************************************
  // get remapped source node (S)
  int remappedS2 = -1;
  remappedS2 = get_remapped_node_or_fail(S, tmp_old2new);
  main_logger->info("S: {0}, S1: {1}, S2: {2}", S, remappedS1, remappedS2);
  // the new index of S is remappedS2
  newS = remappedS2;
  // ********** end: get remapped source node (S)

  new2old.clear();
  old2new.clear();

  tmp_new2old.swap(new2old);
  tmp_old2new.swap(old2new);


  if(debug) {
    main_logger->debug("map indexes");
    main_logger->debug("old2new.size() is {}", old2new.size());

    for (auto const& pp : old2new) {
      main_logger->debug("{0:d} => {1:d}, {2:d} => {3:d}",
                     pp.first,
                     pp.second,
                     pp.second,
                     new2old[pp.second]);
    }
    main_logger->debug("~~~");
  }

  // *************************************************************************
  // Obtain a std::ostream either from std::cout or std::ofstream(file)
  // https://stackoverflow.com/a/366969/2377454
  streambuf * buf;
  ofstream of;

  main_logger->debug("print_stdout is {}", print_stdout);

  if(!print_stdout) {
    main_logger->debug("opening specified output file");
    of.open(output_file);
    buf = of.rdbuf();
  } else {
    buf = cout.rdbuf();
  }

  ostream out(buf);
  // ********** end: obtain out stream

  main_logger->debug("calling circuit()");

  stack<int> circuits_st;
  //if we call compute_scores inside circuit we do not need to pass out as an argument. Though I left it if we still need to see the cycles via write_circuit
  circuit(newS, newS, K, grafo, new2old, old2label, circuits_st, out, print_cycles, scoring_function);

  main_logger->debug("count_calls: {}", count_calls);
  main_logger->debug("called circuit()");

  if (!print_cycles) {
    main_logger->debug("calling write_scores()");
    write_scores(newS, grafo, new2old, old2label, out);
    main_logger->debug("called write_scores()");
  } 

  main_logger->info("Log stop!");
  exit (EXIT_SUCCESS);
}
