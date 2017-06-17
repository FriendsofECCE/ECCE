#ifndef _EWXSTATICBOXSIZER_H
#define _EWXSTATICBOXSIZER_H

#include <wx/sizer.h>

class ewxStaticBoxSizer : public wxStaticBoxSizer
{

    public:
        // constructors
        ewxStaticBoxSizer(wxStaticBox* box, int orient);

        ewxStaticBoxSizer(int orient, wxWindow *parent,
                          const wxString& label = wxEmptyString);


};

#endif
