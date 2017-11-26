#include "copy_root_tree.hh"
#include "get_tree.hh"
#include "tree_copy_opts.hh"

#include "unshittify.hh"

#include "H5Cpp.h"

#include "TFile.h"
#include "TChain.h"

#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
  unshittify();
  AppOpts opts = get_tree_copy_opts(argc, argv);

  // Read in the root tree. We pick whatever tree is on the top level
  // of the file. If there are two we throw an error.
  std::string tree_name = opts.file.tree;
  if (tree_name.size() == 0) tree_name = get_tree(opts.file.in.at(0));
  if (opts.tree.verbose) std::cout << "tree: " << tree_name << std::endl;
  std::unique_ptr<TChain> chain(new TChain(tree_name.c_str()));
  for (const auto& file_name: opts.file.in) {
    if (opts.tree.verbose) std::cout << "adding " << file_name << std::endl;
    int ret_code = chain->Add(file_name.c_str(), -1);
    if (ret_code != 1) {
      std::cerr << "Tree '" << tree_name << "' is missing from "
                << file_name << std::endl;
      return 1;
    }
  }

  // make the output file
  H5::H5File out_file(opts.file.out, H5F_ACC_TRUNC);

  // All the magic appens here
  copy_root_tree(*chain, out_file, opts.tree);

  return 0;
}
