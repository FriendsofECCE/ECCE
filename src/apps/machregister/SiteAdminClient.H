/**
 *  @file
 *
 *  "ecce -admin" on a -remote client: the site settings live on the central
 *  server, so a save is sent there over ssh (the RCommand transport jobs use)
 *  and applied by ecce-site-admin as the administrator's own login; then this
 *  computer's published copy is fetched again.  This client's siteconfig is
 *  only ever written by ecce-remote-setup.
 */
#ifndef SITEADMINCLIENT_HH
#define SITEADMINCLIENT_HH

#include <string>

class SiteRequest;

namespace SiteAdminClient
{
    //  The central server's host, from siteconfig/RemoteServer/DataServers;
    //  "" with err set when there is none.
    std::string serverHost(std::string& err);

    //  Copies the request to the server and runs ecce-site-admin on it.
    //  report is the server's answer, or why it could not be reached.
    bool send(const SiteRequest& request, std::string& report);

    //  ecce-remote-setup --refresh.  False, with what to do, when this
    //  computer's copy could not be fetched again.
    bool refresh(std::string& report);
}

#endif
