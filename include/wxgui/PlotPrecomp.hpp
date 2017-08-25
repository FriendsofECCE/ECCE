// precomp.h

#ifndef _WXPLOTCTRL_PRECOMP_H_
#define _WXPLOTCTRL_PRECOMP_H_

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>          // includes everything wxprec.h does
    #include <wx/menu.h>
    #include <wx/toolbar.h> 
    #include <wx/combobox.h> 
    #include <wx/checkbox.h> 
    #include <wx/msgdlg.h>
    #include <wx/filedlg.h>
#endif // WX_PRECOMP

#include <wx/dcclient.h>

// DO NOT INCLUDE ANY project headers here
// We are precompiling other people's headers. 
// If we include our headers and change them the compile will be much slower.

#endif // _WXPLOTCTRL_PRECOMP_H_
