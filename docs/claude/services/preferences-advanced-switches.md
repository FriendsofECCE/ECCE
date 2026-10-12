---
type: map
title: "Preferences > Advanced: environment switches are applied by Ecce::initialize, and an explicit variable wins"
area: services
paths: ["include/util/EnvSwitches.H", "src/util/genutil/EnvSwitches.C", "src/util/genutil/Ecce.C", "src/wxgui/wxdialogs/GlobalPrefs.C", "tests/envswitches/envswitches_test.C", "tests/apps/advanced_prefs_test.py"]
issues: [307]
---
The testing switches (`ECCE_NATIVE_DIALOGS`, `ECCE_GL_PERPIXEL=0`,
`ECCE_NO_PROGRESS_BACKFILL`, `ECCE_DEBUG_*`) can be ticked in Edit >
Preferences > Advanced, for Windows users who start ECCE from the Start
menu. The list is one table, `EnvSwitches::all()`; the tab is built from it.

- Stored in `EcceGlobal` (`Advanced.<Name>` booleans, `.File` for the two
  log files). There is one copy of GlobalPrefs (Organizer and Gateway share
  the class); no launcher change is needed.
- `EnvSwitches::apply()` runs in `Ecce::initialize()`, so every ECCE program
  sets the variables in its own process before reading them, and children
  inherit them. No launcher script is involved, so it works on Windows
  where there is no gateway or `ecce` wrapper.
- A variable set by hand wins. A value this module set is listed in
  `ECCE_PREF_ENV`, which children inherit; only listed names are replaced or
  removed when a switch is unticked. Do not test "is it set" with a plain
  `getenv` in a new switch: use `EnvSwitches::setExplicitly`.
- A program that is already running keeps its switches until restarted (the
  tab says so); a program started later reads the preference again.
- The log files default to `<user ECCE folder>/mosym.log` and `davauth.log`.
