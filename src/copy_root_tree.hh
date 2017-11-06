#ifndef COPY_ROOT_TREE_HH
#define COPY_ROOT_TREE_HH

#include "tree_copy_opts.hh"

class TTree;

namespace H5 {
  class Group;
}

void copy_root_tree(TTree& tt, H5::Group& fg, const TreeCopyOpts& opts);

#endif
