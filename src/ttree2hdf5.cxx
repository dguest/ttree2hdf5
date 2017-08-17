#include "copy_root_tree.hh"
#include "get_tree.hh"

#include "unshittify.hh"

#include "H5Cpp.h"

#include "TFile.h"
#include "TTree.h"

// todo, use boost program options or something else that doesn't suck
// for option parsing.
void usage(const char* prog) {
  printf("usage: %s <root-file> <output-file-name> [length..]\n", prog);
}

const size_t CHUNK_SIZE = 128;

int main(int argc, char* argv[]) {
  unshittify();
  if (argc < 3 || argc > 5) {
    usage(argv[0]);
    return 1;
  }
  TFile file(argv[1]);
  TTree* tree = dynamic_cast<TTree*>(file.Get(get_tree(argv[1]).c_str()));
  if (!tree) {
    throw std::logic_error("no tree found");
  }
  size_t length = 0;            // no 2d written by default
  if (argc >= 4) length = atoi(argv[3]);
  size_t length2 = 0;
  if (argc == 5) length2 = atoi(argv[4]);
  // make the output file
  H5::H5File out_file(argv[2], H5F_ACC_TRUNC);
  copy_root_tree(*tree, out_file, length, length2, CHUNK_SIZE);

  return 0;
}
