/**
 * @file
 * @author Ken Swanson
 *
 */

#ifndef WXBASISTOOLAPP_HH
#define WXBASISTOOLAPP_HH

#include "util/JMSMessage.H"


#include "wxgui/ewxApp.H"

#include "WxBasisTool.H"



class WxBasisToolApp : public ewxApp
{
    public:
        WxBasisToolApp();
        virtual bool OnInit();
        virtual string getName() const;
    private:
        WxBasisTool *p_basisToolFrame;
};
DECLARE_APP(WxBasisToolApp)

#endif      // WXBASISTOOLAPP_HH
