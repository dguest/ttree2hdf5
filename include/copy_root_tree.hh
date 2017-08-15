#ifndef COPY_ROOT_TREE_HH
#define COPY_ROOT_TREE_HH

class TTree;

namespace H5 {
  class CommonFG;
}

void copy_root_tree(TTree& tt, H5::CommonFG& fg);

#endif
