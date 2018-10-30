#include "H5Traits.h"

#include "H5Cpp.h"

namespace H5Utils {
  namespace internal {

    // bool is a special case, we need to define it with a more
    // complicated function
    H5::DataType get_bool_type();

    typedef H5::PredType PT;
    const H5::DataType H5Traits<int>::type = PT::NATIVE_INT;
    const H5::DataType H5Traits<long long>::type = PT::NATIVE_LLONG;
    const H5::DataType H5Traits<unsigned int>::type = PT::NATIVE_UINT;
    const H5::DataType H5Traits<unsigned char>::type = PT::NATIVE_UCHAR;
    const H5::DataType H5Traits<float>::type = PT::NATIVE_FLOAT;
    const H5::DataType H5Traits<double>::type = PT::NATIVE_DOUBLE;
    const H5::DataType H5Traits<bool>::type = get_bool_type();

    // define the spaciel bool case
    H5::DataType get_bool_type() {
      bool TRUE = true;
      bool FALSE = false;
      H5::EnumType btype(sizeof(bool));
      btype.insert("TRUE", &TRUE);
      btype.insert("FALSE", &FALSE);
      return btype;
    }

  }
}
