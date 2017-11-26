#include "copy_root_tree.hh"

// Root tree copy function
//
// NOTE: I don't recommend trying to understand the variable buffer
// classes until you see the general workflow.
//
// The main work is done in copy_root_tree below, skip down to that to
// understand what is going on.
//

#include "HdfTuple.hh"

#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"

#include <memory>
#include <regex>
#include <iostream>

// ______________________________________________________________________
// Variable buffer classes
//
// These classes act as glue between HDF and ROOT: they hold pointers
// to the buffers that ROOT will read into, and as such are
// "ROOT-side" buffers.
//
// The constructors are given a reference to a `VariableFillers`
// container, which contains the "HDF5-side" logic. Each of the
// VariableFiller objects contains a "filler" function that the HDF5
// writer will call to read from the ROOT buffer.
//
// Some of the filler functions close over an index vector. This
// vector is incremented by the HDF5 Writer to read out successive
// entires in a std::vector stored in the root file.
//
// If this all seems complicated, it's because it's the deep internals
// of the code. Try to understand the copy_root_tree function first.

// Base class, just to hold a virtual destructor
class IBuffer
{
public:
  virtual ~IBuffer() {}
};

// Simple variables types are stored in the `Buffer` class.
template <typename T>
class Buffer: public IBuffer
{
public:
  Buffer(VariableFillers& vars, TTree& tt, const std::string& name);
private:
  T _buffer;
};

// Buffer for vector types
template <typename T>
class VBuf: public IBuffer
{
public:
  // These require an index for the vector (`idx`)
  VBuf(VariableFillers& vars, std::vector<size_t>& idx, TTree& tt,
       const std::string& name, T default_value = T());
  ~VBuf();
private:
  std::vector<T>* _buffer;
};

// Buffer for vectors of vectors
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

// _____________________________________________________________________
// main copy root tree function
//
// The basic workflow is as follows:
//
//  - Define the ROOT-side buffers that we'll read the information
//    into. At the same time, define HDF-side objects that read out of
//    these buffers and copy the information into the HDF5 buffer.
//
//  - Build the output HDF5 files
//
//  - Loop over the input tree and fill the output files
//
// Note that this means the event loop happens last: most of the work
// is just setting up the read and write buffers.

