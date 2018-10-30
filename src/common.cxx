#include "HdfTuple.hh"

#include "H5Cpp.h"

#include <cassert>
#include <iostream>             // for printing errors in the destructor


namespace H5Utils {
  namespace internal {
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
    H5::DataSpace getUnlimitedSpace(const std::vector<hsize_t>& extent) {
      std::vector<hsize_t> initial{0};
      initial.insert(initial.end(), extent.begin(), extent.end());
      std::vector<hsize_t> eventual{H5S_UNLIMITED};
      eventual.insert(eventual.end(), extent.begin(), extent.end());
      return H5::DataSpace(eventual.size(), initial.data(), eventual.data());
    }
    H5::DSetCreatPropList getChunckedDatasetParams(
      const std::vector<hsize_t>& extent,
      hsize_t batch_size,
      const H5::CompType& type,
      const std::vector<data_buffer_t>& default_value) {
      H5::DSetCreatPropList params;
      std::vector<hsize_t> chunk_size{batch_size};
      chunk_size.insert(chunk_size.end(), extent.begin(), extent.end());
      params.setChunk(chunk_size.size(), chunk_size.data());
      params.setFillValue(type, default_value.data());
      params.setDeflate(7);
      return params;
    }
    std::vector<hsize_t> getStriding(std::vector<hsize_t> extent) {
      // calculate striding
      extent.push_back(1);
      for (size_t iii = extent.size(); iii - 1 != 0; iii--) {
        extent.at(iii-2) = extent.at(iii-2) * extent.at(iii-1);
      }
      return extent;
    }
    void throwIfExists(const std::string& name, const H5::Group& in_group) {
      if (H5Lexists(in_group.getLocId(), name.c_str(), H5P_DEFAULT)) {
        throw std::logic_error("tried to overwrite '" + name + "'");
      }
    }

    // DS parameters
    DSParameters::DSParameters(const H5::CompType type_,
                               std::vector<hsize_t> extent_,
                               hsize_t batch_size_):
      type(type_),
      extent(extent_),
      dim_stride(getStriding(extent_)),
      batch_size(batch_size_)
    {
    }


  }

}
