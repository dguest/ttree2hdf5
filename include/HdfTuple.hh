#ifndef HDF_TUPLE_HH
#define HDF_TUPLE_HH

// HDF5 Tuple Writer
//
// Skip down to the `WriterXd` and `VariableFillers` classes below to
// see the stuff that you'll have to interact with.

#include "H5Cpp.h"

#include <functional>
#include <vector>
#include <memory>
#include <cassert>

// buffer used by the HDF5 library to store data which is about to be
// written to disk
union data_buffer_t
{
  int _int;
  long long _llong;
  unsigned int _uint;
  unsigned char _uchar;
  float _float;
  double _double;
  bool _bool;
};

// _____________________________________________________________________
// internal classes
//
// We have lots of code to get around HDF5's rather weak typing. These
// templates are defined in the cxx file.

template <typename T> H5::DataType get_type();
template<> H5::DataType get_type<int>();
template<> H5::DataType get_type<long long>();
template<> H5::DataType get_type<unsigned int>();
template<> H5::DataType get_type<unsigned char>();
template<> H5::DataType get_type<float>();
template<> H5::DataType get_type<double>();
template<> H5::DataType get_type<bool>();

// check to make sure one of the above specializations is used
template<typename T>
H5::DataType get_type() {
  static_assert(sizeof(T) != sizeof(T), "you must override this class");
  return H5::DataType();
}

template <typename T>
T& get_ref(data_buffer_t& buf);
template<> int& get_ref<int>(data_buffer_t& buf);
template<> long long& get_ref<long long>(data_buffer_t& buf);
template<> unsigned int& get_ref<unsigned int>(data_buffer_t& buf);
template<> float& get_ref<float>(data_buffer_t& buf);
template<> double& get_ref<double>(data_buffer_t& buf);
template<> bool& get_ref<bool>(data_buffer_t& buf);

// check to make sure one of the above specializations is used
template <typename T>
T& get_ref(data_buffer_t& buf) {
  static_assert(sizeof(T) != sizeof(T), "you must override this class");
  return T();
}

// This is used to buld the unions we use to build the hdf5 memory
// buffer. Together with the `get_type` templates it gives us a
// strongly typed HDF5 writer.

// variable filler class header
template <typename... I>
class IVariableFiller
{
public:
  virtual ~IVariableFiller() {}
  virtual data_buffer_t get_buffer(I...) const = 0;
  virtual H5::DataType get_type() const = 0;
  virtual std::string name() const = 0;
};

// implementation for variable filler
template <typename T, typename... I>
class VariableFiller: public IVariableFiller<I...>
{
public:
  VariableFiller(const std::string&, const std::function<T(I...)>&);
  data_buffer_t get_buffer(I...) const;
  H5::DataType get_type() const;
  std::string name() const;
private:
  std::function<T(I...)> _getter;
  std::string _name;
};
template <typename T, typename... I>
VariableFiller<T, I...>::VariableFiller(const std::string& name,
                                  const std::function<T(I...)>& func):
  _getter(func),
  _name(name)
{
}
template <typename T, typename... I>
data_buffer_t VariableFiller<T, I...>::get_buffer(I... args) const {
  data_buffer_t buffer;
  get_ref<T>(buffer) = _getter(args...);
  return buffer;
}
template <typename T, typename... I>
H5::DataType VariableFiller<T, I...>::get_type() const {
  return ::get_type<T>();
}
template <typename T, typename... I>
std::string VariableFiller<T, I...>::name() const {
  return _name;
}



// _________________________________________________________________________
// The class that holds the variable fillers
//
// This is what you actually interact with. You need to give each
// variable a name and a function that fills the variable. Note that
// these functions have no inputs, but they can close over whatever
// buffers you want to read from.
//
// For examples, see `copy_root_tree.cxx`
template <typename... I>
class VariableFillers: public std::vector<std::shared_ptr<IVariableFiller<I...> > >
{
public:
  // This should be the only method you need in this class
  template <typename T>
  void add(const std::string& name, const std::function<T(I...)>&);
  template <typename T, typename F>
  void add(const std::string& name, const F func) {
    add(name, std::function<T(I...)>(func));
  }
};

template <typename... I>
template <typename T>
void VariableFillers<I...>::add(const std::string& name,
                          const std::function<T(I...)>& fun) {
  this->push_back(std::make_shared<VariableFiller<T, I...> >(name, fun));
}



// ___________________________________________________________________
// writer class
//
// This is the other thing you interact with.
//
// You'll have to specify the H5::Group to write the dataset to, the
// name of the new dataset, and the extent of the dataset.
//
// To fill, use the `fill_while_incrementing` function, which will
// iterate over all possible values of `indices` and call the filler
// functions.

namespace H5Utils {

  // packing utility
  H5::CompType packed(H5::CompType in);

