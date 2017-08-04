#include <sstream>
  using std::ostringstream;

#include "util/NotImplementedException.H"

#include "tdat/Fragment.H"

#include "DefaultCalculation.H"


IMPLEMENT_CALCULATION(
        DefaultCalculation,
        New Calculation,
        DefaultCalculation::canOpen,
        DefaultCalculation::open,
        DefaultCalculation::inputMasks)


const string DefaultCalculation::DEFAULT_NAME("Unnamed");
int DefaultCalculation::p_count(0);


DefaultCalculation::DefaultCalculation()
  : p_myIndex(++p_count)
{
}


EcceURL DefaultCalculation::getURL() const
{
  ostringstream os;
  os << DEFAULT_NAME << p_myIndex;
  return os.str(); 
}


bool DefaultCalculation::isReadOnly() const
{
  return false;
}


string DefaultCalculation::nextURL()
{
  ostringstream os;
  os << DEFAULT_NAME << (p_count+1);
  return os.str();
}


// IPropCalculation framework
bool DefaultCalculation::canOpen(const EcceURL& url)
{
  bool ret = url == nextURL();
  return ret;
}


IPropCalculation *DefaultCalculation::open(const EcceURL& url)
{
  IPropCalculation * ret = new DefaultCalculation();
  return ret;
}


string DefaultCalculation::inputMasks()
{
  return "";
}

