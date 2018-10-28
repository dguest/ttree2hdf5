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



namespace H5Utils {

  // packing utility
  H5::CompType packed(H5::CompType in) {
    // TODO: Figure out why a normal copy constructor doesn't work here.
    //       The normal one seems to create shallow copies.
    auto out = H5::CompType(H5Tcopy(in.getId()));
    out.pack();
    return out;
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
    hsize_t batch_size,
    const H5::CompType& type,
    const std::vector<data_buffer_t>& default_value) {
    H5::DSetCreatPropList params;
    std::vector<hsize_t> chunk_size{batch_size};
    chunk_size.insert(chunk_size.end(), max_length.begin(), max_length.end());
    params.setChunk(chunk_size.size(), chunk_size.data());
    params.setFillValue(type, default_value.data());
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
  dim_stride(H5Utils::getStriding(max_length_)),
  batch_size(batch_size_)
{
}

