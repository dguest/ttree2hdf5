#include "tree_copy_opts.hh"
#include <boost/program_options.hpp>
#include <iostream>

AppOpts get_tree_copy_opts(int argc, char* argv[])
{
  namespace po = boost::program_options;
  AppOpts app;
  std::string usage = "usage: " + std::string(argv[0]) + " <file>"
    + " -o <output> [-h] [opts...]\n";
  po::options_description opt(usage + "\nConvert a root tree to HDF5");
  opt.add_options()
    ("in-file",
     po::value(&app.file.in)->required(),
     "input file name")
    ("out-file,o",
     po::value(&app.file.out)->required(),
     "output file name")
    ("tree-name,t",
     po::value(&app.file.tree)->default_value("", "found"),
     "tree to use, use whatever is there by default (or crash if multiple)")
    ("help,h", "Print help messages")
    ("branch-regex,r",
     po::value(&app.tree.branch_regex)->default_value(""),
     "regex to filter branches")
    ("vector-lengths,l",
     po::value(&app.tree.vector_lengths)->multitoken()->value_name("args..."),
     "max size of vectors to write")
    ("verbose,v",
     po::bool_switch(&app.tree.verbose),
     "print branches copied")
    ("n-entries,n",
     po::value(&app.tree.n_entries)->default_value(0, "all")->implicit_value(1),
     "number of entries to copy")
    ("chunk-size,c",
     po::value(&app.tree.chunk_size)->default_value(CHUNK_SIZE),
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
  return app;
}

