/**
 * @file
 *
 *  The executable names a code is installed under, and the file that must sit
 *  next to it.  One table for Register Machines' Find button and for the
 *  first-start registration of this computer.  No wx.
 */
#ifndef CODEPROGRAMS_HH
#define CODEPROGRAMS_HH

#include <string>
#include <vector>

namespace CodePrograms
{
    /// Names to look for, best first; the code's lower-case name if unknown.
    std::vector<std::string> names(const std::string& code);
    /// A file that must be next to the executable (ORCA: orca_2mkl), or "".
    std::string companion(const std::string& code);
    /// First match of names() on the absolute entries of $PATH: an executable
    /// file, and with a companion, one next to it after following links.
    /// Returns the resolved path, or "".
    std::string findOnPath(const std::string& code);
}

#endif
