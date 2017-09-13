#include "tree_copy_opts.hh"
#include "parser.hh"
#include <iostream>

namespace {
  size_t safe_get_size(int in) {
    if (in < 0) {
      std::cerr << "ERROR: got negative value for a size" << std::endl;
      exit(1);
    }
    return in;
  }
}

std::tuple<TreeCopyOpts, IOOpts> get_tree_copy_opts(
int argc, const char* argv[])
{
optionparser::parser opt;
  TreeCopyOpts opts;
  IOOpts files;
  opt.add_option("--in-file", "-i")
    .help("input file name").required(true)
    .mode(optionparser::store_value);
  opt.add_option("--out-file", "-o")
    .help("output file name").required(true)
    .mode(optionparser::store_value);
  opt.add_option("--tree-name", "-t").help(
    "tree to use, use whatever is there by default (or crash if multiple)")
    .default_value("")
    .mode(optionparser::store_value);
  opt.add_option("--branch-regex", "-r").help("regex to filter branches")
    .default_value("")
    .mode(optionparser::store_value);
  opt.add_option("--vector-lengths", "-l")
    .help("max size of vectors to write")
    .mode(optionparser::store_mult_values);
  opt.add_option("--verbose", "-v").help("print branches copied")
    .mode(optionparser::store_true);
  opt.add_option("--n-entries", "-n").help("number of entries to copy")
    .default_value(0)
    .mode(optionparser::store_value);
  opt.add_option("--chunk-size", "-c").help("chunk size in HDF5 file")
    .default_value(CHUNK_SIZE)
    .mode(optionparser::store_value);
  opt.eat_arguments(argc, argv);
  files.in = opt.get_value<std::string>("in-file");
  files.out = opt.get_value<std::string>("out-file");
  files.tree = opt.get_value<std::string>("tree-name");
  opts.branch_regex = opt.get_value<std::string>("branch-regex");
  std::vector<int> vector_lengths;
  for (int length: opt.get_value<std::vector<int> >("vector-lengths")) {
    opts.vector_lengths.push_back(safe_get_size(length));
  }
  opts.verbose = opt.get_value<bool>("verbose");
  opts.n_entries = safe_get_size(opt.get_value<int>("n-entries"));
  opts.chunk_size = safe_get_size(opt.get_value<int>("chunk-size"));
  return std::tie(opts, files);
}

