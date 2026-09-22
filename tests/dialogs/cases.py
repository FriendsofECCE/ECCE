"""
Declarative expectations for the dialog suite.

Same contract as ``tests/parsers/cases.py``: every allowlist is checked in
BOTH directions, so an exception that no longer applies is itself a failure
and the lists cannot rot silently.  Never add an entry to make a failure go
away -- add it because the mismatch is genuinely harmless, and say why.
"""


# ---------------------------------------------------------------------------
# Offered by a dialog, not recognised by the generator -- but harmless.
#   (code, GUIValues key) -> {offered string: why it is fine}
# ---------------------------------------------------------------------------
KNOWN_UNMAPPED = {
    ("Amica", "ES.Theory.CC.Direct"): {
        "None": "Sentinel for 'no CC direct mode'; ai.amica tests the other "
                "two values and treats anything else as off.",
    },
}

# ---------------------------------------------------------------------------
# Recognised by the generator, never offered by any dialog.
# Usually a capability the UI cannot reach (how Gaussian's double hybrids
# stayed hidden), occasionally a deliberately retired value.
#   (code, GUIValues key) -> {mapped string: why it is fine}
# ---------------------------------------------------------------------------
KNOWN_UNOFFERED = {}

# ---------------------------------------------------------------------------
# Keys whose value the generator deliberately passes through verbatim, so
# there is no finite set to diff even though the tracer found comparisons.
# ---------------------------------------------------------------------------
FREEFORM_KEYS = set()

# ---------------------------------------------------------------------------
# Open bugs.  Reported loudly under "KNOWN BUGS", do not fail the run.
# Remove the entry when the bug is fixed; the suite fails if a listed bug
# stops reproducing, so these cannot be left behind either.
#
# Each entry: (code, key_or_dialog, marker) -> explanation
# `marker` is the specific offered/mapped string, or "" for a whole-dialog
# failure.
# ---------------------------------------------------------------------------
XFAIL = {
    # Empty.  The fifteen entries this suite found on its first run were all
    # fixed in the same session:
    #   * NWChem BP86 (GGA) vs BP86 (hybrid)        -> ai.nwchem, alternation
    #   * "None" in the combined-XC list, x4        -> ai.gauss{03,09,16,98}
    #   * except wx.PyDeadObjectError, x4           -> ged{03,09,16,98}theory
    #   * ged03 CheckDependency touching SCRF
    #     widgets built only for DFT                -> ged03theory
    #   * wx.FlexGridSizer(0, 2)                    -> amicatheory
    #   * five missing commas in solvent lists      -> ged{03,09,16}theory
    #   * one missing comma in nedtheory
    # Add an entry when a check finds a real bug that is not being fixed in
    # the same change, with enough detail to act on without re-deriving it.
}

# ---------------------------------------------------------------------------
# Deliberate implicit string concatenation inside a list literal, as
# (script, line).  Should stay empty: the legitimate use of implicit
# concatenation is a long message split over lines, which is not in a list.
# ---------------------------------------------------------------------------
KNOWN_CONCATENATION = set()

# ---------------------------------------------------------------------------
# Missing commas currently in the tree.  Each makes both neighbouring entries
# unselectable and inserts a nonsense one in their place.
# ---------------------------------------------------------------------------
CONCATENATION_XFAIL = {}

# ---------------------------------------------------------------------------
# Dialog scripts that are not expected to run headlessly at all.
# ---------------------------------------------------------------------------
SKIP_DIALOGS = {}


