#include "HdfTuple.hh"

#include "H5Cpp.h"

#include <cassert>
#include <functional>
#include <numeric>

template<>
H5::DataType get_type<float>() {
  return H5::PredType::NATIVE_FLOAT;
}
template<>
H5::DataType get_type<int>() {
  return H5::PredType::NATIVE_INT;
}
template<>
H5::DataType get_type<double>() {
  return H5::PredType::NATIVE_DOUBLE;
}
template<>
H5::DataType get_type<bool>() {
  bool TRUE = true;
  bool FALSE = false;
  H5::EnumType btype(sizeof(bool));
  btype.insert("TRUE", &TRUE);
  btype.insert("FALSE", &FALSE);
  return btype;
}

template<> int& get_ref<int>(data_buffer_t& buf) {
  return buf._int;
}
template<> float& get_ref<float>(data_buffer_t& buf) {
  return buf._float;
}
template<> double& get_ref<double>(data_buffer_t& buf) {
  return buf._double;
}
template<> bool& get_ref<bool>(data_buffer_t& buf) {
  return buf._bool;
}



namespace {

  // packing utility
  H5::CompType packed(H5::CompType in) {
    // TODO: Figure out why a normal copy constructor doesn't work here.
    //       The normal one seems to create shallow copies.
    auto out = H5::CompType(H5Tcopy(in.getId()));
    out.pack();
    return out;
  }
  void build_type(H5::CompType& type, const VariableFillers& fillers) {
    size_t dt_offset = 0;
    for (const auto& filler: fillers) {
      type.insertMember(filler->name(), dt_offset, filler->get_type());
      dt_offset += sizeof(data_buffer_t);
    }
  }
}

// _______________________________________________________________________
// 1d writter
//
Writer::Writer(H5::CommonFG& group, const std::string& name,
               VariableFillers fillers, hsize_t batch_size):
  _type(fillers.size() * sizeof(data_buffer_t)),
  _batch_size(batch_size),
  _offset(0),
  _fillers(fillers)
{
  if (batch_size < 1) {
    throw std::logic_error("batch size must be > 0");
  }
  // create space
  hsize_t initial[1] = {0};
  hsize_t eventual[1] = {H5S_UNLIMITED};
  H5::DataSpace space(1, initial, eventual);

  // create params
  H5::DSetCreatPropList params;
  hsize_t chunk_size[1] = {batch_size};
  params.setChunk(1, chunk_size);
  params.setDeflate(7);

  // build up type
  build_type(_type, fillers);

  // create ds
  _ds = group.createDataSet(name, packed(_type), space, params);
}

void Writer::fill() {
  if (buffer_size() == _batch_size) {
    flush();
  }
  // todo: make sure we don't insert unless everything is pushed
  for (const auto& filler: _fillers) {
    _buffer.push_back((filler)->get_buffer());
  }
}

void Writer::flush() {
  if (buffer_size() == 0) return;
  // extend the ds
  hsize_t slab_dims[1] = {buffer_size()};
  hsize_t total_dims[1] = {buffer_size() + _offset};
  _ds.extend(total_dims);

  // setup dataspaces
  H5::DataSpace file_space = _ds.getSpace();
  H5::DataSpace mem_space(1, slab_dims);
  hsize_t offset_dims[1] = {_offset};
  file_space.selectHyperslab(H5S_SELECT_SET, slab_dims, offset_dims);

  // write out
  _ds.write(_buffer.data(), _type, mem_space, file_space);
  _offset += buffer_size();
  _buffer.clear();
}
void Writer::close() {
  _ds.close();
}
hsize_t Writer::buffer_size() const {
  return _buffer.size() / _fillers.size();
}


// _______________________________________________________________________
// 2d writter
//


WriterXd::WriterXd(H5::CommonFG& group, const std::string& name,
                   VariableFillers fillers,
                   std::vector<hsize_t> max_length,
                   hsize_t batch_size):
  _type(fillers.size() * sizeof(data_buffer_t)),
  _max_length(max_length),
  _dim_stride(max_length),
  _batch_size(batch_size),
  _offset(0),
  _fillers(fillers)
{
  if (batch_size < 1) {
    throw std::logic_error("batch size must be > 0");
  }
  // create space
  std::vector<hsize_t> initial{0};
  initial.insert(initial.end(), max_length.begin(), max_length.end());
  std::vector<hsize_t> eventual{H5S_UNLIMITED};
  eventual.insert(eventual.end(), max_length.begin(), max_length.end());
  H5::DataSpace space(eventual.size(), initial.data(), eventual.data());

  // create params
  H5::DSetCreatPropList params;
  std::vector<hsize_t> chunk_size{batch_size};
  chunk_size.insert(chunk_size.end(), max_length.begin(), max_length.end());
  params.setChunk(chunk_size.size(), chunk_size.data());
  params.setDeflate(7);

  // calculate striding
  std::accumulate(max_length.begin(), max_length.end(),
                  1, std::multiplies<hsize_t>());

  // build up type
  build_type(_type, fillers);

  // create ds
  _ds = group.createDataSet(name, packed(_type), space, params);
}

// TODO: figure out if we really need this `size` here. Instead we
// could just increment to the max length and let the getter functions
// sort out the "out of range" values.
void WriterXd::fill_while_incrementing(size_t& index, const size_t& size) {
  if (buffer_size() == _batch_size) {
    flush();
  }
  // todo: build buffer and _then_ insert it so that exceptions
  // don't leave the buffer in a weird state
  for (index = 0; index < _max_length.at(0); index++) {
    if (index < size) {
      for (const auto& filler: _fillers) {
        _buffer.push_back(filler->get_buffer());
      }
    } else {
      for (const auto& filler: _fillers) {
        _buffer.push_back(filler->get_empty());
      }
    }
  }
}
void WriterXd::flush() {
  if (buffer_size() == 0) return;
  // extend the ds
  std::vector<hsize_t> slab_dims{buffer_size()};
  slab_dims.insert(slab_dims.end(), _max_length.begin(), _max_length.end());
  std::vector<hsize_t> total_dims{buffer_size() + _offset};
  total_dims.insert(total_dims.end(), _max_length.begin(), _max_length.end());
  _ds.extend(total_dims.data());

  // setup dataspaces
  H5::DataSpace file_space = _ds.getSpace();
  H5::DataSpace mem_space(slab_dims.size(), slab_dims.data());
  std::vector<hsize_t> offset_dims{_offset};
  offset_dims.resize(1 + slab_dims.size(), 0);
  file_space.selectHyperslab(H5S_SELECT_SET,
                             slab_dims.data(), offset_dims.data());

  // write out
  _ds.write(_buffer.data(), _type, mem_space, file_space);
  _offset += buffer_size();
  _buffer.clear();
}
void WriterXd::close() {
  _ds.close();
}

hsize_t WriterXd::buffer_size() const {
  size_t n_entries = _buffer.size() / _fillers.size();
  assert(n_entries % _dim_stride.back() == 0);
  return n_entries / _dim_stride.back();
}

