#include "copy_root_tree.hh"
#include "HdfTuple.hh"

#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"

#include <iostream>

class DataBuffers: public std::vector<data_buffer_t*> {
public:
  ~DataBuffers() {
    for (auto buf : *this) {
      delete buf;
    }
  }
};

void copy_root_tree(TTree& tt, H5::CommonFG& fg) {
  const std::string tree_name = tt.GetName();

  // note: this leaks memory!
  DataBuffers buffer;
  std::set<std::string> skipped;

  TIter next(tt.GetListOfLeaves());
  VariableFillers vars;
  TLeaf* leaf;
  while ((leaf = dynamic_cast<TLeaf*>(next()))) {
    std::string leaf_name = leaf->GetName();
    std::string leaf_type = leaf->GetTypeName();
    if (leaf_type == "Int_t") {
      buffer.push_back(new data_buffer_t);
      int& buf = buffer.back()->_int;
      tt.SetBranchAddress(leaf_name.c_str(), &buf);
      vars.add<int>(leaf_name, [&buf](){return buf;});
    } else if (leaf_type == "Float_t") {
      buffer.push_back(new data_buffer_t);
      float& buf = buffer.back()->_float;
      tt.SetBranchAddress(leaf_name.c_str(), &buf);
      vars.add<float>(leaf_name, [&buf](){return buf;});
    } else if (leaf_type == "Double_t") {
      buffer.push_back(new data_buffer_t);
      double& buf = buffer.back()->_double;
      tt.SetBranchAddress(leaf_name.c_str(), &buf);
      vars.add<double>(leaf_name, [&buf](){return buf;});
    } else {
      skipped.insert(leaf_type);
    }
  }

  Writer writer(fg, tree_name, vars);
  size_t n_entries = tt.GetEntries();
  for (size_t iii = 0; iii < n_entries; iii++) {
    tt.GetEntry(iii);
    writer.fill();
  }
  writer.flush();
  writer.close();

  for (const auto& name: skipped) {
    std::cout << "skipped " << name << std::endl;
  }
}

// int main(int argc, char* argv[]) {

//   // build the variables
//   VariableFillers vars;
//   size_t xxx = 0;
//   vars.add<int>("bob", [&xxx](){return xxx;});
//   vars.add<float>("alice", [&xxx](){return xxx*0.5;});
//   vars.add<double>("alice2", [&xxx](){return xxx*0.5;});
//   vars.add<bool>("bool", [&xxx](){return xxx % 2;});

//   // make the output file
//   H5::H5File file("test.h5", H5F_ACC_TRUNC);

//   // create the writer
//   Writer writer(file, "thing", vars, 256);

//   // fill file
//   for (; xxx < 1000; xxx++) {
//     writer.fill();
//   }
//   writer.flush();
//   writer.close();
//   return 0;
// }
