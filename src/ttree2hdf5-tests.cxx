#include "HdfTuple.hh"
#include <array>
#include <cmath>

int main(int argc, char* argv[]) {

  // make the output file
  H5::H5File file("test-new.h5", H5F_ACC_TRUNC);
  // build the variables
  H5Utils::Consumers<int> argvars;
  argvars.add<float>("argthing", [](int x) {return x;}, NAN);
  argvars.add<bool>("bool", [](int x) {return x % 2;}, NAN);
  H5Utils::Writer<0,int> writer(file, "thing3", argvars);

  // fill file
  for (int xxx = 0; xxx < 10000; xxx++) {
    writer.fill<int>(xxx);
  }
  return 0;
}
