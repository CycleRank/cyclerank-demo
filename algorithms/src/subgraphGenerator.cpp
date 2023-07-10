/*
usage: bin/subgraphGenerator -f examples/toy0/toy0.in.txt \
 -q ../cyclerank-api/output/57f85183-d081-4fe0-bb38-1d730fa4f729_20230308161145_cr-f_toy0.in.txt-s0-k3-dfalse-vfalse-eexp.csv -d -v
*/

#include <cassert>
#include <fstream>
#include <iomanip>      /* needed for std::setprecision() */
#include <iostream>
#include <vector>
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <string.h>
#include <limits>

extern "C" {
   #include <igraph.h>
}

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"

using namespace std;
namespace spd = spdlog;
namespace opts = cxxopts;

// *************************************************************************
// global variables
shared_ptr<spd::logger> main_logger;

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

// hsl to rgb conversion - https://gist.github.com/ciembor/1494530
typedef struct rgb {
  float r, g, b;
} RGB;

float hue2rgb(float p, float q, float t) {

  if (t < 0)
    t += 1;
  if (t > 1)
    t -= 1;
  if (t < 1./6)
    return p + (q - p) * 6 * t;
  if (t < 1./2)
    return q;
  if (t < 2./3)
    return p + (q - p) * (2./3 - t) * 6;

  return p;

}

RGB hsl2rgb(float h, float s, float l) {

  RGB result;

  if(0 == s) {
    result.r = result.g = result.b = l; // achromatic
  }
  else {
    float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    result.r = hue2rgb(p, q, h + 1./3) * 255;
    result.g = hue2rgb(p, q, h) * 255;
    result.b = hue2rgb(p, q, h - 1./3) * 255;
  }

  return result;

}

