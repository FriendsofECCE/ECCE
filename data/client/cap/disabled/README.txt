Disabled computational-code capability files
=============================================

The .edml files in this directory were moved out of data/client/cap/
(the directory CodeFactory::getCodes() scans, non-recursively, to build
the list of registered codes) on 2026-08-30.

These four were never anything more than embryonic stubs to begin with:
each has real basis-set-translation metadata (a BasisTranslationScript
entry, and a matching script actually exists under scripts/parsers/),
but none ever had the InputGenerator/Template wiring needed to actually
set up and submit a calculation, and none were ever reachable from any
"New ... Calculation" menu (confirmed: zero references anywhere in
ResourceDescriptor.xml/ResourceDescriptorRxn.xml). Moving them here
doesn't remove any working feature or orphan any existing calculation
data -- there was never a way to create one of these in the first place.

To re-enable one: move its .edml back up into data/client/cap/. That
alone won't add a menu entry (none ever existed) -- someone would still
need to add InputGenerator/Template to the .edml and build the actual
job-setup/submission wiring, i.e. finish the embryonic integration, not
just flip a switch.

See data/client/config/disabled-codes-archive.txt for the separate,
larger removal of AMICA/Gaussian-03/Gaussian-98/MetaDyn's *menu*
registrations (those codes were fully wired and had real menu entries,
unlike these four -- a different, easier-to-reverse kind of removal).
