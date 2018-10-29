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


template <typename T>
struct H5Traits {
  static T& ref(data_buffer_t& buf) = 0;
};
template <> struct H5Traits<int> {
  static const H5::DataType type;
  static int& ref(data_buffer_t& buf) { return buf._int; }
};
template <> struct H5Traits<long long> {
  static const H5::DataType type;
  static long long& ref(data_buffer_t& buf) { return buf._llong; }
};
template <> struct H5Traits<unsigned int> {
  static const H5::DataType type;
  static unsigned int& ref(data_buffer_t& buf) { return buf._uint; }
};
template <> struct H5Traits<unsigned char> {
  static const H5::DataType type;
  static unsigned char& ref(data_buffer_t& buf) { return buf._uchar; }
};
template <> struct H5Traits<float> {
  static const H5::DataType type;
  static float& ref(data_buffer_t& buf) { return buf._float; }
};
template <> struct H5Traits<double> {
  static const H5::DataType type;
  static double& ref(data_buffer_t& buf) { return buf._double; }
};
template <> struct H5Traits<bool> {
  static const H5::DataType type;
  static bool& ref(data_buffer_t& buf) { return buf._bool; }
};

// This is used to buld the unions we use to build the hdf5 memory
// buffer. Together with the `get_type` templates it gives us a
// strongly typed HDF5 writer.

// variable filler class header
template <typename I>
class IVariableFiller
{
public:
  virtual ~IVariableFiller() {}
  virtual data_buffer_t get_buffer(I) const = 0;
  virtual data_buffer_t get_default() const = 0;
  virtual H5::DataType get_type() const = 0;
  virtual std::string name() const = 0;
};

