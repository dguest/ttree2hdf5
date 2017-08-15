#include "HdfTuple.hh"

int main(int argc, char* argv[]) {
  VariableFillers vars;
  size_t xxx = 0;
  vars.push_back(new VariableFiller<int>("bob", [&xxx](){return xxx;}));
  vars.push_back(new VariableFiller<float>("alice", [&xxx](){return xxx*0.5;}));
  // vars.emplace_back("bob", [&xxx](){return xxx;});
  // vars.emplace_back("alice", [&xxx](){return xxx*0.5;});
  H5::H5File file("test.h5", H5F_ACC_TRUNC);
  Writer writer(file, "thing", vars, 256);
  for (; xxx < 1000; xxx++) {
    writer.fill();
  }
  writer.flush();
  writer.close();
  return 0;
}
