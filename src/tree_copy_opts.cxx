#include "tree_copy_opts.hh"
#include <boost/program_options.hpp>
#include <iostream>

std::tuple<TreeCopyOpts, IOOpts> get_tree_copy_opts(
  int argc, char* argv[])
{
  namespace po = boost::program_options;
  TreeCopyOpts opts;
  IOOpts files;
  std::string usage = "usage: " + std::string(argv[0]) + " <file>"
    + " -o <output> [-h] [opts...]\n";
  po::options_description opt(usage + "\nConvert a root tree to HDF5");
  opt.add_options()
    ("in-file", po::value(&files.in)->required(), "input file name")
    ("out-file,o", po::value(&files.out)->required(), "output file name")
    ("help,h", "Print help messages")
    ("branch-regex,r", po::value(&opts.branch_regex)->default_value(""),
     "regex to filter branches")
    ("vector-lengths,l",
     po::value(&opts.vector_lengths)->multitoken()->value_name("args..."),
     "max size of vectors to write")
    ("verbose,v", po::bool_switch(&opts.verbose), "print branches copied")
    ("n-entries,n",
     po::value(&opts.n_entries)->default_value(0, "all")->implicit_value(1),
     "number of entries to copy")
    ("chunk-size,c", po::value(&opts.chunk_size)->default_value(CHUNK_SIZE),
     "chunk size in HDF5 file")
    ;
  po::positional_options_description pos_opts;
  pos_opts.add("in-file", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(opt)
            .positional(pos_opts).run(), vm);
    if ( vm.count("help") ) {
      std::cout << opt << std::endl;
      exit(1);
    }
    po::notify(vm);
  } catch (po::error& err) {
    std::cerr << usage << "ERROR: " << err.what() << std::endl;
    exit(1);
  }
  return std::tie(opts, files);
}

