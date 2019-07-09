#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <frovedis.hpp>
#include <frovedis/ml/tree/tree.hpp>

namespace po = boost::program_options;
using namespace frovedis;

// default values for program options
constexpr size_t ARGNAME_LENGTH = 24 - (2 + 1);
#define POSARG_CAPTION "positional arguments"
#define OPTARG_CAPTION "optional arguments"

template <typename T>
void do_train(
  const std::string& input,
  const std::string& label,
  const std::string& output,
  const std::unordered_map<size_t, size_t>& categorical_features_info,
  const size_t num_trees,
  const tree::feature_subset_strategy feature_subset_strategy,
  const T feature_subset_rate,
  const size_t max_depth,
  const size_t max_bins,
  const size_t seed,
  const bool binary_mode
) {
  time_spent timer(DEBUG);
  random_forest_model<T> model;

  auto strategy = tree::make_regression_strategy<T>()
    .set_categorical_features_info(categorical_features_info)
    .set_num_trees(num_trees)
    .set_feature_subset_strategy(feature_subset_strategy)
    .set_feature_subset_rate(feature_subset_rate)
    .set_max_depth(max_depth)
    .set_max_bins(max_bins)
    .set_seed(seed);
  auto builder = make_random_forest_builder(strategy);

  if (binary_mode) {
    timer.reset();
    auto dataset = make_rowmajor_matrix_loadbinary<T>(input);
    timer.show("load matrix: ");
    auto labels = make_dvector_loadbinary<T>(label);
    timer.show("load labels: ");
    model = builder.run(dataset, labels);
    timer.show("train tree:  ");
    model.savebinary(output);
    timer.show("save model:  ");
  } else {
    timer.reset();
    auto dataset = make_rowmajor_matrix_load<T>(input);
    timer.show("load matrix: ");
    auto labels = make_dvector_loadline(label).map(
      +[] (const std::string& line) -> T {
        return boost::lexical_cast<T>(line);
      }
    );
    timer.show("load labels: ");
    model = builder.run(dataset, labels);
    timer.show("train tree:  ");
    model.save(output);
    timer.show("save model:  ");
  }

  RLOG(TRACE) << std::endl << model;
}

template <typename T>
void do_train(const po::variables_map& argmap) {
  do_train<T>(
    argmap["input"].as<std::string>(),
    argmap["label"].as<std::string>(),
    argmap["output"].as<std::string>(),
    tree::parse_categorical_features_info(
      argmap["category"].as<std::string>(),
      argmap["default-cardinality"].as<size_t>()
    ),
    argmap["trees"].as<size_t>(),
    tree::get_feature_subset_strategy(
      argmap["feature-subset-strategy"].as<std::string>()
    ),
    argmap["feature-subset-rate"].as<double>(),
    argmap["depth"].as<size_t>(),
    argmap["bins"].as<size_t>(),
    argmap["seed"].as<size_t>(),
    argmap.count("binary")
  );
}

po::variables_map parse(int argc, char** argv) {
  // a description of positional arguments
  po::options_description posarg_desc(POSARG_CAPTION);
  posarg_desc.add_options()(
    "input", po::value<std::string>(), "an input matrix"
  )(
    "label", po::value<std::string>(), "an input label"
  )(
    "output", po::value<std::string>(), "an output model"
  );
  po::positional_options_description posarg_settings;
  posarg_settings.add("input", 1).add("label", 1).add("output", 1);

  // a description of optional arguments
  po::options_description optarg_desc(OPTARG_CAPTION);
  optarg_desc.add_options()(
    "help,h", "show this help message and exit"
  )(
    "category,c", po::value<std::string>()->default_value(""),
    "categorical features information (index: cardinality, ...)"
  )(
    "default-cardinality", po::value<size_t>()->default_value(2),
    "default cardinality of categorical features"
  )(
    "trees,t", po::value<size_t>()->default_value(10),
    "number of iterations (forest size)"
  )(
    "feature-subset-strategy",
    po::value<std::string>()->default_value("auto"),
    "feature subset strategy"
  )(
    "feature-subset-rate", po::value<double>()->default_value(1.0),
    "feature subset custom rate"
  )(
    "depth,d", po::value<size_t>()->default_value(5),
    "maximum depth"
  )(
    "bins,b", po::value<size_t>()->default_value(32),
    "maximum number of bins"
  )(
    "seed", po::value<size_t>()->default_value(0),
    "random seed"
  )(
    "binary", "use binary input/output"
  )(
    "double", "use 64-bit float type (default: 32-bit float)"
  )(
    "verbose", "set log-level to DEBUG"
  )(
    "trace", "set log-level to TRACE"
  );

  // merge descriptions of all arguments
  po::options_description arg_desc;
  arg_desc.add(posarg_desc).add(optarg_desc);

  po::variables_map argmap;
  try {
    auto parsed = po::command_line_parser(argc, argv)
      .options(arg_desc)
      .positional(posarg_settings)
//    .allow_unregistered()
      .run();

    // make sure there are no `optional-style` positional arguments
    for (const auto parsed_opt: parsed.options) {
      if (parsed_opt.position_key < 0) {
        for (const auto opt: posarg_desc.options()) {
          if (parsed_opt.string_key == opt->long_name()) {
            throw po::unknown_option(parsed_opt.string_key);
          }
        }
      }
    }

    po::store(parsed, argmap);
    po::notify(argmap);
  } catch (const po::error_with_option_name& e) {
    std::cerr << e.what() << std::endl;
    finalizefrovedis(1);
  }

  // help message
  if (argmap.count("help")) {
    size_t max_length = ARGNAME_LENGTH;
    for (const auto opt: posarg_desc.options()) {
      const size_t temp = opt->long_name().length();
      if (max_length < temp) { max_length = temp; }
    }
    for (const auto opt: optarg_desc.options()) {
      const size_t namelen = opt->format_name().length();
      const size_t paramlen = opt->format_parameter().length();
      const size_t temp = namelen + paramlen + !!(paramlen > 0);
      if (max_length < temp) { max_length = temp; }
    }

    std::cerr << POSARG_CAPTION << ":" << std::endl;
    for (const auto opt: posarg_desc.options()) {
      const auto name = opt->long_name();
      std::cerr <<
        "  " << name <<
        std::string(max_length - name.length(), ' ') << " " <<
        opt->description() <<
      std::endl;
    }
    std::cerr << optarg_desc;

    finalizefrovedis(0);
  }

  // check required arguments
  for (const auto opt: posarg_desc.options()) {
    const std::string& name = opt->long_name();
    if (!argmap.count(name)) {
      std::cerr << "missing argument '" << name << "'" << std::endl;
      finalizefrovedis(1);
    }
  }

  return argmap;
}

int main(int argc, char** argv) {
  use_frovedis use(argc, argv);

  const auto argmap = parse(argc, argv);
  if (argmap.count("verbose")) { set_loglevel(DEBUG); }
  if (argmap.count("trace")) { set_loglevel(TRACE); }

  try {
    if (argmap.count("double")) {
      do_train<double>(argmap);
    } else {
      do_train<float>(argmap);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    finalizefrovedis(1);
  }

  return 0;
}
