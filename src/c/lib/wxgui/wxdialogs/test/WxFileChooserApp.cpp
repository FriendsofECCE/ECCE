#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
using namespace std;

#include "util/JMSMessage.hpp"

#include "wxgui/WxJMSSubscriber.hpp"
#include "wxgui/WxJMSMessageDispatch.hpp"
#include "wxgui/WxFileChooser.hpp"

#include "WxFileChooserApp.hpp"


IMPLEMENT_APP(WxFileChooserApp)


WxFileChooserApp::WxFileChooserApp() : WxAppJMSMessageDispatch("WxFileChooserApp") 
{
}


bool WxFileChooserApp::OnInit()
{
  ewxApp::OnInit();

  WxFileChooser * dialog = new WxFileChooser(NULL);
  dialog->ShowModal();
  cerr << "Selected file:n" << dialog->getFullPath() << endl;

  dialog->Destroy();

  subscribeMessages();
  startSubscriber();
  notifyReady();

  return true;
}


string WxFileChooserApp::identify()
{
  return getName();
}