  template<typename... I>
  H5::CompType build_type(const VariableFillers<I...>& fillers) {
    H5::CompType type(fillers.size() * sizeof(data_buffer_t));
    size_t dt_offset = 0;
    for (const auto& filler: fillers) {
      type.insertMember(filler->name(), dt_offset, filler->get_type());
      dt_offset += sizeof(data_buffer_t);
    }
    return type;
  }

  void print_destructor_error(const std::string& msg);

  // new functions
  H5::DataSpace getUnlimitedSpace(const std::vector<hsize_t>& max_length);
  H5::DSetCreatPropList getChunckedDatasetParams(
    const std::vector<hsize_t>& max_length,
    hsize_t batch_size);
  std::vector<hsize_t> getStriding(std::vector<hsize_t> max_length);
  void throwIfExists(const std::string& name, const H5::Group& in_group);
}

struct DSParameters {
  DSParameters(const H5::CompType type,
               std::vector<hsize_t> max_length,
               hsize_t batch_size);
  hsize_t buffer_size(const std::vector<data_buffer_t>&) const;
  H5::CompType type;
  std::vector<hsize_t> max_length;
  std::vector<hsize_t> dim_stride;
  hsize_t batch_size;
};

template <typename... I>
class WriterXd {
public:
  WriterXd(H5::Group& group, const std::string& name,
           VariableFillers<I...> fillers,
           std::vector<hsize_t> dataset_dimensions,
           hsize_t chunk_size = 2048);
  WriterXd(const WriterXd&) = delete;
  WriterXd& operator=(WriterXd&) = delete;
  ~WriterXd();
  void fill_while_incrementing(std::vector<size_t>& indices = WriterXd::NONE,
                               I... args);
  void flush();
private:
  static std::vector<size_t> NONE;
  const DSParameters _pars;
  hsize_t _offset;
  std::vector<data_buffer_t> _buffer;
  VariableFillers<I...> _fillers;
  H5::DataSet _ds;
  H5::DataSpace _file_space;
};


template <typename... I>
std::vector<size_t> WriterXd<I...>::NONE = {};

template <typename... I>
WriterXd<I...>::WriterXd(H5::Group& group, const std::string& name,
                         VariableFillers<I...> fillers,
                   std::vector<hsize_t> max_length,
                   hsize_t batch_size):
  _pars(H5Utils::build_type(fillers), max_length, batch_size),
  _offset(0),
  _fillers(fillers),
  _file_space(H5S_SIMPLE)
{
  using namespace H5Utils;
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
  _file_space = _ds.getSpace();
  _file_space.selectNone();
}

template <typename... I>
WriterXd<I...>::~WriterXd() {
  using namespace H5Utils;
  try {
    flush();
  } catch (H5::Exception& err) {
    print_destructor_error(err.getDetailMsg());
  } catch (std::exception& err) {
    print_destructor_error(err.what());
  }
}

template <typename... I>
void WriterXd<I...>::fill_while_incrementing(std::vector<size_t>& indices,
                                             I... args) {
  if (_pars.buffer_size(_buffer) == _pars.batch_size) {
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
      temp.push_back(filler->get_buffer(args...));
    }
  }

  // add these elements to the file space
  std::vector<hsize_t> slab_dims{1};
  slab_dims.insert(slab_dims.end(),
                   _pars.max_length.begin(),
                   _pars.max_length.end());
  std::vector<hsize_t> offset_dims{_offset + _pars.buffer_size(_buffer)};
  offset_dims.resize(slab_dims.size(), 0);
  _file_space.selectHyperslab(H5S_SELECT_OR,
                              slab_dims.data(), offset_dims.data());

  _buffer.insert(_buffer.end(), temp.begin(), temp.end());
}

template <typename... I>
void WriterXd<I...>::flush() {
  const hsize_t buffer_size = _pars.buffer_size(_buffer);
  if (buffer_size == 0) return;

  // extend the ds
  std::vector<hsize_t> slab_dims{buffer_size};
  slab_dims.insert(slab_dims.end(),
                   _pars.max_length.begin(),
                   _pars.max_length.end());
  std::vector<hsize_t> total_dims{buffer_size + _offset};
  total_dims.insert(total_dims.end(),
                    _pars.max_length.begin(),
                    _pars.max_length.end());
  _ds.extend(total_dims.data());
  _file_space.setExtentSimple(total_dims.size(), total_dims.data());

  // write out
  assert(static_cast<size_t>(_file_space.getSelectNpoints())
         == _buffer.size() / _fillers.size());
  H5::DataSpace mem_space(slab_dims.size(), slab_dims.data());
  _ds.write(_buffer.data(), _pars.type, mem_space, _file_space);
  _offset += buffer_size;
  _buffer.clear();
  _file_space.selectNone();
}

#endif