void copy_root_tree(TTree& tt, H5::Group& fg, const TreeCopyOpts& opts) {

  // define the buffers for root to read into
  std::vector<std::unique_ptr<IBuffer> > buffers;

  // this keeps track of the things we couldn't read
  std::set<std::string> skipped;


  // Each `VariableFiller` must be constructed with a "filler"
  // function (or callable object), which takes no arguments and
  // returns the variable we want to write out. In this case they are
  // implemented as closures over the buffers that ROOT is reading
  // into.

  // This is the 1d variables
  VariableFillers vars;

  // These are 2d variables (i.e. vector<T> in the root file)
  //
  // We also need an index which the HDF5 writer increments as it
  // fills. This is shared with the ROOT buffers to index entries in
  // std::vectors
  VariableFillers vars2d;
  std::vector<size_t> idx(1,0);

  // 3d variables (index is now 2d)
  VariableFillers vars3d;
  std::vector<size_t> idx2(2,0);

  // Iterate over all the leaf names. There are some duplicates in the
  // list of keys, so we have to build the set ourselves.
  std::regex branch_filter(opts.branch_regex);
  TIter next(tt.GetListOfLeaves());
  TLeaf* leaf;
  std::set<std::string> leaf_names;
  while ((leaf = dynamic_cast<TLeaf*>(next()))) {
    leaf_names.insert(leaf->GetName());
  }
  if (leaf_names.size() == 0) throw std::logic_error("no branches found");

  // Loop over all the leafs, assign buffers to each
  //
  // These `Buffer` classes are defined above. The buffers turn the
  // branchs on, so we can set them all off to start.
  tt.SetBranchStatus("*", false);
  for (const auto& lname: leaf_names) {
    bool keep = true;
    if (opts.branch_regex.size() > 0) {
      keep = std::regex_search(lname, branch_filter);
    }
    if (opts.verbose) {
      std::cout << (keep ? "found " : "rejecting ") << lname << std::endl;
    }
    if (!keep) continue;

    leaf = tt.GetLeaf(lname.c_str());
    std::string leaf_type = leaf->GetTypeName();
    if (leaf_type == "Int_t") {
      buffers.emplace_back(new Buffer<int>(vars, tt, lname));
    } else if (leaf_type == "Float_t") {
      buffers.emplace_back(new Buffer<float>(vars, tt, lname));
    } else if (leaf_type == "Double_t") {
      buffers.emplace_back(new Buffer<double>(vars, tt, lname));
    } else if (leaf_type == "Bool_t") {
      buffers.emplace_back(new Buffer<bool>(vars, tt, lname));
    } else if (leaf_type == "Long64_t") {
      buffers.emplace_back(new Buffer<long long>(vars, tt, lname));
    } else if (leaf_type == "UInt_t") {
      buffers.emplace_back(new Buffer<unsigned int>(vars, tt, lname));
    } else if (leaf_type == "vector<float>") {
      buffers.emplace_back(new VBuf<float>(vars2d, idx, tt, lname, NAN));
    } else if (leaf_type == "vector<double>") {
      buffers.emplace_back(new VBuf<double>(vars2d, idx, tt, lname, NAN));
    } else if (leaf_type == "vector<int>") {
      buffers.emplace_back(new VBuf<int>(vars2d, idx, tt, lname, 0));
    } else if (leaf_type == "vector<unsigned int>") {
      buffers.emplace_back(new VBuf<unsigned int>(vars2d, idx, tt, lname, 0));
    } else if (leaf_type == "vector<bool>") {
      buffers.emplace_back(new VBuf<bool>(vars2d, idx, tt, lname, 0));
    } else if (leaf_type == "vector<vector<int> >") {
      buffers.emplace_back(new VVBuf<int>(vars3d, idx2, tt, lname, 0));
    } else if (leaf_type == "vector<vector<unsigned int> >") {
      buffers.emplace_back(new VVBuf<unsigned int>(vars3d, idx2, tt, lname, 0));
    } else if (leaf_type == "vector<vector<float> >") {
      buffers.emplace_back(new VVBuf<float>(vars3d, idx2, tt, lname, NAN));
    } else if (leaf_type == "vector<vector<double> >") {
      buffers.emplace_back(new VVBuf<double>(vars3d, idx2, tt, lname, NAN));
    } else if (leaf_type == "vector<vector<bool> >") {
      buffers.emplace_back(new VVBuf<bool>(vars3d, idx2, tt, lname, 0));
    } else {
      skipped.insert(leaf_type);
    }
  }

  // Build HDF5 Outputs
  //
  // In the simple case where we're not reading vectors, we store one
  // dataset with the same name as the tree. If there are vectors, we
  // instead create a group with the same name as the tree, and name
  // the datasets 1d, 2d, etc.
  //
  const std::string tree_name = tt.GetName();

  std::unique_ptr<WriterXd> writer1d;
  std::unique_ptr<WriterXd> writer2d;
  std::unique_ptr<WriterXd> writer3d;
  std::unique_ptr<H5::Group> top_group;
  if (opts.vector_lengths.size() > 0) {
    if (opts.vector_lengths.size() > 2) throw std::logic_error(
      "we don't support outputs with rank > 3");
    size_t length = opts.vector_lengths.at(0);
    top_group.reset(new H5::Group(fg.createGroup(tree_name)));
    if (opts.vector_lengths.size() > 1) {
      size_t length2 = opts.vector_lengths.at(1);
      if (vars3d.size() > 0) {
        writer3d.reset(new WriterXd(*top_group, "3d", vars3d,
                                    {length, length2}, opts.chunk_size));
      }
    }
    if (vars2d.size() > 0) {
      writer2d.reset(new WriterXd(*top_group, "2d", vars2d,
                                  {length}, opts.chunk_size));
    }
    if (vars.size() > 0) {
      writer1d.reset(new WriterXd(*top_group, "1d",
                                  vars, {}, opts.chunk_size));
    }
  } else {
    if (vars.size() > 0) {
      writer1d.reset(new WriterXd(fg, tree_name, vars, {}, opts.chunk_size));
    }
  }

  // Main event loop
  //
  // Very little actually happens here since the buffers are already
  // defined, as are the HDF5 reader functions.
  //
  size_t n_entries = tt.GetEntries();
  for (size_t iii = 0; iii < n_entries; iii++) {
    if (opts.n_entries && iii >= opts.n_entries) break;
    tt.GetEntry(iii);
    if (writer1d) writer1d->fill_while_incrementing();
    if (writer2d) writer2d->fill_while_incrementing(idx);
    if (writer3d) writer3d->fill_while_incrementing(idx2);
  }

  // Flush the memory buffers on the HDF5 side. (This is done by the
  // destructor automatically, but we do it here to make any errors
  // more explicit.)
  if (writer1d) writer1d->flush();
  if (writer2d) writer2d->flush();
  if (writer3d) writer3d->flush();

  // Print the names of any classes that we were't able to read from
  // the root file.
  if (opts.verbose) {
    for (const auto& name: skipped) {
      std::cerr << "could not read branch of type " << name << std::endl;
    }
  }
}


// ______________________________________________________________________
// Buffer implementation

// 1d buffer
template <typename T>
Buffer<T>::Buffer(VariableFillers& vars, TTree& tt,
                  const std::string& name)
{
  tt.SetBranchStatus(name.c_str(), true);
  tt.SetBranchAddress(name.c_str(), &_buffer);
  T& buf = _buffer;
  vars.add<T>(name, [&buf](){return buf;});
}

// 2d buffer
template <typename T>
VBuf<T>::VBuf(VariableFillers& vars, std::vector<size_t>& idx, TTree& tt,
              const std::string& name, T default_value):
  _buffer(new std::vector<T>)
{
  tt.SetBranchStatus(name.c_str(), true);
  tt.SetBranchAddress(name.c_str(), &_buffer);
  std::vector<T>& buf = *_buffer;
  auto filler = [&buf, &idx, default_value]() -> T {
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


// 3d buffers
template <typename T>
VVBuf<T>::VVBuf(VariableFillers& vars, std::vector<size_t>& idx, TTree& tt,
                const std::string& name, T default_value):
  _buffer(new std::vector<std::vector<T> >)
{
  tt.SetBranchStatus(name.c_str(), true);
  tt.SetBranchAddress(name.c_str(), &_buffer);
  std::vector<std::vector<T> >& buf = *_buffer;
  auto filler = [&buf, &idx, default_value]() -> T {
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

