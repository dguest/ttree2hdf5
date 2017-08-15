#include "copy_root_tree.hh"
#include "get_tree.hh"

#include "H5Cpp.h"

#include "TFile.h"
#include "TTree.h"

int main(int argc, char* argv[]) {
  if (argc != 2) return 1;
  TFile file(argv[1]);
  TTree* tree = dynamic_cast<TTree*>(file.Get(get_tree(argv[1]).c_str()));

  // make the output file
  H5::H5File out_file("test.h5", H5F_ACC_TRUNC);
  copy_root_tree(*tree, out_file);

  return 0;
}