int main(int argc, char* argv[]) {

    // *************************************************************************
    // parse command-line options
    opts::Options* options;

    string input_dataset="";
    string input_query="";
    string log_file="";
    string output_file="";
    int maxnodes = -1;
    bool verbose = false;
    bool debug = false;
    bool help = false;
    bool print_stdout = false;

    try {
      options = new cxxopts::Options(argv[0]);
      options->add_options()
        ("f,file", "Input dataset file.",
         cxxopts::value<string>(input_dataset),
         "FILE"
         )
        ("q,query", "Input query file.",
         cxxopts::value<string>(input_query),
         "FILE"
         )
        ("n,maxnodes", "Set max number of nodes of the subgraph.",
        cxxopts::value(maxnodes)
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
        ("l,log", "Logfile [default: stderr].",
         cxxopts::value<string>(log_file),
         "LOG_FILE"
         )
        ("o,output", "Output file [default: stdout].",
         cxxopts::value<string>(output_file),
         "OUTPUT_FILE"
         )
        ;
      auto arguments = options->parse(argc, argv);
      //if output_file field is not filled, we print the result in stdout, and log everything in an arbitrary log file
      if(output_file.empty()) {
        print_stdout = true;
      }
    } catch (const cxxopts::OptionException& e) {
      cerr << "error parsing options: " << e.what() << endl;
      exit(EXIT_FAILURE);
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
    main_logger->debug("input_dataset: {}", input_dataset);
    main_logger->debug("input_query: {}", input_query);
    main_logger->debug("maxnodes: {}", maxnodes);
    main_logger->debug("verbose: {}", verbose);
    main_logger->debug("debug: {}", debug);

    // ********** end: start logging

    // *************************************************************************

    // start file streams (in: input_dataset, input_query. out: output_file)

    ifstream in_dataset(input_dataset);
    ifstream in_query(input_query);

    vector<bool> nodes;
    int N, M, nparam;

    if(in_dataset.fail()){
      cerr << "Error! Could not open input dataset file: " << input_dataset << endl;
      exit(EXIT_FAILURE);
    }
    if(in_query.fail()){
      cerr << "Error! Could not open input query file: " << input_query << endl;
      exit(EXIT_FAILURE);
    }

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

    // ********** end: start file streams

    nparam = count_parameters(in_dataset);
    in_dataset.close();

    main_logger->info("reading dataset...");
    in_dataset.open(input_dataset);
    if(nparam == 4) {
      string tmpS;
      int tmpK;
      in_dataset >> N >> M >> tmpS >> tmpK;
    } else if(nparam == 2) {
      in_dataset >> N >> M;
    } else {
      cerr << "Error! Error while reading file (" << input_dataset \
          << "), unexpected number of parameters" << endl;
      exit(EXIT_FAILURE);
    }

    nodes.resize(N);
    fill(nodes.begin(), nodes.end(), false);

    main_logger->info("reading query...");

    if (maxnodes == -1) {
      maxnodes = N;
    }

    string node, line, label, score, delimiter = "\t";
    int pos, linesRead = 0, nodesRead = 0, edgesRead = 0;
    vector<int> subgraphEdges;
    map<int, int> old2new;

    igraph_t igrafo;
    igraph_vector_int_t iedges;
    igraph_vector_int_t imembership;
    igraph_real_t iresolution(2.0);

    //in_query >> node >> label >> score
    while(getline(in_query, line) && linesRead < maxnodes) {
        pos = line.find(delimiter);
        node = line.substr(0, pos);
        line.erase(0, pos + delimiter.length());

        pos = line.find(delimiter);
        label = line.substr(0, pos);
        line.erase(0, pos + delimiter.length());

        pos = line.find(delimiter);
        score = line.substr(0, pos);
        line.erase(0, pos + delimiter.length());

        nodes[stoi(node)] = true;
        out << node << "\t" << label << "\t" << score << endl;
        old2new[linesRead] = stoi(node);
        linesRead++;
    }

    nodesRead = linesRead;

    out << "---" << endl;
    main_logger->info("--> read query");

    int s, t;
    for(int i=0; i<M; i++) {
        in_dataset >> s >> t;
        if(nodes[s] && nodes[t]) {
            out << s << "\t" << t << endl;
            edgesRead++;
            subgraphEdges.push_back(s);
            subgraphEdges.push_back(t);
        }
    }

    out << "---" << endl;
    main_logger->info("--> read dataset");

    // initialize vertices vector 4*num_edges
    igraph_vector_int_init(&iedges, 4*edgesRead);
    // initialize membership vector N
    igraph_vector_int_init(&imembership, nodesRead);

    // fwd
    for(int i=0; i<2*edgesRead; i=i+2) {
        VECTOR(iedges)[i] = subgraphEdges[i];
        VECTOR(iedges)[i+1] = subgraphEdges[i+1];
    }

    // rwd
    for(int i=0; i<2*edgesRead; i=i+2) {
        VECTOR(iedges)[2*edgesRead+ i]   = subgraphEdges[i+1];
        VECTOR(iedges)[2*edgesRead+ i+1] = subgraphEdges[i];
    }

    // create an igraph from the edges vector
    // int igraph_create(igraph_t *graph, const igraph_vector_int_t *edges,
    //   igraph_integer_t n, igraph_bool_t directed);
    igraph_create(&igrafo, &iedges, nodesRead, false);

    // apply louvain algorithm
    // igraph_error_t igraph_community_multilevel(const igraph_t *graph,
    //                                        const igraph_vector_t *weights,
    //                                        const igraph_real_t resolution,
    //                                        igraph_vector_int_t *membership,
    //                                        igraph_matrix_int_t *memberships,
    //                                        igraph_vector_t *modularity);
    igraph_community_multilevel(
      &igrafo,
      0,
      iresolution,
      &imembership,
      0,
      0
    );

    map<int, int> clusters;
    int clusterId = 0;
    for(int i=0; i<N; i++) {
        if(nodes[i]) {
            if(clusters.find(VECTOR(imembership)[i]) == clusters.end()) {
                clusters[VECTOR(imembership)[i]] = clusterId;
                clusterId++;
            }
            // out << i << "\t" << clusters[VECTOR(imembership)[i]] << endl;

            double PHI = (1 + sqrt(5))/2;
            double n = clusters[VECTOR(imembership)[i]] * PHI - floor(clusters[VECTOR(imembership)[i]] * PHI);

            // main_logger->info("hue: {}", n);
            RGB rgb = hsl2rgb(n, 0.95, 0.75);
            // main_logger->info("R: {}", rgb.r);
            // main_logger->info("G: {}", rgb.g);
            // main_logger->info("B: {}", rgb.b);

            out << i << "\t" << "rgb(" << (int)rgb.r << "," << (int)rgb.g << "," << (int)rgb.b << ")" << endl;
        }
    }

    main_logger->info("--> clusters computed");

    main_logger->info("Log stop!");
    exit(EXIT_SUCCESS);
}
