#ifndef _EWXCOLOR_H
#define _EWXCOLOR_H

#include "wx/colour.h"
#include "util/Color.H"


class ewxColor : public wxColour 
{
public:
  // constructors
  ewxColor();
  ~ewxColor();
  ewxColor( wxColour color );
  ewxColor( const wxString & colorName );
  ewxColor( const string & colorName );
  ewxColor( const char * colorName );
  ewxColor( const Color& color );
  ewxColor( float red, float green, float blue);
  ewxColor( int red, int green, int blue );

  string ToString() const;
};

#endif
