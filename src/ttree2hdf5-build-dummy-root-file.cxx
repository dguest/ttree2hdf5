#include "unshittify.hh"
#include "TFile.h"
#include "TTree.h"

#include <type_traits>

namespace {

  template <typename T>
  T subvec(const std::vector<T>& vec) {
    return T();
  }

  class TreeStuff {
  public:
    TreeStuff(TTree&);
    ~TreeStuff();
    TreeStuff(TreeStuff&) = delete;
    TreeStuff& operator=(TreeStuff&) = delete;
    void fill(int number);
  private:
    int _int;
    float _float;
    std::vector<int>* _intv;
    std::vector<float>* _floatv;
    std::vector<std::vector<int> >* _intvv;
    std::vector<std::vector<float> >* _floatvv;
  };
  TreeStuff::TreeStuff(TTree& tree):
    _intv(new std::vector<int>),
    _floatv(new std::vector<float>),
    _intvv(new std::vector<std::vector<int> >),
    _floatvv(new std::vector<std::vector<float> >)
  {
    tree.Branch("int", &_int);
    tree.Branch("float", &_float);
    tree.Branch("intv", &_intv);
    tree.Branch("floatv", &_floatv);
    tree.Branch("intvv", &_intvv);
    tree.Branch("floatvv", &_floatvv);
  }
  TreeStuff::~TreeStuff() {
    delete _intv;
    delete _floatv;
    delete _intvv;
    delete _floatvv;
  }
  void TreeStuff::fill(int number) {
    _intv->clear();
    _floatv->clear();
    _intvv->clear();
    _floatvv->clear();

    _int = number;
    _float = number;
    for (int idx = 0; idx < number; idx++) {
      _intv->push_back(idx);
      _floatv->push_back(idx);
      _intvv->push_back(subvec(*_intvv));
      _floatvv->push_back(subvec(*_floatvv));
      for (int idx2 = 0; idx2 < idx; idx2++) {
        _intvv->back().push_back(idx2);
        _floatvv->back().push_back(idx2);
      }
    }
  }
}

int main(int argc, char* argv[]) {
  unshittify();
  std::string out_file = "test.root";
  if (argc > 1) out_file = argv[1];
  TFile out(out_file.c_str(), "recreate");
  TTree otree("bob", "bob");
  TreeStuff tstuff(otree);
  size_t max = 10;
  if (argc > 2) max = atoi(argv[2]);
  for (int idx = 0; idx < max; idx++) {
    tstuff.fill(idx);
    otree.Fill();
  }
  otree.Write();
}
