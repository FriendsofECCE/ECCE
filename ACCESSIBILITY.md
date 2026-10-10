# Accessibility

We want ECCE to be usable by everyone who does computational chemistry,
including people who use the keyboard rather than a mouse, use a screen
reader, have reduced colour vision or low vision, or need larger text.
This page says what ECCE does today, what we aim for, and how to tell us
about a barrier.

## What ECCE does today

- **Text size:** Edit → Preferences sets the font size (Small to Extra
  Large), or follows the system's text size ("Use the system font size").
- **Themes:** on Linux, ECCE's windows follow the desktop theme, including
  dark and high-contrast themes.
- **Not colour alone:** each run state in the Organizer has its own shape
  as well as its own colour. Errors and warnings are shown in the message
  line by colour and text, and optionally by a beep.
- **Native controls:** buttons, menus, lists and text fields are the
  system's own (through wxWidgets), so they work with the system's
  keyboard navigation and screen reader support. ECCE's own drawings, the
  3-D view and the plots, are not covered by that.
- **Keyboard shortcuts** for common actions, shown next to their menu
  items.
- **Results as text:** energies, charges, dipoles, orbitals and
  frequencies are listed in tables beside the 3-D view.
- **Windows that fit:** automated tests open every window at small screen
  sizes and check that no label is cut off.
- **Help pages** in plain language, with real headings and lists, and a
  text description for every picture. They also open in a web browser.

## What we aim for

These are directions for the work, not promises with dates:

- **[WCAG 2.2 Level AA](https://www.w3.org/TR/WCAG22)** where it applies
  to a desktop program, as a guide. ECCE has not been evaluated against
  it, and we make no claim that it meets it.
- Every control has a name a screen reader can announce, and every action
  can be reached from the keyboard.
- Colour is never the only way information is shown, and the default
  colours are safe for colour vision deficiencies.
- Windows stay usable at large text sizes and display scales.
- Problems of these kinds are found by automated checks as the code
  changes, so they are caught before a release.

Known gaps are listed in
[issue #267](https://github.com/FriendsofECCE/ECCE/issues/267).

## For contributors

When you change something people see or use:

- give every control a name: a label, or a tooltip for an icon-only
  button;
- give new actions a keyboard shortcut in their menu label where it makes
  sense;
- don't use colour alone to show a difference: add a shape, a pattern, a
  label or a value;
- in the help pages, use real headings and lists, descriptive link text,
  and a text description for every picture.

## Reporting a barrier

If something in ECCE keeps you from doing what you need, please tell us.
We treat these reports as expertise, not complaints.

Open an [issue](https://github.com/FriendsofECCE/ECCE/issues/new/choose),
or email andy.ohlin@ik.me if you prefer not to post publicly. It helps to
include:

- what you were trying to do, and what went wrong;
- which window or part of ECCE;
- your ECCE version (`ecce --version`), operating system, and any
  assistive technology you use;
- a screenshot or recording, if you are comfortable sharing one.

You never need to tell us about a disability or diagnosis. ECCE is
maintained by a small team; we read every report, and barriers that stop
someone from completing a task come before other work.
