#ifndef HDF_TUPLE_HH
#define HDF_TUPLE_HH

// HDF5 Writer
//
// Skip down to the `Writer` and `Consumers` classes below to
// see the stuff that you'll have to interact with.

#include "H5Traits.h"
#include "common.h"

#include "H5Cpp.h"

#include <functional>
#include <vector>
#include <memory>
#include <cassert>

namespace H5Utils {

  // _________________________________________________________________________
  // Internal structures for the Consumers class
  //
  namespace internal {

    // variable filler class header
    template <typename I>
    class IDataConsumer
    {
    public:
      virtual ~IDataConsumer() {}
      virtual data_buffer_t get_buffer(I) const = 0;
      virtual data_buffer_t get_default() const = 0;
      virtual H5::DataType get_type() const = 0;
      virtual std::string name() const = 0;
    };

    // implementation for variable filler
    template <typename T, typename I>
    class DataConsumer: public IDataConsumer<I>
    {
    public:
      DataConsumer(const std::string&,
                   const std::function<T(I)>&,
                   const T default_value = T());
      data_buffer_t get_buffer(I) const;
      data_buffer_t get_default() const;
      H5::DataType get_type() const;
      std::string name() const;
    private:
      std::function<T(I)> m_getter;
      std::string m_name;
      T m_default_value;
    };
    template <typename T, typename I>
    DataConsumer<T, I>::DataConsumer(const std::string& name,
                                     const std::function<T(I)>& func,
                                     const T default_value):
      m_getter(func),
      m_name(name),
      m_default_value(default_value)
    {
    }
    template <typename T, typename I>
    data_buffer_t DataConsumer<T, I>::get_buffer(I args) const {
      data_buffer_t buffer;
      H5Traits<T>::ref(buffer) = m_getter(args);
      return buffer;
    }
    template <typename T, typename I>
    data_buffer_t DataConsumer<T, I>::get_default() const {
      data_buffer_t default_value;
      H5Traits<T>::ref(default_value) = m_default_value;
      return default_value;
    }
    template <typename T, typename I>
    H5::DataType DataConsumer<T, I>::get_type() const {
      return H5Traits<T>::type;
    }
    template <typename T, typename I>
    std::string DataConsumer<T, I>::name() const {
      return m_name;
    }
  }


  // _________________________________________________________________________
  // Consumers class
  //
  template <typename I>
  using SharedConsumer = std::shared_ptr<internal::IDataConsumer<I> >;
  template <typename I>
  class Consumers: public std::vector<SharedConsumer<I> >
  {
  public:
    // This should be the only method you need in this class
    template <typename T>
    void add(const std::string& name, const std::function<T(I)>&,
             const T& default_value = T());
    // overload to cast lambdas into functions
    template <typename T, typename F>
    void add(const std::string& name, const F func, const T& def = T()) {
      add(name, std::function<T(I)>(func), def);
    }
  };

  // implementation details for Consumers
  template <typename I>
  template <typename T>
  void Consumers<I>::add(const std::string& name,
                         const std::function<T(I)>& fun,
                         const T& def_val)
  {
    this->push_back(
      std::make_shared<internal::DataConsumer<T, I> >(name, fun, def_val));
  }



  // _________________________________________________________________________
  // Internal structures for the Writer class
  //
  namespace internal {

    // Data flattener class: this is used by the writer to read in the
    // elements one by one and put them in an internal buffer.
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


    // Adapters to translate configuration info into the objects
    // needed by the writer.
    //
    template<typename I>
    H5::CompType build_type(const Consumers<I>& fillers) {
      H5::CompType type(fillers.size() * sizeof(data_buffer_t));
      size_t dt_offset = 0;
      for (const auto& filler: fillers) {
        type.insertMember(filler->name(), dt_offset, filler->get_type());
        dt_offset += sizeof(data_buffer_t);
      }
      return type;
    }
    template<typename I>
    std::vector<data_buffer_t> build_default(const Consumers<I>& f) {
      std::vector<data_buffer_t> def;
      for (const auto& filler: f) {
        def.push_back(filler->get_default());
      }
      return def;
    }

    // some internal functions take a vector, others take arrays
    template <size_t N>
    std::vector<hsize_t> vec(std::array<size_t,N> a) {
      return std::vector<hsize_t>(a.begin(),a.end());
    }

