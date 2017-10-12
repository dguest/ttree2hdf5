#include "copy_root_tree.hh"
#include "get_tree.hh"
#include "tree_copy_opts.hh"

#include "unshittify.hh"

#include "H5Cpp.h"

#include "TFile.h"
#include "TTree.h"

#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
  unshittify();
  AppOpts opts = get_tree_copy_opts(argc, argv);

  // Read in the root tree. We pick whatever tree is on the top level
  // of the file. If there are two we throw an error.
  std::string tree_name = opts.file.tree;
  if (tree_name.size() == 0) tree_name = get_tree(opts.file.in);
  if (opts.tree.verbose) std::cout << "tree: " << tree_name << std::endl;
  std::unique_ptr<TFile> file(TFile::Open(opts.file.in.c_str()));
  TTree* tree = dynamic_cast<TTree*>(file->Get(tree_name.c_str()));
  if (!tree) {
    throw std::logic_error("no tree '" + tree_name + "' found");
  }

  // make the output file
  H5::H5File out_file(opts.file.out, H5F_ACC_TRUNC);

  // All the magic appens here
  copy_root_tree(*tree, out_file, opts.tree);

  return 0;
}
