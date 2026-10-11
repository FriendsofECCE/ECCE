//  TFormula's Hill-order display (#23 of the 2026-10-11 round): no dot after a
//  two-letter symbol, Hill order unchanged, older stored "He.2" still read.
#include <cstdio>
#include <string>

#include "tdat/TFormula.H"

static int failures = 0;

static void expect(const std::string& in, const std::string& want)
{
  const std::string got = TFormula::hillOrder(in);
  printf("%s %s -> %s\n", got == want ? "ok  " : "FAIL", in.c_str(),
         got.c_str());
  if (got != want) failures++;
}

int main()
{
  expect("He2", "He2");
  expect("He.2", "He2");          // stored by older versions
  expect("H2O", "H2O");
  expect("OH2", "H2O");
  expect("H8C3O3", "C3H8O3");     // with carbon: C, H, then alphabetical
  expect("Fe.2Cl3+", "Cl3Fe2+");  // without: alphabetical
  expect("Co2", "Co2");
  expect("CO2", "CO2");
  expect("NaCl", "ClNa");
  return failures ? 1 : 0;
}
