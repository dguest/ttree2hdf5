#include "HdfTuple.hh"

int main(int argc, char* argv[]) {

  // build the variables
  VariableFillers vars;
  size_t xxx = 0;
  std::vector<size_t> iii(1,0);
  vars.add<int>("bob",       [&xxx, &iii](){return xxx + iii.at(0);});
  vars.add<float>("alice",   [&xxx, &iii](){return xxx*0.5 + iii.at(0);});
  vars.add<double>("alice2", [&xxx, &iii](){return xxx*0.5 / (iii.at(0) + 1);});
  vars.add<bool>("bool",     [&xxx, &iii](){return (xxx + iii.at(0)) % 2;});

  // make the output file
  H5::H5File file("test.h5", H5F_ACC_TRUNC);

  // create the writers
  WriterXd writer(file, "thing", vars, {}, 256);
  WriterXd writer2(file, "thing2", vars, {10}, 256);

  // fill file
  for (; xxx < 1000; xxx++) {
    writer.fill_while_incrementing();
    writer2.fill_while_incrementing(iii);
  }
  writer.flush();
  writer2.flush();
  return 0;
}
