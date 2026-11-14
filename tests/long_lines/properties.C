#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/PropTSTable.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropVecTable.H"

class PropFactory {
public:
  template<class T> static T* make() { return new T("test", "test"); }
};

template<class T> bool tables(const std::string& pad)
{
  std::unique_ptr<T> property(PropFactory::make<T>());
  std::istringstream input("Size:2,1," + pad + "2\nrow:col:1.25\nrow:col:-2.5\n"
                          "row:col:3.5\nrow:col:4.75\n");
  property->setData(input);
  return property->value(0, 0, 0) == 1.25 && property->value(0, 0, 1) == -2.5 &&
         property->value(1, 0, 0) == 3.5 && property->value(1, 0, 1) == 4.75;
}

int main()
{
  for (const std::string& pad : {std::string(), std::string(600, ' ')}) {
    std::unique_ptr<PropVector> vector(PropFactory::make<PropVector>());
    std::istringstream vi("Size:" + pad + "2\nfirst:1.25\nlast:-2.5\n");
    vector->setData(vi);
    if (vector->value(0) != 1.25 || vector->value(1) != -2.5) return 1;
    std::unique_ptr<PropTable> table(PropFactory::make<PropTable>());
    std::istringstream ti("Size:1," + pad + "2\nrow:col:1.25\nrow:col:-2.5\n");
    table->values(ti);
    if (table->value(0, 0) != 1.25 || table->value(0, 1) != -2.5) return 1;
    if (!tables<PropTSTable>(pad) || !tables<PropTSVecTable>(pad) ||
        !tables<PropVecTable>(pad)) return 1;
  }
  std::cout << "PASS all five property size readers, short and long lines\n";
}
