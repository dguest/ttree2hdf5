#include "HdfTuple.hh"
#include <array>

int main(int argc, char* argv[]) {

  // build the variables
  VariableFillers<> vars;
  size_t xxx = 0;
  std::array<size_t,1> iii;
  vars.add<int>("bob", [&xxx, &iii](){return xxx + iii.at(0);});
  vars.add<float>("alice", [&xxx, &iii]() {return xxx*0.5 + iii.at(0);});
  vars.add<double>("alice2", [&xxx, &iii](){return xxx*0.5 / (iii.at(0) + 1);});
  vars.add<bool>("bool",     [&xxx, &iii](){return (xxx + iii.at(0)) % 2;});

  // make the output file
  H5::H5File file("test.h5", H5F_ACC_TRUNC);

  // create the writers
  WriterXd<0> writer(file, "thing", vars, std::array<hsize_t,0>{}, 256);
  WriterXd<1> writer2(file, "thing2", vars, {{10}}, 256);

  VariableFillers<int> argvars;
  argvars.add<float>("argthing", [](int x) {return x;});
  WriterXd<1,int> writer3(file, "thing3", argvars, {{10}});

  // fill file
  for (; xxx < 1000; xxx++) {
    writer.fill_while_incrementing();
    writer2.fill_while_incrementing(iii);
    writer3.fill_while_incrementing(iii, xxx);
  }
  writer.flush();
  writer2.flush();
  return 0;
}
