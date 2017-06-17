#include "inv/SoOffscreenRenderer.H"

#include "util/SFile.H"

#include "wxviz/RenderFileCmd.H"
#include "wxviz/SGViewer.H"
#include "wxviz/VizRender.H"


RenderFileCmd::RenderFileCmd(const string& name, Receiver *receiver)
       : Command(name, receiver)
{
  init();
}

RenderFileCmd::~RenderFileCmd()
{
}

void RenderFileCmd::init() 
{
   addParameter(new CommandParameter("width",120));
   addParameter(new CommandParameter("height",120));
   addParameter(new CommandParameter("type","RGB"));
   addParameter(new CommandParameter("filename","print.rgb"));
   addParameter(new CommandParameter("red",0.0));
   addParameter(new CommandParameter("green",0.0));
   addParameter(new CommandParameter("blue",0.0));
}

bool RenderFileCmd::execute() throw(EcceException)
{
   // check whether site has disabled creation of image files because of
   // lack of OpenGL offscreen rendering
   if (getenv("ECCE_NO_VIZIMAGES")) {
     return true;
   }

   SGViewer * viewer = dynamic_cast<SGViewer*>(getReceiver());
   if (viewer) {
      SFile file(getParameter("filename")->getString());
      if (VizRender::file(viewer->getTopNode(), &file,
                             getParameter("type")->getString(),
                             getParameter("width")->getInteger(),
                             getParameter("height")->getInteger(),
                             getParameter("red")->getDouble(),
                             getParameter("green")->getDouble(),
                             getParameter("blue")->getDouble())) {
      } else {
        throw EcceException(VizRender::msg(), __FILE__, __LINE__);
      }
   } else {
     throw EcceException("Could not cast to SGViewer", __FILE__, __LINE__);
   }
   return true;
}

bool RenderFileCmd::isUndoable() const
{
  return false;
}

void RenderFileCmd::undo()
{
}
