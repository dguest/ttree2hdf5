#include "copy_root_tree.hh"
#include "HdfTuple.hh"

#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"

#include <memory>

#include <iostream>

class IBuffer
{
public:
  virtual ~IBuffer() {}
};

template <typename T>
class Buffer: public IBuffer
{
public:
  Buffer(VariableFillers& vars, TTree& tt, const std::string& name);
private:
  T _buffer;
};
template <typename T>
Buffer<T>::Buffer(VariableFillers& vars, TTree& tt,
                  const std::string& name)
{
  tt.SetBranchAddress(name.c_str(), &_buffer);
  T& buf = _buffer;
  vars.add<T>(name, [&buf](){return buf;});
}


// main function to do all the things
void copy_root_tree(TTree& tt, H5::CommonFG& fg) {
  const std::string tree_name = tt.GetName();

  std::vector<std::unique_ptr<IBuffer> > buffer_vec;
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
      buffer_vec.emplace_back(new Buffer<int>(vars, tt, leaf_name));
    } else if (leaf_type == "Float_t") {
      buffer_vec.emplace_back(new Buffer<float>(vars, tt, leaf_name));
    } else if (leaf_type == "Double_t") {
      buffer_vec.emplace_back(new Buffer<double>(vars, tt, leaf_name));
    } else if (leaf_type == "Bool_t") {
      buffer_vec.emplace_back(new Buffer<bool>(vars, tt, leaf_name));
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
