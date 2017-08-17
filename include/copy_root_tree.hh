#ifndef COPY_ROOT_TREE_HH
#define COPY_ROOT_TREE_HH

#include <cstddef>

class TTree;

namespace H5 {
  class CommonFG;
}

void copy_root_tree(TTree& tt, H5::CommonFG& fg,
                    size_t length, size_t length2,
                    size_t chunk_size = 2048);

#endif
