#include <boost/program_options.hpp>
#include <frovedis.hpp>
#include <frovedis/ml/clustering/gmm.hpp>

using namespace boost;
using namespace frovedis;
using namespace std;

void do_gmm(const string& input, const string& output, int k, int n_init,
            int num_iteration, double eps, bool switch_init, long seed,
            bool binary) {
  if (binary) {
    time_spent t(DEBUG);
    auto mat = make_rowmajor_matrix_loadbinary<double>(input);
    t.show("load matrix: ");
    auto gmm_model =
        frovedis::gmm(mat, k, num_iteration, eps, switch_init, seed);
    t.show("train: ");
    LOG(DEBUG) << "number of loops until convergence: " << gmm_model.loops
               << std::endl;
    LOG(DEBUG) << "likelihood: " << gmm_model.likelihood << std::endl;
    gmm_model.savebinary(output);

  } else {
    time_spent t(DEBUG);
    auto mat = make_rowmajor_matrix_load<double>(input);
    t.show("load matrix: ");
    auto gmm_model =
        frovedis::gmm(mat, k, num_iteration, eps, switch_init, seed);
    t.show("gmm time: ");
    LOG(DEBUG) << "number of loops until convergence: " << gmm_model.loops
               << std::endl;
    LOG(DEBUG) << "likelihood: " << gmm_model.likelihood << std::endl;

    gmm_model.save(output);
  }
}

void do_assign(const string& input, const string& input_cluster,
               const string& output, int k, bool binary) {
  if (binary) {
    time_spent t(DEBUG);
    auto mat = make_rowmajor_matrix_local_load<double>(input);
    t.show("load matrix: ");
    auto gmm_cluster = frovedis::gmm_assign_cluster(mat, k, input_cluster);
    t.show("gmm time: ");
    gmm_cluster.savebinary(output);
  } else {
    time_spent t(DEBUG);
    auto mat = make_rowmajor_matrix_local_load<double>(input);
    t.show("load matrix: ");
    auto gmm_cluster = frovedis::gmm_assign_cluster(mat, k, input_cluster);
    t.show("gmm time: ");
    gmm_cluster.save(output);
  }
}

int main(int argc, char* argv[]) {
  use_frovedis use(argc, argv);

  using namespace boost::program_options;

  options_description opt("option");
  // clang-format off
  opt.add_options()("help,h", "print help")
    ("assign,a", "assign data to cluster mode")
    ("input,i", value<string>(),"input matrix")
    ("cluster,c", value<string>(), "input model,cov,pi for assignment")
    ("output,o", value<string>(), "output centroids or cluster")
    ("k,k", value<int>(), "number of clusters")
    ("num-init,t", value<int>(),"number of time for running with different centroid seeds (default: 1)")
    ("num-iteration,n", value<int>(),"maximum number of iteration (default: 300)")
    ("eps,e", value<double>(),"epsilon to stop the iteration (default: 0.0001)")
    ("swicth_init,s", value<bool>(), "kmeans(0) or random(1) (default: 0)")
    ("seed,r", value<long>(), "seed for init randomizer (default: 0)")
    ("verbose", "set loglevel to DEBUG")
    ("verbose2", "set loglevel to TRACE")
    ("binary,b", "use binary input/output");
  // clang-format on
  variables_map argmap;
  store(command_line_parser(argc, argv).options(opt).allow_unregistered().run(),
        argmap);
  notify(argmap);

  string input, output, input_cluster;
  int k = 0;
  int num_iteration = 300, n_init = 1;
  double eps = 0.01;
  long seed = 123;
  bool assign = false;
  bool binary = false;
  bool switch_init = 0;
  if (argmap.count("help")) {
    cerr << opt << endl;
    exit(1);
  }

  if (argmap.count("input")) {
    input = argmap["input"].as<string>();
  } else {
    cerr << "input is not specified" << endl;
    cerr << opt << endl;
    exit(1);
  }

  if (argmap.count("output")) {
    output = argmap["output"].as<string>();
  } else {
    cerr << "output is not specified" << endl;
    cerr << opt << endl;
    exit(1);
  }

  if (argmap.count("assign")) {
    assign = true;
  }

  if (argmap.count("cluster")) {
    input_cluster = argmap["cluster"].as<string>();
  } else {
    if (assign == true) {
      cerr << "cluster is not specified" << endl;
      cerr << opt << endl;
      exit(1);
    }
  }

  if (argmap.count("k")) {
    k = argmap["k"].as<int>();
  } else {
    if (assign == false) {
      cerr << "number of cluster is not specified" << endl;
      cerr << opt << endl;
      exit(1);
    }
  }

  if (argmap.count("num-iteration")) {
    num_iteration = argmap["num-iteration"].as<int>();
  }

  if (argmap.count("num-init")) {
    n_init = argmap["num-init"].as<int>();
  }

  if (argmap.count("epsilon")) {
    eps = argmap["epsilon"].as<double>();
  }
  if (argmap.count("switch_init")) {
    eps = argmap["switch_init"].as<bool>();
  }

  if (argmap.count("seed")) {
    seed = argmap["seed"].as<long>();
  }
  if (argmap.count("binary")) {
    binary = true;
  }
  if (argmap.count("verbose")) {
    set_loglevel(DEBUG);
  }
  if (argmap.count("verbose2")) {
    set_loglevel(TRACE);
  }

  if (assign)
    do_assign(input, input_cluster, output, k, binary);
  else
    do_gmm(input, output, k, n_init, num_iteration, eps, switch_init, seed,
           binary);
}