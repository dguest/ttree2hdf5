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
data_buffer_t get_buffer_from_func(const std::function<T()>& func) {
  data_buffer_t buffer;
  get_ref<T>(buffer) = func();
  return buffer;
}

// variable filler class header
class IVariableFiller
{
public:
  virtual ~IVariableFiller() {}
  virtual data_buffer_t get_buffer() const = 0;
  virtual data_buffer_t get_empty() const = 0;
  virtual H5::DataType get_type() const = 0;
  virtual std::string name() const = 0;
};

// implementation for variable filler
template <typename T>
class VariableFiller: public IVariableFiller
{
public:
  VariableFiller(const std::string&, const std::function<T()>&);
  data_buffer_t get_buffer() const;
  data_buffer_t get_empty() const;
  H5::DataType get_type() const;
  std::string name() const;
private:
  std::function<T()> _getter;
  std::string _name;
};
template <typename T>
VariableFiller<T>::VariableFiller(const std::string& name,
                                  const std::function<T()>& func):
  _getter(func),
  _name(name)
{
}
template <typename T>
data_buffer_t VariableFiller<T>::get_buffer() const {
  return ::get_buffer_from_func<T>(_getter);
}
template <typename T>
data_buffer_t VariableFiller<T>::get_empty() const {
  data_buffer_t buffer;
  get_ref<T>(buffer) = 0;
  return buffer;
}
template <typename T>
H5::DataType VariableFiller<T>::get_type() const {
  return ::get_type<T>();
}
template <typename T>
std::string VariableFiller<T>::name() const {
  return _name;
}



// _________________________________________________________________________
// the class that holds the variable fillers
//
// (this is what you actually interact with)

class VariableFillers: public std::vector<std::shared_ptr<IVariableFiller> >
{
public:
  template <typename T>
  void add(const std::string& name, const std::function<T()>&);
};

template <typename T>
void VariableFillers::add(const std::string& name,
                             const std::function<T()>& fun) {
  this->push_back(std::make_shared<VariableFiller<T> >(name, fun));
}



// ___________________________________________________________________
// writer class
//
// (this is another thing you interact with)

class WriterXd {
public:
  WriterXd(H5::CommonFG& group, const std::string& name,
           VariableFillers fillers,
           std::vector<hsize_t> max_lengths,
           hsize_t chunk_size = 2048);
  WriterXd(const WriterXd&) = delete;
  WriterXd& operator=(WriterXd&) = delete;
  void fill_while_incrementing(std::vector<size_t>& index = WriterXd::dummy);
  void flush();
  void close();
private:
  static std::vector<size_t> dummy;
  hsize_t buffer_size() const;
  H5::CompType _type;
  std::vector<hsize_t> _max_length;
  std::vector<hsize_t> _dim_stride;
  hsize_t _batch_size;
  hsize_t _offset;
  std::vector<data_buffer_t> _buffer;
  VariableFillers _fillers;
  H5::DataSet _ds;
};


#endif