// implementation for variable filler
template <typename T, typename I>
class VariableFiller: public IVariableFiller<I>
{
public:
  VariableFiller(const std::string&,
                 const std::function<T(I)>&,
                 const T default_value = T());
  data_buffer_t get_buffer(I) const;
  data_buffer_t get_default() const;
  H5::DataType get_type() const;
  std::string name() const;
private:
  std::function<T(I)> _getter;
  std::string _name;
  T _default_value;
};
template <typename T, typename I>
VariableFiller<T, I>::VariableFiller(const std::string& name,
                                     const std::function<T(I)>& func,
                                     const T default_value):
  _getter(func),
  _name(name),
  _default_value(default_value)
{
}
template <typename T, typename I>
data_buffer_t VariableFiller<T, I>::get_buffer(I args) const {
  data_buffer_t buffer;
  H5Traits<T>::ref(buffer) = _getter(args);
  return buffer;
}
template <typename T, typename I>
data_buffer_t VariableFiller<T, I>::get_default() const {
  data_buffer_t default_value;
  H5Traits<T>::ref(default_value) = _default_value;
  return default_value;
}
template <typename T, typename I>
H5::DataType VariableFiller<T, I>::get_type() const {
  return H5Traits<T>::type;
}
template <typename T, typename I>
std::string VariableFiller<T, I>::name() const {
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
template <typename I>
class VariableFillers: public std::vector<std::shared_ptr<IVariableFiller<I> > >
{
public:
  // This should be the only method you need in this class
  template <typename T>
  void add(const std::string& name, const std::function<T(I)>&,
           const T& default_value = T());
  template <typename T, typename F>
  void add(const std::string& name, const F func, const T& def = T()) {
    add(name, std::function<T(I)>(func), def);
  }
};

template <typename I>
template <typename T>
void VariableFillers<I>::add(const std::string& name,
                                const std::function<T(I)>& fun,
                                const T& default_value)
{
  this->push_back(
    std::make_shared<VariableFiller<T, I> >(name, fun, default_value));
}



// ___________________________________________________________________
// writer class
//
// This is the other thing you interact with.
//
// You'll have to specify the H5::Group to write the dataset to, the
// name of the new dataset, and the extent of the dataset.
//
// To fill, use the `fill` function.

namespace H5Utils {

  // packing utility
  H5::CompType packed(H5::CompType in);

  template<typename I>
  H5::CompType build_type(const VariableFillers<I>& fillers) {
    H5::CompType type(fillers.size() * sizeof(data_buffer_t));
    size_t dt_offset = 0;
    for (const auto& filler: fillers) {
      type.insertMember(filler->name(), dt_offset, filler->get_type());
      dt_offset += sizeof(data_buffer_t);
    }
    return type;
  }

  template<typename I>
  std::vector<data_buffer_t> build_default(const VariableFillers<I>& f) {
    std::vector<data_buffer_t> def;
    for (const auto& filler: f) {
      def.push_back(filler->get_default());
    }
    return def;
  }

  template <size_t N, typename F, typename T>
  struct DataFlattener {
    std::vector<data_buffer_t> buffer;
    std::vector<std::array<hsize_t, N> > element_offsets;
    DataFlattener(const F& filler, T args):
      buffer() {
      hsize_t offset = 0;
      for (const auto& arg: args) {
        DataFlattener<N-1, F, decltype(arg)> in(filler, arg);
        buffer.insert(buffer.end(), in.buffer.begin(), in.buffer.end());
        for (const auto& in_ele: in.element_offsets){
          std::array<hsize_t, N> element_pos;
          element_pos.at(0) = offset;
          std::copy(in_ele.begin(), in_ele.end(), element_pos.begin() + 1);
          element_offsets.push_back(element_pos);
        }
        offset++;
      }
    }
  };
  template <typename F, typename T>
  struct DataFlattener<0, F, T> {
    std::vector<data_buffer_t> buffer;
    std::vector<std::array<hsize_t, 0> > element_offsets;
    DataFlattener(const F& f, T args):
      buffer(),
      element_offsets(1) {
      for (const auto& filler: f) {
        buffer.push_back(filler->get_buffer(args));
      }
    }
  };

  template <size_t N>
  std::vector<hsize_t> vec(std::array<size_t,N> a) {
    return std::vector<hsize_t>(a.begin(),a.end());
  }

  void print_destructor_error(const std::string& msg);

  // new functions
  H5::DataSpace getUnlimitedSpace(const std::vector<hsize_t>& max_length);
  H5::DSetCreatPropList getChunckedDatasetParams(
    const std::vector<hsize_t>& max_length,
    hsize_t batch_size,
    const H5::CompType&,
    const std::vector<data_buffer_t>& default_value);
  std::vector<hsize_t> getStriding(std::vector<hsize_t> max_length);
  void throwIfExists(const std::string& name, const H5::Group& in_group);
}

struct DSParameters {
  DSParameters(const H5::CompType type,
               std::vector<hsize_t> max_length,
               hsize_t batch_size);
  H5::CompType type;
  std::vector<hsize_t> max_length;
  std::vector<hsize_t> dim_stride;
  hsize_t batch_size;
};

// helper for default constructor argument
template <size_t N>
std::array<size_t, N> uniform(size_t val) {
  std::array<size_t, N> ar;
  ar.fill(val);
  return ar;
}

template <size_t N, typename I>
class WriterXd {
public:
  WriterXd(H5::Group& group, const std::string& name,
           const VariableFillers<I>& fillers,
           const std::array<size_t, N>& dimensions = uniform<N>(5),
           hsize_t chunk_size = 2048);
  WriterXd(const WriterXd&) = delete;
  WriterXd& operator=(WriterXd&) = delete;
  ~WriterXd();
  template <typename T>
  void fill(T);
  void flush();
private:
  static std::array<size_t,N> NONE;
  const DSParameters _pars;
  hsize_t _offset;
  hsize_t _buffer_rows;
  std::vector<data_buffer_t> _buffer;
  VariableFillers<I> _fillers;
  H5::DataSet _ds;
  H5::DataSpace _file_space;
};


template <size_t N, typename I>
std::array<size_t, N> WriterXd<N, I>::NONE;

template <size_t N, typename I>
WriterXd<N, I>::WriterXd(H5::Group& group, const std::string& name,
                            const VariableFillers<I>& fillers,
                            const std::array<size_t,N>& max_length,
                            hsize_t batch_size):
  _pars(H5Utils::build_type(fillers), H5Utils::vec(max_length), batch_size),
  _offset(0),
  _buffer_rows(0),
  _fillers(fillers),
  _file_space(H5S_SIMPLE)
{
  using namespace H5Utils;
  if (batch_size < 1) {
    throw std::logic_error("batch size must be > 0");
  }
  // create space
  H5::DataSpace space = getUnlimitedSpace(vec(max_length));

  // create params
  H5::DSetCreatPropList params = getChunckedDatasetParams(
    vec(max_length), batch_size, _pars.type, H5Utils::build_default(fillers));

  // create ds
  throwIfExists(name, group);
  _ds = group.createDataSet(name, packed(_pars.type), space, params);
  _file_space = _ds.getSpace();
  _file_space.selectNone();
}

template <size_t N, typename I>
WriterXd<N, I>::~WriterXd() {
  using namespace H5Utils;
  try {
    flush();
  } catch (H5::Exception& err) {
    print_destructor_error(err.getDetailMsg());
  } catch (std::exception& err) {
    print_destructor_error(err.what());
  }
}

template <size_t N, typename I>
template <typename T>
void WriterXd<N, I>::fill(T arg) {
  if (_buffer_rows == _pars.batch_size) {
    flush();
  }
  H5Utils::DataFlattener<N, decltype(_fillers), T> buf(_fillers, arg);
  hsize_t n_elements = buf.element_offsets.size();
  std::vector<hsize_t> elements;
  for (const auto& el_local: buf.element_offsets) {
    std::array<hsize_t, N+1> el_global;
    el_global.at(0) = _offset + _buffer_rows;
    std::copy(el_local.begin(), el_local.end(), el_global.begin() + 1);
    elements.insert(elements.end(), el_global.begin(), el_global.end());
  }
  _file_space.selectElements(H5S_SELECT_APPEND, n_elements, elements.data());
  _buffer.insert(_buffer.end(), buf.buffer.begin(), buf.buffer.end());
  _buffer_rows++;
}

template <size_t N, typename I>
void WriterXd<N, I>::flush() {
  const hsize_t buffer_size = _buffer_rows;
  if (buffer_size == 0) return;

  // extend the ds
  std::vector<hsize_t> total_dims{buffer_size + _offset};
  total_dims.insert(total_dims.end(),
                    _pars.max_length.begin(),
                    _pars.max_length.end());
  _ds.extend(total_dims.data());
  _file_space.setExtentSimple(total_dims.size(), total_dims.data());

  // write out
  hsize_t n_buffer_pts = _buffer.size() / _fillers.size();
  assert(_file_space.getSelectNpoints() == n_buffer_pts);
  H5::DataSpace mem_space(1, &n_buffer_pts);
  _ds.write(_buffer.data(), _pars.type, mem_space, _file_space);
  _offset += buffer_size;
  _buffer.clear();
  _buffer_rows = 0;
  _file_space.selectNone();
}

#endif
