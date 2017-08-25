#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <stdio.h>  // sprintf
#include "wxgui/ewxColor.H"
#include "util/Color.H"
#include <iostream>
using namespace std;

ewxColor::ewxColor()
{
  Set(255, 255, 255);
}

ewxColor::~ewxColor()
{
}

ewxColor::ewxColor( wxColour color )
{
  Set(color.Red(), color.Green(), color.Blue());
}


ewxColor::ewxColor( const wxString & colorName )
{
  if (colorName[0] == '#' && colorName.Len() == 7)
    *this = ewxColor(Color(colorName.c_str()));
  else 
  Set(colorName);
}


ewxColor::ewxColor( const string & colorName )
{
  if (colorName[0] == '#' && colorName.length() == 7)
    *this = ewxColor(Color(colorName.c_str()));
  else
    Set(colorName.c_str());
}


ewxColor::ewxColor( const char * colorName )
{
  if (colorName[0] == '#' && strlen(colorName) == 7)
    *this = ewxColor(Color(colorName));
  else
    Set(colorName);
}


ewxColor::ewxColor( const Color& color )
{
  Set((unsigned char) (color.red()   * 255.0),
      (unsigned char) (color.green() * 255.0),
      (unsigned char) (color.blue()  * 255.0));
}


ewxColor::ewxColor( float red, float green, float blue )
{
  Set((unsigned char) (red   * 255.0),
      (unsigned char) (green * 255.0),
      (unsigned char) (blue  * 255.0));
}


ewxColor::ewxColor( int red, int green, int blue )
{
  Set(red, green, blue);
}


string ewxColor::ToString() const
{
  char buf[10];
  sprintf(buf,"#%02x%02x%02x", Red(), Green(), Blue());
  return (string) buf;
}

