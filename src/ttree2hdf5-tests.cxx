#include "HdfTuple.hh"

int main(int argc, char* argv[]) {

  // build the variables
  VariableFillers vars;
  size_t xxx = 0;
  size_t iii = 0;
  vars.add<int>("bob",       [&xxx, &iii](){return xxx + iii;});
  vars.add<float>("alice",   [&xxx, &iii](){return xxx*0.5 + iii;});
  vars.add<double>("alice2", [&xxx, &iii](){return xxx*0.5 / (iii + 1);});
  vars.add<bool>("bool",     [&xxx, &iii](){return (xxx + iii) % 2;});

  // make the output file
  H5::H5File file("test.h5", H5F_ACC_TRUNC);

  // create the writers
  Writer writer(file, "thing", vars, 256);
  WriterXd writer2(file, "thing2", vars, {10}, 256);

  // fill file
  for (; xxx < 1000; xxx++) {
    writer.fill();
    writer2.fill_while_incrementing(iii, 5);
  }
  writer.flush();
  writer.close();
  writer2.flush();
  writer2.close();
  return 0;
}
