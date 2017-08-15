#include "HdfTuple.hh"

#include "H5Cpp.h"


template<>
data_buffer_t get_buffer(const std::function<int()>& func) {
  return {._int = func()};
}

template<>
data_buffer_t get_buffer(const std::function<float()>& func) {
  return {._float = func()};
}
template<>
data_buffer_t get_buffer(const std::function<double()>& func) {
  return {._double = func()};
}
template<>
data_buffer_t get_buffer(const std::function<bool()>& func) {
  return {._bool = func()};
}

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



namespace {

  // packing utility
  H5::CompType packed(H5::CompType in) {
    // TODO: Figure out why a normal copy constructor doesn't work here.
    //       The normal one seems to create shallow copies.
    auto out = H5::CompType(H5Tcopy(in.getId()));
    out.pack();
    return out;
  }
}

// 1d writter
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
  size_t dt_offset = 0;
  for (const auto& filler: fillers) {
    _type.insertMember((filler)->name(), dt_offset, (filler)->get_type());
    dt_offset += sizeof(data_buffer_t);
  }

  // create ds
  _ds = group.createDataSet(name, packed(_type), space, params);
}

Writer::~Writer() {
  // for (auto filler: _fillers) {
  //   delete filler;
  // }
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
