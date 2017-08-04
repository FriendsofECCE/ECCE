
#include <wx/link.h>

#include "util/STLUtil.H"

#include "tdat/Fragment.H"

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "TrajectoryCalculation.H"


wxFORCE_LINK_THIS_MODULE(TrajectoryCalculation)


IMPLEMENT_CALCULATION(
        TrajectoryCalculation,
        Trajectory,
        TrajectoryCalculation::canOpen,
        TrajectoryCalculation::open,
        TrajectoryCalculation::inputMasks
)


TrajectoryCalculation::TrajectoryCalculation(const EcceURL& url)
  : AbstractPropCalculation(),
    p_url(url)
{
}


TrajectoryCalculation::~TrajectoryCalculation()
{
}


vector<string> TrajectoryCalculation::propertyNames(void)
{
  vector<string> names;
  names.push_back("TRJ");
  return names;
}


EcceURL TrajectoryCalculation::getURL() const
{
  return p_url;
}


bool TrajectoryCalculation::isReadOnly() const
{
  return true;
}


bool TrajectoryCalculation::canOpen(const EcceURL& url)
{
  bool ret = false;

  string ext = STLUtil::upper(STLUtil::afterLast(url,"."));
  if (ext == "TRJ") {
    ret = true;
  } else if (ext == "XYZ") {
    Fragment frag;
    // get a stream pointer 
    // we need to see if this xyz trajectory has multiple steps
    Resource * resource = EDSIFactory::getResource(url);
    istream * is = resource->getDocument();
    if (frag.restoreXYZ(*is,1.0,false)) {
      string tmp;
      getline(*is,tmp);
      if (is->good() && !is->eof()) {
        ret = true;
      }
    }
    delete is;
  }

  return ret;
}


IPropCalculation *TrajectoryCalculation::open(const EcceURL& url)
{
  return new TrajectoryCalculation(url);
}


string TrajectoryCalculation::inputMasks()
{
  return "*.trj;*.xyz";
}


