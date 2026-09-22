"""
Per-app expectations for the GUI smoke suite.

House rule, as in tests/parsers and tests/dialogs: every list is checked in
both directions, so an entry that no longer applies is itself a failure and
these cannot rot silently.  Never add an entry to make a failure go away --
add it because the behaviour is genuinely correct, and say why.
"""

# ---------------------------------------------------------------------------
# Apps that are helpers, not top-level applications: they are launched by
# other apps with arguments and are expected to exit immediately when run
# bare.  Not window-checked.
#   name -> why
# ---------------------------------------------------------------------------
HELPERS = {
    "msgdialog": "A message-box helper other apps exec with the text to "
                 "show; with no arguments it has nothing to display.",
    "passdialog": "The authentication prompt, exec'd by the apps that need "
                  "credentials.",
    "vizthumbnail": "Offscreen thumbnail renderer, run per calculation with "
                    "a structure argument; not interactive.",
    "launcher": "The app-launch helper the gateway drives over JMS.",
}

# ---------------------------------------------------------------------------
# Apps that need a document/context argument to be useful and will exit or
# refuse when started bare.  Window-checked only if they do open one.
# ---------------------------------------------------------------------------
NEEDS_CONTEXT = {}

# ---------------------------------------------------------------------------
# Known-bad apps: reproduced failures that are not being fixed right now.
# Reported loudly, do not fail the run, and a bug that stops reproducing is
# itself a failure.
# ---------------------------------------------------------------------------
XFAIL = {
    # Empty.  The one entry this suite found on its first run -- metadyn
    # dereferencing a NULL ResourceTool from getTool(METADYNAMICS) and
    # SIGSEGVing on every launch since 2026-08-30 -- was fixed in
    # MetaEd.C, packaged, installed, and confirmed gone (the run reported
    # it as a stale entry, which is what that check is for).
    #
    # The sibling audit it implies is still open: about a dozen apps do
    # the same unchecked getTool(...)->getIcon() (Builder.C:492,
    # CalcMgr.C:164, SolvateEd.C:104, WxLauncher.C:118,
    # MachineBrowser.C:145, WxMachineRegister.C:96, MDPrep.C:163,
    # Polyrate.C:106, NWDirdy.C:117, WxBasisTool.C:286,
    # PartialCharge.C:93, ...), each a latent crash of the same shape
    # waiting for its own <Tool> entry to be removed or renamed.  This
    # suite cannot see them until one actually breaks.
}

# ---------------------------------------------------------------------------
# Per-app timing overrides, for apps that legitimately take longer to build
# their first window.
# ---------------------------------------------------------------------------
TIMEOUTS = {}
