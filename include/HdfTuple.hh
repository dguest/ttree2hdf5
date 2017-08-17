#ifndef HDF_TUPLE_HH
#define HDF_TUPLE_HH

#include "H5Cpp.h"

#include <functional>
#include <vector>
#include <memory>

union data_buffer_t
{
  int _int;
  float _float;
  double _double;
  bool _bool;
};

// _____________________________________________________________________
// internal classes
//
// these guys are defined in the cxx file

template <typename T> H5::DataType get_type();
template<> H5::DataType get_type<int>();
template<> H5::DataType get_type<float>();
template<> H5::DataType get_type<double>();
template<> H5::DataType get_type<bool>();

template <typename T>
T& get_ref(data_buffer_t& buf);
template<> int& get_ref<int>(data_buffer_t& buf);
template<> float& get_ref<float>(data_buffer_t& buf);
template<> double& get_ref<double>(data_buffer_t& buf);
template<> bool& get_ref<bool>(data_buffer_t& buf);

// this is used to buld the unions we use to build the hdf5 memory
// buffer
template <typename T>
data_buffer_t get_buffer(const T& arg) {
  data_buffer_t buffer;
  get_ref<T>(buffer) = arg;
  return buffer;
}

// variable filler class header
template <typename M>
class IVariableFiller
{
public:
  virtual ~IVariableFiller() {}
  virtual data_buffer_t get_buffer(const M&) const = 0;
  virtual data_buffer_t get_empty() const = 0;
  virtual H5::DataType get_type() const = 0;
  virtual std::string name() const = 0;
};

// implementation for variable filler
template <typename T, typename M>
class VariableFiller: public IVariableFiller<M>
{
public:
  VariableFiller(const std::string&, const std::function<T(const M&)>&);
  data_buffer_t get_buffer(const M&) const;
  data_buffer_t get_empty() const;
  H5::DataType get_type() const;
  std::string name() const;
private:
  std::function<T()> _getter;
  std::string _name;
};
template <typename T, typename M>
VariableFiller<T,M>::VariableFiller(const std::string& name,
                                  const std::function<T(const M&)>& func):
  _getter(func),
  _name(name)
{
}
template <typename T, typename M>
data_buffer_t VariableFiller<T, M>::get_buffer(const M& arg) const {
  return ::get_buffer<T>(_getter(arg));
}
template <typename T, typename M>
data_buffer_t VariableFiller<T, M>::get_empty() const {
  data_buffer_t buffer;
  get_ref<T>(buffer) = 0;
  return buffer;
}
template <typename T, typename M>
H5::DataType VariableFiller<T, M>::get_type() const {
  return ::get_type<T>();
}
template <typename T, typename M>
std::string VariableFiller<T, M>::name() const {
  return _name;
}



// _________________________________________________________________________
// the class that holds the variable fillers
//
// (this is what you actually interact with)

template <typename M>
class VariableFillers: public std::vector<std::shared_ptr<IVariableFiller<M> > >
{
public:
  template <typename T>
  void add(const std::string& name, const std::function<T(const M&)>&);
};

template <typename M>
template <typename T>
void VariableFillers<M>::add(const std::string& name,
                             const std::function<T(const M&)>& fun) {
  this->push_back(std::make_shared<VariableFiller<T,M> >(name, fun));
}



// ___________________________________________________________________
// writer class
//
// (this is another thing you interact with)

template <typename M>
class Writer
{
public:
  Writer(H5::CommonFG& out_file, const std::string& name,
         VariableFillers<M> fillers, hsize_t batch_size = 1000);
  Writer(const Writer&) = delete;
  Writer& operator=(Writer&) = delete;
  void fill();
  void flush();
  void close();
private:
  hsize_t buffer_size() const;
  H5::CompType _type;
  hsize_t _batch_size;
  hsize_t _offset;
  std::vector<data_buffer_t> _buffer;
  VariableFillers<M> _fillers;
  H5::DataSet _ds;
};

template <typename M>
class Writer2d {
public:
  Writer2d(H5::CommonFG& group, const std::string& name,
           VariableFillers<M> fillers,
           hsize_t max_length, hsize_t batch_size = 1000);
  Writer2d(const Writer2d&) = delete;
  Writer2d& operator=(Writer2d&) = delete;
  void fill_while_incrementing(size_t& index, const size_t& size);
  void flush();
  void close();
private:
  hsize_t buffer_size() const;
  H5::CompType _type;
  hsize_t _max_length;
  hsize_t _batch_size;
  hsize_t _offset;
  std::vector<data_buffer_t> _buffer;
  VariableFillers<M> _fillers;
  H5::DataSet _ds;
};


#endif
