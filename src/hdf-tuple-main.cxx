#include "HdfTuple.hh"

int main(int argc, char* argv[]) {

  // build the variables
  VariableFillers vars;
  size_t xxx = 0;
  vars.add<int>("bob", [&xxx](){return xxx;});
  vars.add<float>("alice", [&xxx](){return xxx*0.5;});
  vars.add<double>("alice2", [&xxx](){return xxx*0.5;});
  vars.add<bool>("bool", [&xxx](){return xxx % 2;});

  // make the output file
  H5::H5File file("test.h5", H5F_ACC_TRUNC);

  // create the writer
  Writer writer(file, "thing", vars, 256);

  // fill file
  for (; xxx < 1000; xxx++) {
    writer.fill();
  }
  writer.flush();
  writer.close();
  return 0;
}
