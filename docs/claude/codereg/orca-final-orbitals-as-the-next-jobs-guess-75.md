---
type: pitfall
title: "ORCA final orbitals as the next job's guess (#75): six silent traps on the way from the .gbw to MORead"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["data/client/cap/ORCA.edml", "scripts/parsers/ai.orca", "scripts/parsers/orca.tpl", "data/client/config/verify/ORCA.words", "src/dsm/edsiimpl/OrbitalGuess.C", "src/dsm/edsiimpl/TaskJob.C", "src/dsm/edsiimpl/DavCalculation.C", "src/comm/commxt/Launch.C", "src/comm/commxt/JobStore.C", "src/apps/organizer/CalcMgr.C", "src/apps/calced/CalcEd.C", "tests/launch/orca_guess_test.py"]
issues: [75]
---
"Duplicate with Final Orbitals" (Organizer) copies the last geometry and
the source's `orca.orcain.gbw` into a duplicate, which then reads it with
`! MORead` and `%moinp "guess.gbwin"`; the Calculation Editor shows
"Initial guess: MOs from <calc>" (and "projected from <basis>" when the
basis differs). What does not follow from the code:

1. **Declaring a file in `<DataFiles>` does not upload it.** The job
   monitor sends the parse/output files and `filesToTransfer`; the job
   store copies and stores only those. An `<Output type="auxiliary">` is
   just a name the code knows (NWChem's `movec.nw_mo` is declared and, as
   far as the code reads, never sent). ORCA's `.gbw` is sent because
   `<RetainAuxiliaryOutput>yes</RetainAuxiliaryOutput>` in the `.edml`
   makes `Launch` write `filesToTransfer orca.orcain.gbw` into the monitor
   config and `JobStore` store the file that arrived. The flag is per
   code on purpose: switching it on for NWChem would start uploading
   MO files of every NWChem job on every installation.
2. **The stream form of a stored file is not binary safe.**
   `DavEDSI::getDataSet()` builds an `istrstream` from a C string, so the
   file ends at its first NUL byte; a `.gbw` comes back truncated from a
   data server (local mode, `FileEDSI`, is fine, which hides it). Anything
   binary goes through `getAnyFile(url, SFile*)` (the `ostream&` form) and
   is read back with `ios::binary`; `Launch` does this for the guess file
   and `copyStoredFile`'s line loop would also have mangled it.
3. **Every Save removes all of a calculation's input files.** The
   generator runs, `TaskJob::removeInputFiles()` clears `Inputs/`, the new
   deck is stored. The guess is an input, so it is kept there by name
   (`TaskJob::orbitalGuessFile()`); without that the first Save after the
   duplicate silently dropped it and the next deck had no MORead. Also
   why `ai.orca` is told about the guess by `CalcEd::write_setup`
   (`ES.Theory.SCF.OrbitalGuessFile`, only while the file is attached),
   not by a dialog key: `p_GUIValues` is replaced on theory changes
   ([calced-owns-use-symmetry-key](calced-owns-use-symmetry-key.md)).
4. **The input copy has its own name and type** (`guess.gbwin`,
   `chemical/x-orca-guess`), the output is `orca.orcain.gbw`
   (`chemical/x-orca-gbw`). `TaskJob::getDataFiles()` matches by MIME type
   across inputs and outputs together, so one shared type would have made
   a calculation's own `.gbw` an "auxiliary input" of itself and
   staged it. Both extensions need an `AddType` in
   `packaging/dataserver/httpd.conf.ecce` and a line in
   `data/client/config/mimetypes` ([datafiles filenames](datafiles-filenames-need-a-distinctive-extension.md)).
   ORCA accepts any extension in `%moinp` (verified, ORCA 6.1.1).
5. **`moread` has to be in `data/client/config/verify/ORCA.words`**, or
   Verify reports the `!` line as UNSURE. And a duplicate made without a
   geometry trace stays READY with the source's deck, which has no MORead,
   so the Organizer handler sets it back to CREATED to force a new deck.

6. **ORCA names its `.gbw` after the whole input file name**, extension
   included: `orca.orcain` gives `orca.orcain.gbw`, not `orca.gbw`. The
   first declaration was `orca.gbw`, nothing matched it, and no file was
   stored; the end-to-end test is what showed it.

Measured with ORCA 6.1.1 (water, HF, TightSCF, 1 core): default guess 11
cycles in def2-SVP and 12 in def2-TZVP; the def2-SVP orbitals as guess:
2 cycles in def2-SVP, and 13 in def2-TZVP, where ORCA reports "Input basis
set needs projection" and the same energy. Projection does not shorten an
easy case; it is for the hard ones (open-shell metal complexes), which the
test does not cover.

Test: `tests/launch/orca_guess_test.py` (local data; `--central` for a
data server and binary files over WebDAV).
