#ifndef TREE_COPY_OPTS_HH
#define TREE_COPY_OPTS_HH

#include <string>
#include <vector>

const size_t CHUNK_SIZE = 128;

struct TreeCopyOpts
{
  std::string branch_regex;
  std::vector<size_t> vector_lengths;
  size_t chunk_size;
  size_t n_entries;
  bool verbose;
};

struct IOOpts
{
  std::string in;
  std::string out;
  std::string tree;
};

std::tuple<TreeCopyOpts, IOOpts> get_tree_copy_opts(
  int argc, const char* argv[]);

#endif
