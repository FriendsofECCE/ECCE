# Scripting and AI assistants

Everything ECCE does can be done from the command line, so scripts and AI
assistants can work with it too. The commands are `ecce-launch`,
`ecce-terminate`, `ecce-reconnect`, `ecce-fetch` and `ecce-contacts`; see
[Launch Selected](launch-selected.md) for the first four.

## The AGENTS.md file

ECCE ships a file, `AGENTS.md`, that says how ECCE stores its data (projects,
calculations, `Inputs`, `Outputs`, `Props`, `.ecce-meta`, studies), what each
command does with examples and exit codes, how to check a result, and the
safety rules for working on a scientist's data. It is installed in the `doc`
folder of the ECCE installation (`/opt/ecce/doc/AGENTS.md` on a package
install), next to this help, and is `docs/AGENTS.md` in the source.

To use it, tell the assistant to read the file before it touches anything, for
example: "Read AGENTS.md from the ECCE installation and then check my latest
calculation." Assistants that look for an `AGENTS.md` in the working folder
find it when it is copied or linked there.

For Claude Code, the same instructions are packaged as a skill in the
`share/ecce/ai/skills/ecce` folder of the installation. Copy the `ecce` folder
to `~/.claude/skills/` to use it.

## What to keep in mind

- You stay in control. Ask the assistant to say what it will launch, on which
  machine and with which settings, before it does.
- An assistant can be wrong about chemistry. Check what it tells you against
  the output and against what you know.
- What an assistant reads is sent to its provider. Do not give it data that
  may not leave your computer.
- For an experiment, use a separate state directory so that your own data is
  not touched; AGENTS.md says how.
