///////////////////////////////////////////////////////////////////////////////
// FILENAME: SpinMult.C
//
//
// CLASS SYNOPSIS:
//    Used to convert between spin mult enumerated values and text strings.
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::cout;
  using std::endl;

#include <ctype.h>  // for tolower()
#include <stdio.h>

#include "util/ErrMsg.H"
#include "tdat/SpinMult.H"

// PUBLIC STATIC MEMBER FUNCTIONS

/*************************************************************************
 Method:  toString
 Summary: Converts enumerated value to a string
*************************************************************************/
string SpinMult::toString (SpinMult::SpinMultEnum enumValue)
{
   string result;
   switch (enumValue)
   {
      case SpinMult::unknown : result = "Unknown"; break;
      case SpinMult::singlet : result = "Singlet"; break;
      case SpinMult::doublet : result = "Doublet"; break;
      case SpinMult::triplet : result = "Triplet"; break;
      case SpinMult::quartet : result = "Quartet"; break;
      case SpinMult::quintet : result = "Quintet"; break;
      case SpinMult::sextet  : result = "Sextet" ; break;
      case SpinMult::septet  : result = "Septet" ; break;
      case SpinMult::octet   : result = "Octet"  ; break;
      case SpinMult::nontet  : result = "Nontet" ; break;
      default :
        char buf[16];   
        sprintf(buf, "%d", (int)enumValue);
        result = buf;
   };
   return result;
}

/*************************************************************************
 Method:  toSpinMult
 Summary: Converts string to enumerated value
*************************************************************************/
SpinMult::SpinMultEnum SpinMult::toSpinMult(const string& value)
{ 
  SpinMult::SpinMultEnum enumValue;

  if (value == "Singlet" || value == "singlet" || value == "SINGLET")
    enumValue = SpinMult::singlet;
  else if (value == "Doublet" || value == "doublet" || value == "DOUBLET")
    enumValue = SpinMult::doublet;
  else if (value == "Triplet" || value == "triplet" || value == "TRIPLET")
    enumValue = SpinMult::triplet;
  else if (value == "Quartet" || value == "quartet" || value == "QUARTET")
    enumValue = SpinMult::quartet;
  else if (value == "Quintet" || value == "quintet" || value == "QUINTET")
    enumValue = SpinMult::quintet;
  else if (value == "Sextet" || value == "sextet" || value == "SEXTET")
    enumValue = SpinMult::sextet;
  else if (value == "Septet" || value == "septet" || value == "SEPTET")
    enumValue = SpinMult::septet;
  else if (value == "Octet" || value == "octet" || value == "OCTET")
    enumValue = SpinMult::octet;
  else if (value == "Nontet" || value == "nontet" || value == "NONTET")
    enumValue = SpinMult::nontet;
  else
    enumValue = (SpinMult::SpinMultEnum)strtoul(value.c_str(), NULL, 10);

  return enumValue;
}
