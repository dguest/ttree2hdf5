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

template <typename T>
class VBuf: public IBuffer
{
public:
  VBuf(VariableFillers& vars, size_t& idx, TTree& tt,
       const std::string& name, T default_value = T());
private:
  std::vector<T>* _buffer;
};
template <typename T>
VBuf<T>::VBuf(VariableFillers& vars, size_t& idx, TTree& tt,
              const std::string& name, T default_value):
  _buffer(new std::vector<T>)
{
  tt.SetBranchAddress(name.c_str(), &_buffer);
  std::vector<T>& buf = *_buffer;
  auto filler = [&buf, &idx, default_value](){
    if (idx < buf.size()) {
      return buf.at(idx);
    } else {
      return default_value;
    }
  };
  vars.add<T>(name, filler);
}



// main function to do all the things
void copy_root_tree(TTree& tt, H5::CommonFG& fg,
                    size_t length, size_t chunk_size) {
  const std::string tree_name = tt.GetName();

  std::vector<std::unique_ptr<IBuffer> > buffers;
  std::set<std::string> skipped;

  TIter next(tt.GetListOfLeaves());
  TLeaf* leaf;
  std::set<std::string> leaf_names;
  while ((leaf = dynamic_cast<TLeaf*>(next()))) {
    leaf_names.insert(leaf->GetName());
  }
  VariableFillers vars;
  VariableFillers vars2d;
  size_t idx = 0;
  for (const auto& leaf_name: leaf_names) {
    leaf = tt.GetLeaf(leaf_name.c_str());
    std::string leaf_type = leaf->GetTypeName();
    if (leaf_type == "Int_t") {
      buffers.emplace_back(new Buffer<int>(vars, tt, leaf_name));
    } else if (leaf_type == "Float_t") {
      buffers.emplace_back(new Buffer<float>(vars, tt, leaf_name));
    } else if (leaf_type == "Double_t") {
      buffers.emplace_back(new Buffer<double>(vars, tt, leaf_name));
    } else if (leaf_type == "Bool_t") {
      buffers.emplace_back(new Buffer<bool>(vars, tt, leaf_name));
    } else if (leaf_type == "vector<float>") {
      buffers.emplace_back(new VBuf<float>(vars2d, idx, tt, leaf_name, NAN));
    } else if (leaf_type == "vector<double>") {
      buffers.emplace_back(new VBuf<double>(vars2d, idx, tt, leaf_name, NAN));
    } else if (leaf_type == "vector<int>") {
      buffers.emplace_back(new VBuf<int>(vars2d, idx, tt, leaf_name, 0));
    } else {
      skipped.insert(leaf_type);
    }
  }

  // build outputs
  std::unique_ptr<Writer> writer1d;
  std::unique_ptr<Writer2d> writer2d;
  std::unique_ptr<H5::Group> top_group;
  if (length > 0) {
    top_group.reset(new H5::Group(fg.createGroup(tree_name)));
    writer2d.reset(new Writer2d(*top_group, "2d", vars2d,
                                length, chunk_size));
    writer1d.reset(new Writer(*top_group, "1d", vars, chunk_size));
  } else {
    writer1d.reset(new Writer(fg, tree_name, vars, chunk_size));
  }

  // fill outputs
  size_t n_entries = tt.GetEntries();
  for (size_t iii = 0; iii < n_entries; iii++) {
    tt.GetEntry(iii);
    writer1d->fill();
    if (writer2d) writer2d->fill_while_incrementing(idx, length);
  }
  writer1d->flush();
  writer1d->close();
  if (writer2d) {
    writer2d->flush();
    writer2d->close();
  }

  for (const auto& name: skipped) {
    std::cerr << "skipped " << name << std::endl;
  }
}
