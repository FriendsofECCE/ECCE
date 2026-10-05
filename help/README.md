# help/

Source of ECCE's built-in help (issue #219).

- `src/` holds the help as Markdown, one file per section. `src/index.md`
  lists the sections in reading order.
- `src/img/` holds the screenshots the text refers to. They are meant to be
  produced with `tools/screenshots/look.py`; each section marks, in an HTML
  comment next to the image, which window and state to capture.
- The build is meant to convert `src/` with python3-markdown into a
  wxHtmlHelpController book that ships in the client package. That
  conversion is not implemented yet.

Writing rules: short, task-oriented, one action per numbered step. Every
menu item, button, dialog and field name is copied from the source. Each
file ends with a `<!-- sources: ... -->` comment listing where the labels
were found, and anything not confirmed from the code is marked
`[TO CHECK: ...]`.