    // default initalizer for writers where the extent isn't specified
    template <size_t N>
    std::array<size_t, N> uniform(size_t val) {
      std::array<size_t, N> ar;
      ar.fill(val);
      return ar;
    }


  }

  // ___________________________________________________________________
  // Writer Class
  //
  // This is the other thing you interact with.
  //
  // You'll have to specify the H5::Group to write the dataset to, the
  // name of the new dataset, and the extent of the dataset.
  //
  // To fill, use the `fill` function.

  template <size_t N, typename I>
  class Writer {
  public:
    Writer(H5::Group& group, const std::string& name,
           const Consumers<I>& consumers,
           const std::array<size_t, N>& extent = internal::uniform<N>(5),
           hsize_t chunk_size = 2048);
    Writer(const Writer&) = delete;
    Writer& operator=(Writer&) = delete;
    ~Writer();
    template <typename T>
    void fill(T);
    void flush();
  private:
    const internal::DSParameters m_par;
    hsize_t m_offset;
    hsize_t m_buffer_rows;
    std::vector<internal::data_buffer_t> m_buffer;
    Consumers<I> m_consumers;
    H5::DataSet m_ds;
    H5::DataSpace m_file_space;
  };

  template <size_t N, typename I>
  Writer<N, I>::Writer(H5::Group& group, const std::string& name,
                       const Consumers<I>& con,
                       const std::array<size_t,N>& extent,
                       hsize_t batch_size):
    m_par(internal::build_type(con), internal::vec(extent), batch_size),
    m_offset(0),
    m_buffer_rows(0),
    m_consumers(con),
    m_file_space(H5S_SIMPLE)
  {
    using namespace internal;
    if (batch_size < 1) {
      throw std::logic_error("batch size must be > 0");
    }
    // create space
    H5::DataSpace space = getUnlimitedSpace(vec(extent));

    // create params
    H5::DSetCreatPropList params = getChunckedDatasetParams(
      vec(extent), batch_size, m_par.type, build_default(con));

    // create ds
    throwIfExists(name, group);
    m_ds = group.createDataSet(name, packed(m_par.type), space, params);
    m_file_space = m_ds.getSpace();
    m_file_space.selectNone();
  }

  template <size_t N, typename I>
  Writer<N, I>::~Writer() {
    using namespace H5Utils;
    try {
      flush();
    } catch (H5::Exception& err) {
      internal::print_destructor_error(err.getDetailMsg());
    } catch (std::exception& err) {
      internal::print_destructor_error(err.what());
    }
  }

  template <size_t N, typename I>
  template <typename T>
  void Writer<N, I>::fill(T arg) {
    if (m_buffer_rows == m_par.batch_size) {
      flush();
    }
    internal::DataFlattener<N, decltype(m_consumers), T> buf(m_consumers, arg);
    hsize_t n_el = buf.element_offsets.size();
    std::vector<hsize_t> elements;
    for (const auto& el_local: buf.element_offsets) {
      std::array<hsize_t, N+1> el_global;
      el_global[0] = m_offset + m_buffer_rows;
      std::copy(el_local.begin(), el_local.end(), el_global.begin() + 1);
      elements.insert(elements.end(), el_global.begin(), el_global.end());
    }
    m_file_space.selectElements(H5S_SELECT_APPEND, n_el, elements.data());
    m_buffer.insert(m_buffer.end(), buf.buffer.begin(), buf.buffer.end());
    m_buffer_rows++;
  }

  template <size_t N, typename I>
  void Writer<N, I>::flush() {
    const hsize_t buffer_size = m_buffer_rows;
    if (buffer_size == 0) return;

    // extend the ds
    std::vector<hsize_t> total_dims{buffer_size + m_offset};
    total_dims.insert(total_dims.end(),
                      m_par.extent.begin(),
                      m_par.extent.end());
    m_ds.extend(total_dims.data());
    m_file_space.setExtentSimple(total_dims.size(), total_dims.data());

    // write out
    hsize_t n_buffer_pts = m_buffer.size() / m_consumers.size();
    assert(m_file_space.getSelectNpoints() == n_buffer_pts);
    H5::DataSpace mem_space(1, &n_buffer_pts);
    m_ds.write(m_buffer.data(), m_par.type, mem_space, m_file_space);
    m_offset += buffer_size;
    m_buffer.clear();
    m_buffer_rows = 0;
    m_file_space.selectNone();
  }

}

#endif
