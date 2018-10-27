#include "HdfTuple.hh"

#include "H5Cpp.h"

#include <cassert>
#include <iostream>             // for printing errors in the destructor

template<>
H5::DataType get_type<float>() {
  return H5::PredType::NATIVE_FLOAT;
}
template<>
H5::DataType get_type<int>() {
  return H5::PredType::NATIVE_INT;
}
template<>
H5::DataType get_type<long long>() {
  return H5::PredType::NATIVE_LLONG;
}
template<>
H5::DataType get_type<unsigned int>() {
  return H5::PredType::NATIVE_UINT;
}
template<>
H5::DataType get_type<unsigned char>() {
  return H5::PredType::NATIVE_UCHAR;
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
template<> long long& get_ref<long long>(data_buffer_t& buf) {
  return buf._llong;
}
template<> unsigned int& get_ref<unsigned int>(data_buffer_t& buf) {
  return buf._uint;
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
  H5::CompType build_type(const VariableFillers& fillers) {
    H5::CompType type(fillers.size() * sizeof(data_buffer_t));
    size_t dt_offset = 0;
    for (const auto& filler: fillers) {
      type.insertMember(filler->name(), dt_offset, filler->get_type());
      dt_offset += sizeof(data_buffer_t);
    }
    return type;
  }

  void print_destructor_error(const std::string& msg) {
    std::cerr << "ERROR: an exception was thrown in the destructor of an "
      "HDF5 file, the output buffer may be corrupted";
    std::cerr << " (error message: " << msg << ")" << std::endl;
  }

  // new functions
  H5::DataSpace getUnlimitedSpace(const std::vector<hsize_t>& max_length) {
    std::vector<hsize_t> initial{0};
    initial.insert(initial.end(), max_length.begin(), max_length.end());
    std::vector<hsize_t> eventual{H5S_UNLIMITED};
    eventual.insert(eventual.end(), max_length.begin(), max_length.end());
    return H5::DataSpace(eventual.size(), initial.data(), eventual.data());
  }
  H5::DSetCreatPropList getChunckedDatasetParams(
    const std::vector<hsize_t>& max_length,
    hsize_t batch_size) {
    H5::DSetCreatPropList params;
    std::vector<hsize_t> chunk_size{batch_size};
    chunk_size.insert(chunk_size.end(), max_length.begin(), max_length.end());
    params.setChunk(chunk_size.size(), chunk_size.data());
    params.setDeflate(7);
    return params;
  }
  std::vector<hsize_t> getStriding(std::vector<hsize_t> max_length) {
    // calculate striding
    max_length.push_back(1);
    for (size_t iii = max_length.size(); iii - 1 != 0; iii--) {
      max_length.at(iii-2) = max_length.at(iii-2) * max_length.at(iii-1);
    }
    return max_length;
  }
  void throwIfExists(const std::string& name, const H5::Group& in_group) {
    if (H5Lexists(in_group.getLocId(), name.c_str(), H5P_DEFAULT)) {
      throw std::logic_error("tried to overwrite '" + name + "'");
    }
  }
}

// _______________________________________________________________________
// Xd writter
//

DSParameters::DSParameters(const H5::CompType type_,
                           std::vector<hsize_t> max_length_,
                           hsize_t batch_size_):
  type(type_),
  max_length(max_length_),
  dim_stride(getStriding(max_length_)),
  batch_size(batch_size_)
{
}

std::vector<size_t> WriterXd::NONE = {};

WriterXd::WriterXd(H5::Group& group, const std::string& name,
                   VariableFillers fillers,
                   std::vector<hsize_t> max_length,
                   hsize_t batch_size):
  _pars(build_type(fillers), max_length, batch_size),
  _offset(0),
  _fillers(fillers)
{
  if (batch_size < 1) {
    throw std::logic_error("batch size must be > 0");
  }
  // create space
  H5::DataSpace space = getUnlimitedSpace(max_length);

  // create params
  H5::DSetCreatPropList params = getChunckedDatasetParams(
    max_length, batch_size);

  // create ds
  throwIfExists(name, group);
  _ds = group.createDataSet(name, packed(_pars.type), space, params);
}

WriterXd::~WriterXd() {
  try {
    flush();
  } catch (H5::Exception& err) {
    print_destructor_error(err.getDetailMsg());
  } catch (std::exception& err) {
    print_destructor_error(err.what());
  }
}

void WriterXd::fill_while_incrementing(std::vector<size_t>& indices) {
  if (buffer_size() == _pars.batch_size) {
    flush();
  }
  indices.resize(_pars.max_length.size());

  // build buffer and _then_ insert it so that exceptions don't leave
  // the buffer in a weird state
  std::vector<data_buffer_t> temp;

  std::fill(indices.begin(), indices.end(), 0);
  for (size_t gidx = 0; gidx < _pars.dim_stride.front(); gidx++) {

    // we might be able to make this more efficient and less cryptic
    for (size_t iii = 0; iii < indices.size(); iii++) {
      indices.at(iii) = (
        gidx % _pars.dim_stride.at(iii)) / _pars.dim_stride.at(iii+1);
    }

    for (const auto& filler: _fillers) {
      temp.push_back(filler->get_buffer());
    }
  }
  _buffer.insert(_buffer.end(), temp.begin(), temp.end());
}
void WriterXd::flush() {
  if (buffer_size() == 0) return;
  // extend the ds
  std::vector<hsize_t> slab_dims{buffer_size()};
  slab_dims.insert(slab_dims.end(),
                   _pars.max_length.begin(),
                   _pars.max_length.end());
  std::vector<hsize_t> total_dims{buffer_size() + _offset};
  total_dims.insert(total_dims.end(),
                    _pars.max_length.begin(),
                    _pars.max_length.end());
  _ds.extend(total_dims.data());

  // setup dataspaces
  H5::DataSpace file_space = _ds.getSpace();
  H5::DataSpace mem_space(slab_dims.size(), slab_dims.data());
  std::vector<hsize_t> offset_dims{_offset};
  offset_dims.resize(slab_dims.size(), 0);
  file_space.selectHyperslab(H5S_SELECT_SET,
                             slab_dims.data(), offset_dims.data());

  // write out
  assert(static_cast<size_t>(file_space.getSelectNpoints())
         == _buffer.size() / _fillers.size());
  _ds.write(_buffer.data(), _pars.type, mem_space, file_space);
  _offset += buffer_size();
  _buffer.clear();
}

hsize_t WriterXd::buffer_size() const {
  assert(_fillers.size() == _pars.type.getNmembers());
  size_t n_entries = _buffer.size() / _fillers.size();
  assert(n_entries % _pars.dim_stride.front() == 0);
  return n_entries / _pars.dim_stride.front();
}

