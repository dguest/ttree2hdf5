#include "HdfTuple.hh"
#include <array>
#include <cmath>

int main(int argc, char* argv[]) {

  // make the output file
  H5::H5File file("test-new.h5", H5F_ACC_TRUNC);
  // build the variables
  VariableFillers<int> argvars;
  argvars.add<float>("argthing", [](int x) {return x;}, NAN);
  WriterXd<2,int> writer(file, "thing3", argvars, {{4, 4}});

  // fill file
  for (int xxx = 0; xxx < 10000; xxx++) {
    writer.fill<std::vector<std::vector<int> > >(
      {{-1 * xxx, -2 * xxx, -3 * xxx}, {1, xxx / 2, 3*xxx}, {xxx}});
  }
  return 0;
}
