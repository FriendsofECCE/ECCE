#include "util/NotImplementedException.H"

#include "tdat/Fragment.H"

#include "AbstractPropCalculation.H"


vector<string> AbstractPropCalculation::propertyNames(void)
{
  return vector<string>();
}


TProperty* AbstractPropCalculation::getProperty(const string& name)
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return NULL;
}


TProperty* AbstractPropCalculation::updateProperty(const string& name, const string& value)
{
  return NULL;
}


bool AbstractPropCalculation::putProperty(TProperty* prop)
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return false;
}


bool AbstractPropCalculation::deleteProperties(void)
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return false;
}


void AbstractPropCalculation::flushPropertyCache(void)
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
}


bool AbstractPropCalculation::fragment(Fragment *frag)
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return false;
}


Fragment* AbstractPropCalculation::fragment() const
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return new Fragment;
}


bool AbstractPropCalculation::getFragment(Fragment &frag)
{
  return true;
}


Fragment* AbstractPropCalculation::getFragmentStep(int step)
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return NULL;
}


Fragment* AbstractPropCalculation::getFragmentStep(int step, PropertyDoc& doc)
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return NULL;
}


bool AbstractPropCalculation::getFragmentStep(Fragment *frag, int step)
{
  //NOTIMPLEMENTEDEXCEPTION(false, "");
  return false;
}

long AbstractPropCalculation::getFragmentModifiedDate() const
{
  NOTIMPLEMENTEDEXCEPTION(false, "");
  return 0;
}


bool AbstractPropCalculation::isReadOnly() const
{
  return false;
}


bool AbstractPropCalculation::promptBeforeSave(Fragment *frag,
        string& msg, string& title)
{
  return false;
}

