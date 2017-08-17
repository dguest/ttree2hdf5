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
  VBuf(VariableFillers& vars, std::vector<size_t>& idx, TTree& tt,
       const std::string& name, T default_value = T());
  ~VBuf();
private:
  std::vector<T>* _buffer;
};
template <typename T>
VBuf<T>::VBuf(VariableFillers& vars, std::vector<size_t>& idx, TTree& tt,
              const std::string& name, T default_value):
  _buffer(new std::vector<T>)
{
  tt.SetBranchAddress(name.c_str(), &_buffer);
  std::vector<T>& buf = *_buffer;
  auto filler = [&buf, &idx, default_value](){
    if (idx.at(0) < buf.size()) {
      return buf.at(idx.at(0));
    } else {
      return default_value;
    }
  };
  vars.add<T>(name, filler);
}
template <typename T>
VBuf<T>::~VBuf() {
  delete _buffer;
}

template <typename T>
class VVBuf: public IBuffer
{
public:
  VVBuf(VariableFillers& vars, std::vector<size_t>& idx, TTree& tt,
       const std::string& name, T default_value = T());
  ~VVBuf();
private:
  std::vector<std::vector<T> >* _buffer;
};
template <typename T>
VVBuf<T>::VVBuf(VariableFillers& vars, std::vector<size_t>& idx, TTree& tt,
                const std::string& name, T default_value):
  _buffer(new std::vector<std::vector<T> >)
{
  tt.SetBranchAddress(name.c_str(), &_buffer);
  std::vector<std::vector<T> >& buf = *_buffer;
  auto filler = [&buf, &idx, default_value](){
    size_t idx1 = idx.at(0);
    size_t idx2 = idx.at(1);
    if (idx1 < buf.size()) {
      if (idx2 < buf.at(idx1).size()) {
        return buf.at(idx1).at(idx2);
      }
    }
    return default_value;
  };
  vars.add<T>(name, filler);
}
template <typename T>
VVBuf<T>::~VVBuf() {
  delete _buffer;
}



// main function to do all the things
void copy_root_tree(TTree& tt, H5::CommonFG& fg,
                    size_t length, size_t length2,
                    size_t chunk_size) {
  const std::string tree_name = tt.GetName();

  std::vector<std::unique_ptr<IBuffer> > buffers;
  std::set<std::string> skipped;

  TIter next(tt.GetListOfLeaves());
  TLeaf* leaf;
  std::set<std::string> leaf_names;
  while ((leaf = dynamic_cast<TLeaf*>(next()))) {
    leaf_names.insert(leaf->GetName());
  }
  // 1d vars
  VariableFillers vars;

  // 2d vars
  VariableFillers vars2d;
  std::vector<size_t> idx(1,0);

  // 3d vars
  VariableFillers vars3d;
  std::vector<size_t> idx2(2,0);
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
    } else if (leaf_type == "vector<vector<int> >") {
      buffers.emplace_back(new VVBuf<int>(vars3d, idx2, tt, leaf_name, 0));
    } else if (leaf_type == "vector<vector<float> >") {
      buffers.emplace_back(new VVBuf<float>(vars3d, idx2, tt, leaf_name, NAN));
    } else {
      skipped.insert(leaf_type);
    }
  }

  // build outputs
  std::unique_ptr<Writer> writer1d;
  std::unique_ptr<WriterXd> writer2d;
  std::unique_ptr<WriterXd> writer3d;
  std::unique_ptr<H5::Group> top_group;
  if (length > 0) {
    top_group.reset(new H5::Group(fg.createGroup(tree_name)));
    if (length2 > 0) {
      writer3d.reset(new WriterXd(*top_group, "3d", vars3d,
                                  {length, length2}, chunk_size));
    }
    writer2d.reset(new WriterXd(*top_group, "2d", vars2d,
                                {length}, chunk_size));
    writer1d.reset(new Writer(*top_group, "1d", vars, chunk_size));
  } else {
    writer1d.reset(new Writer(fg, tree_name, vars, chunk_size));
  }

  // fill outputs
  size_t n_entries = tt.GetEntries();
  for (size_t iii = 0; iii < n_entries; iii++) {
    tt.GetEntry(iii);
    writer1d->fill();
    if (writer2d) writer2d->fill_while_incrementing(idx);
    if (writer3d) writer3d->fill_while_incrementing(idx2);
  }
  writer1d->flush();
  writer1d->close();
  if (writer2d) {
    writer2d->flush();
    writer2d->close();
  }
  if (writer3d) {
    writer3d->flush();
    writer3d->close();
  }

  for (const auto& name: skipped) {
    std::cerr << "skipped " << name << std::endl;
  }
}
