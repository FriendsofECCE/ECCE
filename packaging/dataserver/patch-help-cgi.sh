#!/bin/bash
# Patches the vendored help-navigation CGI scripts (extracted from
# data/admin/dataserver/help/eccehelp.tar at build time) for this install:
# the 1990s PNNL shebang path, hardcoded server help_dir, and a bare
# `require 'global.pl'` that relies on "." being in Perl's @INC -- true in
# the 1990s, false since Perl 5.26 removed it by default for security.
# Invoked from CMakeLists.txt's ecce_help_cgi_patch build step -- kept as
# a standalone script rather than inlined sed commands in CMakeLists.txt
# because the nested CMake/shell/sed quoting for a literal
# "$help_dir = ..." pattern doesn't survive CMake's own command-argument
# escaping.
#
# Only the 5 pure-navigation scripts are patched (toolhelp/cshelp/
# key_topics/examples/openindex, all of which js_eccehelp.js's startPage()
# routes to depending on which tab -- Overview/How To/Examples -- is
# active). EcceQSubmissionForm.pl and SubmitToEcceQueue.pl are left
# untouched: even patched, they'd still fail trying to reach PNNL's own
# (long gone) mail/queue servers -- a different, still out-of-scope
# problem, not an environment-path issue like the rest of this tarball.
set -eu

cgi_dir="$1"
# The final installed location, NOT $cgi_dir (the build-tree extraction
# path) -- global.pl must resolve here at runtime regardless of where
# these scripts actually execute from (the installed copy directly, or
# the per-user copy ecce-dataserver-start seeds into ~/.ECCE/dataserver).
installed_cgi_dir="/opt/ecce/data/client/WebHelp/EcceHelp/cgi-bin"

nav_scripts="toolhelp cshelp key_topics examples openindex"

sed -i -e 's|^#!/msrc/apps/bin/perl|#!/usr/bin/perl|' \
       -e "s|^require 'global.pl';|require '$installed_cgi_dir/global.pl';|" \
  $(for s in $nav_scripts; do echo "$cgi_dir/$s"; done)

sed -i "s|^\$help_dir = .*|\$help_dir = '/opt/ecce/data/client/WebHelp/EcceHelp';|" \
  "$cgi_dir/global.pl"

chmod +x $(for s in $nav_scripts; do echo "$cgi_dir/$s"; done)
