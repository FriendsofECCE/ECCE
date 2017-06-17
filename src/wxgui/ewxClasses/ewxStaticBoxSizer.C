#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxStaticBoxSizer.H"


ewxStaticBoxSizer::ewxStaticBoxSizer(wxStaticBox* box, int orient)
        : wxStaticBoxSizer(box, orient)
{
}


ewxStaticBoxSizer::ewxStaticBoxSizer(int orient, wxWindow *parent,
                                        const wxString& label)
        : wxStaticBoxSizer(orient, parent, label)
{
}
