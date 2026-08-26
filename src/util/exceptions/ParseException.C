/**
 * @file
 *
 *
 */
#include "util/ParseException.H"

ParseException::ParseException() : EcceException()
{
   p_severity = ParseException::FATAL;
}


ParseException::ParseException(ParseException::Severity severity, 
                               const char *message,
                               const char *file, int line) 
                               : EcceException(message, file, line)
{
   p_severity = severity;
}

ParseException::ParseException(ParseException::Severity severity, 
                               const string& message,
                               const char *file, int line) 
                               : EcceException(message, file, line)
{
   p_severity = severity;
}



ParseException::ParseException(const ParseException& rhs)
                     : EcceException(rhs)
{
   p_severity = rhs.severity();
}



ParseException::~ParseException() noexcept
{
}


ParseException::Severity ParseException::severity() const
{ 
   return p_severity;
}

EcceException *ParseException::clone() const noexcept
{
   return new ParseException(*this);
}
