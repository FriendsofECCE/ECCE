"""Byte-compatible text and numeric operations shared by the xTB readers."""
import io
import re
import sys

WS = " \t\n\r\f\v"


def configure_streams():
    # Perl reads through LF; a bare CR must stay inside a record, not split it.
    # Job output is untrusted, including on Python 3.6 without reconfigure.
    for name in ("stdin", "stdout"):
        stream = getattr(sys, name)
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace", newline="\n")
        else:
            setattr(sys, name, io.TextIOWrapper(stream.buffer, encoding="utf-8",
                                               errors="replace", newline="\n"))


def fields(text):
    return re.findall(r"[^ \t\n\r\f\v]+", text, re.ASCII)


def number(value, integer=False):
    # Perl consumes a numeric prefix; overflow placeholders coerce to zero.
    m = re.match(r"\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)",
                 value or "", re.ASCII)
    try:
        value = float(m.group(1)) if m else 0.0
        return int(value) if integer else value
    except (ValueError, OverflowError):
        return 0


def scalar(key, value, units="Hartree"):
    print("key: {}\nsize:\n1\nvalues:\n{}\nunits:\n{}\nEND".format(
        key, value, units))


def options(spec, argv):
    import os
    args, found = list(argv), set()
    while args and args[0].startswith("-") and args[0] != "-":
        arg = args.pop(0)[1:]
        if arg == "-":
            break
        if arg in ("-help", "-version"):
            # Keep the diagnostic text for the legacy long flags, even though
            # the parser itself no longer loads that interpreter or module.
            banner = ("{} version [unknown] calling Getopt::Std::getopts "
                      "(version 1.14 [paranoid]),\nrunning under Perl version 5.40.1.\n").format(sys.argv[0])
            if arg == "-help":
                banner += "\nUsage: {} [-OPTIONS [-MORE_OPTIONS]] [--] [PROGRAM_ARG1 ...]\n".format(
                    os.path.basename(sys.argv[0].replace("\\", "/")))
                banner += "\nThe following single-character options are accepted:\n"
                if spec == "p:v":
                    banner += "\tWith arguments: -p\n\tBoolean (without arguments): -v\n"
                else:
                    banner += "\tBoolean (without arguments): -h -v\n"
                banner += "\nOptions may be merged together.  -- stops processing of options."
                if spec == "p:v":
                    banner += "\nSpace is not required between options and their arguments."
                banner += "\n"
            banner += ("  [Now continuing due to backward compatibility and excessive paranoia.\n"
                       "   See 'perldoc Getopt::Std' about $Getopt::Std::STANDARD_HELP_VERSION.]\n")
            sys.stderr.write(banner)
            continue
        raw = os.fsencode(arg)
        while raw:
            opt, raw = raw[:1], raw[1:]
            char = opt.decode("ascii", "replace")
            if char == "p" and spec == "p:v":
                if not raw and args:
                    args.pop(0)
                break
            if char in spec:
                found.add(char)
            else:
                sys.stderr.buffer.write(b"Unknown option: " + opt + b"\n")
    return found, args
