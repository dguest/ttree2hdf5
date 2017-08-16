#include "copy_root_tree.hh"
#include "HdfTuple.hh"

#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"

#include <iostream>

class DataBuffers: public std::vector<data_buffer_t*> {
public:
  ~DataBuffers() {
    for (auto buf : *this) {
      delete buf;
    }
  }
};


// copy function
template <typename T>
void set_branch(VariableFillers& vars, TTree& tt, DataBuffers& buffer,
                const std::string& name) {
  buffer.push_back(new data_buffer_t);
  T& buf = get_ref<T>(*buffer.back());
  tt.SetBranchAddress(name.c_str(), &buf);
  vars.add<T>(name, [&buf](){return buf;});
}

// main function to do all the things
void copy_root_tree(TTree& tt, H5::CommonFG& fg) {
  const std::string tree_name = tt.GetName();

  DataBuffers buffer;
  std::set<std::string> skipped;

  TIter next(tt.GetListOfLeaves());
  TLeaf* leaf;
  std::set<std::string> leaf_names;
  while ((leaf = dynamic_cast<TLeaf*>(next()))) {
    leaf_names.insert(leaf->GetName());
  }
  VariableFillers vars;
  for (const auto& leaf_name: leaf_names) {
    leaf = tt.GetLeaf(leaf_name.c_str());
    std::string leaf_type = leaf->GetTypeName();
    if (leaf_type == "Int_t") {
      set_branch<int>(vars, tt, buffer, leaf_name);
    } else if (leaf_type == "Float_t") {
      set_branch<float>(vars, tt, buffer, leaf_name);
    } else if (leaf_type == "Double_t") {
      set_branch<double>(vars, tt, buffer, leaf_name);
    } else if (leaf_type == "Bool_t") {
      set_branch<bool>(vars, tt, buffer, leaf_name);
    } else {
      skipped.insert(leaf_type);
    }
  }

  Writer writer(fg, tree_name, vars);
  size_t n_entries = tt.GetEntries();
  for (size_t iii = 0; iii < n_entries; iii++) {
    tt.GetEntry(iii);
    writer.fill();
  }
  writer.flush();
  writer.close();

  for (const auto& name: skipped) {
    std::cerr << "skipped " << name << std::endl;
  }
}
