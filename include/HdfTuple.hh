#ifndef HDF_TUPLE_HH
#define HDF_TUPLE_HH

#include "H5Cpp.h"

#include <functional>
#include <vector>

union data_buffer_t
{
  int _int;
  float _float;
  double _double;
  bool _bool;
};

// _____________________________________________________________________
// internal classes

// these guys are defined in the cxx file

template <typename T> data_buffer_t get_buffer(const std::function<T()>&);
template<> data_buffer_t get_buffer(const std::function<int()>&);
template<> data_buffer_t get_buffer(const std::function<float()>&);
template<> data_buffer_t get_buffer(const std::function<double()>&);
template<> data_buffer_t get_buffer(const std::function<bool()>&);

template <typename T> H5::DataType get_type();
template<> H5::DataType get_type<int>();
template<> H5::DataType get_type<float>();
template<> H5::DataType get_type<double>();
template<> H5::DataType get_type<bool>();



// variable filler class header
class IVariableFiller
{
public:
  virtual ~IVariableFiller() {}
  virtual data_buffer_t get_buffer() const = 0;
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
  return ::get_buffer<T>(_getter);
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

class Writer
{
public:
  Writer(H5::CommonFG& out_file, const std::string& name,
         VariableFillers fillers, hsize_t batch_size = 1000);
  Writer(const Writer&) = delete;
  Writer& operator=(Writer&) = delete;
  ~Writer();
  void fill();
  void flush();
  void close();
private:
  hsize_t buffer_size() const;
  H5::CompType _type;
  hsize_t _batch_size;
  hsize_t _offset;
  std::vector<data_buffer_t> _buffer;
  VariableFillers _fillers;
  H5::DataSet _ds;
};

#endif
