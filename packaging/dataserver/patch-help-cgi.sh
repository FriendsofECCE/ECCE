#!/bin/bash
# Patches the vendored toolhelp/cshelp help CGI scripts (extracted from
# data/admin/dataserver/help/eccehelp.tar at build time) for this install:
# the 1990s PNNL shebang path, hardcoded server help_dir, and a bare
# `require 'global.pl'` that relies on "." being in Perl's @INC -- true in
# the 1990s, false since Perl 5.26 removed it by default for security.
# Invoked from CMakeLists.txt's ecce_help_cgi_patch build step -- kept as
# a standalone script rather than inlined sed commands in CMakeLists.txt
# because the nested CMake/shell/sed quoting for a literal
# "$help_dir = ..." pattern doesn't survive CMake's own command-argument
# escaping.
set -eu

cgi_dir="$1"
# The final installed location, NOT $cgi_dir (the build-tree extraction
# path) -- global.pl must resolve here at runtime regardless of where
# toolhelp/cshelp actually execute from (the installed copy directly, or
# the per-user copy ecce-dataserver-start seeds into ~/.ECCE/dataserver).
installed_cgi_dir="/opt/ecce/data/client/WebHelp/EcceHelp/cgi-bin"

sed -i -e 's|^#!/msrc/apps/bin/perl|#!/usr/bin/perl|' \
       -e "s|^require 'global.pl';|require '$installed_cgi_dir/global.pl';|" \
  "$cgi_dir/toolhelp" "$cgi_dir/cshelp"

sed -i "s|^\$help_dir = .*|\$help_dir = '/opt/ecce/data/client/WebHelp/EcceHelp';|" \
  "$cgi_dir/global.pl"

chmod +x "$cgi_dir/toolhelp" "$cgi_dir/cshelp"