# ---------------------------------------------------------------------------
# GUIValues restore round-trips.  See restore.py for the mechanism.
#
# Each case runs the dialog once to get its own freshly constructed state in
# restore-file format, rewrites the named fields, restores that, and asserts
# on the live widget state afterwards.  `expect` keys are GUIValues names;
# each value is a dict of live attributes ('value', 'unit', 'selection',
# 'enabled') that must match exactly.
# ---------------------------------------------------------------------------
RESTORE_CASES = [
    dict(
        name="orca-memory-stale-unit",
        script="orcatheory.py", category="DFT", theory="RDFT",
        changes={"ES.Theory.SCF.MemorySize": ("1000", "Megabytes / core")},
        expect={"ES.Theory.SCF.MemorySize":
                {"value": 1, "unit": "Gigabytes / core"}},
        why="""
            Issue #77, the live 1000x case.  A calc saved when ORCA's Memory
            Per Core meant MB (default 1000, commit 6619e05) restored into the
            field that means GB since 78bb8d0 produced "%maxcore 1000000" --
            1 TB per core -- from a dialog reading "1000 Gigabytes / core",
            with nothing typed in.  The stored unit differs from the widget's,
            so the widget must keep its own default (1) and its own unit
            label.  If this case ever restores 1000, the 1000x bug is back.
        """),

    dict(
        name="orca-memory-matching-unit",
        script="orcatheory.py", category="DFT", theory="RDFT",
        changes={"ES.Theory.SCF.MemorySize": ("12", None)},
        expect={"ES.Theory.SCF.MemorySize":
                {"value": 12, "unit": "Gigabytes / core"}},
        why="""
            The other direction, and the reason #77's fix cannot simply
            ignore stored values: when the stored unit DOES match the widget's,
            the value must still be restored.  Without this case, "keep the
            default whenever anything looks odd" would pass the case above
            while quietly breaking every normal restore.
        """),

    dict(
        name="g16-memory-stale-megawords",
        script="ged16theory.py", category="SCF", theory="RHF",
        changes={"ES.Theory.SCF.MemorySize": ("1600", "Megawords")},
        expect={"ES.Theory.SCF.MemorySize":
                {"value": 6, "unit": "Gigabytes"}},
        why="""
            The original #77 report: a Gaussian Memory field showing
            "Megawords" despite a verified-correct source, because
            FinalizeSetting() called SetUnit(unit) with whatever was persisted
            in the calc's stored data.  The unit label must come from
            ged16theory.py's own constructor, never from the restore file.
        """),

    dict(
        name="g16-menu-and-toggle-restore",
        script="ged16theory.py", category="SCF", theory="RHF",
        changes={"ES.Theory.SCF.Direct": ("To Disk", None),
                 "ES.Theory.SCF.InitialGuess": ("Huckel", None)},
        expect={"ES.Theory.SCF.Direct": {"value": "To Disk",
                                         "selection": 2}},
        why="""
            A plain, unit-free restore of a combo box, so the unit-drift logic
            added for #77 is shown not to have disturbed the ordinary path
            that the majority of fields take.
        """),
]


# ---------------------------------------------------------------------------
# Codes that are fully wired (dialog + generator + template) but deliberately
# not offered in the New Calculation menu.  Checked in both directions: a code
# listed here that IS reachable, or reachable-again, is a failure, so a
# disconnection cannot be quietly undone and this list cannot rot.
#
# The removed menu-registration XML is kept verbatim in
# data/client/config/disabled-codes-archive.txt, so any of these can be put
# back by copy-paste.
# ---------------------------------------------------------------------------
NOT_IN_MENU = {
    "Amica": "Disconnected 2026-08-30 (issue #11) along with Gaussian-03, "
             "Gaussian-98 and MetaDyn.",
    "Gaussian-03": "Disconnected 2026-08-30 (issue #11).",
    "Gaussian-98": "Disconnected 2026-08-30 (issue #11).",
    "MetaDyn": "Disconnected 2026-08-30 (issue #11); the Metadynamics editor "
               "tool registration was archived with it.",
    "GAMESS-UK": "Disconnected 2026-09-21.  Fully wired -- guktheory.py, "
                 "gukruntype.py, ai.gamess-uk, gamess-uk.tpl -- and its "
                 "files are untouched, so existing GAMESS-UK calculations "
                 "still open; only creating a new one from the menu is gone.",
    "Polyrate": "Not disconnected: reached from a reaction study node in "
                "ResourceDescriptorRxn.xml rather than from `project`, so it "
                "never appears in the project New Calculation menu by design.",
}


# ---------------------------------------------------------------------------
# <TheorySummary> keys that legitimately have no dialog widget.
#   code -> {key: why}
# ---------------------------------------------------------------------------
KNOWN_UNBUILT_SUMMARY_KEYS = {}
