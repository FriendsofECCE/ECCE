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


Second batch, moved 2026-09-22
------------------------------

ACESII, GAMESS(US), Gaussian-92, Gaussian-94, HONDO and MOLPRO.

Same situation as the original four, and checked the same way before
moving: none has an <InputGenerator>, none is referenced by
ResourceDescriptor.xml or ResourceDescriptorRxn.xml, and none had a
dialog -- so none of them could ever set up or submit a calculation, and
none was reachable from any "New ... Calculation" menu. Gaussian-94 does
carry a <Template>, but with no generator to fill it that changes
nothing.

NWChemMD was considered and deliberately KEPT in data/client/cap: unlike
these, it IS referenced by both ResourceDescriptor files (the
nwchem_md_study path used by the MD tools), so moving it would break a
live feature. Worth re-checking that before moving anything else out of
here on the assumption that "no generator" means "unused".

Related: #101 collects the remaining retirement candidates, and #56/#57
(ACESII, HONDO) and #63 (HONDOPLUS) cover specific codes here.
