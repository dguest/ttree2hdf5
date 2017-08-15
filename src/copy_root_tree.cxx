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

// functions to convert union types back to whatever they are
template <typename T>
T& get_ref(data_buffer_t* buf);
template<> int& get_ref<int>(data_buffer_t* buf) {
  return buf->_int;
}
template<> float& get_ref<float>(data_buffer_t* buf) {
  return buf->_float;
}
template<> double& get_ref<double>(data_buffer_t* buf) {
  return buf->_double;
}
template<> bool& get_ref<bool>(data_buffer_t* buf) {
  return buf->_bool;
}

// copy function
template <typename T>
void set_branch(VariableFillers& vars, TTree& tt, DataBuffers& buffer,
                const std::string& name) {
  buffer.push_back(new data_buffer_t);
  T& buf = get_ref<T>(buffer.back());
  tt.SetBranchAddress(name.c_str(), &buf);
  vars.add<T>(name, [&buf](){return buf;});
}

// main function to do all the things
void copy_root_tree(TTree& tt, H5::CommonFG& fg) {
  const std::string tree_name = tt.GetName();

  DataBuffers buffer;
  std::set<std::string> skipped;

  TIter next(tt.GetListOfLeaves());
  VariableFillers vars;
  TLeaf* leaf;
  while ((leaf = dynamic_cast<TLeaf*>(next()))) {
    std::string leaf_name = leaf->GetName();
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
