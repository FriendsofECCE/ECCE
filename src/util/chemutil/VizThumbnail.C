/**
 * @file
 *
 */

#include "dsm/ResourceDescriptor.H"

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/StringConverter.H"
#include "util/VizThumbnail.H"

void VizThumbnail::thumbnail(const string& whoami, const string& calcurl,
                             int width, int height,
                             double r, double g, double b)
{
  vector<string> calcurls;
  calcurls.push_back(calcurl);
  VizThumbnail::thumbnail(whoami, calcurls, width, height, r, g, b);
}


void VizThumbnail::thumbnail(const string& whoami,
                             const vector<string>& calcurls,
                             int width, int height,
                             double r, double g, double b)
{
  JMSPublisher publisher(whoami);

  // Send the start app request to gateway:
  Target gateway(GATEWAY, "");
  JMSMessage * msg = publisher.newMessage(gateway);

  msg->addProperty("width",   StringConverter::toString(width));
  msg->addProperty("height",  StringConverter::toString(height));
  msg->addProperty("r",       StringConverter::toString(r));
  msg->addProperty("g",       StringConverter::toString(g));
  msg->addProperty("b",       StringConverter::toString(b));

  for (size_t i = 0; i < calcurls.size(); i++) {
    msg->addProperty("calcurl" + StringConverter::toString((int)i),calcurls[i]);
  }

  //publisher.invoke(*msg, VIZTHUMBNAIL, 0, calcurl);
  msg->addProperty("appname", VIZTHUMBNAIL);
  publisher.publish("ecce_get_app", *msg);

  delete msg;
}

