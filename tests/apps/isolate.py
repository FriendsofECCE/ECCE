"""Keep a run of this suite out of a real, live ECCE session.

A test run brings up the same two per-user services a person's session uses
-- the ActiveMQ broker plus its JMSDispatcher, and the per-user Apache that
is the data server -- and those services are keyed by state on disk and by
fixed ports.  Overlapping with a developer's own running ECCE therefore does
not produce a tidy "port in use" error.  It produces two brokers contending
for one `~/.ECCE/activemq` data directory, a data server that early-exits
because "something is already listening" and then serves somebody else's
document root, and a pile of app failures that read exactly like application
bugs.  That is the class of false failure that makes a suite untrustworthy,
so this module exists to make the overlap impossible rather than unlikely.

Four things have to move together, and the reason this was got wrong before
is that they are in four different places:

  * **the state directory** -- `$ECCE_REALUSERHOME/.ECCE`, which holds the
    preferences, the JMS port files, the ActiveMQ data directory and the
    data server's entire document root.  Both the C++ (`Ecce::realUserHome`)
    and every service script honour `ECCE_REALUSERHOME`.  `ECCE_TEST_STATE`
    was documented as the way to move it -- but nothing ever exported it as
    `ECCE_REALUSERHOME`, so it moved only `fixture.py`'s idea of where the
    state was, and not one of the services.
  * **the two ports** -- `ECCE_DATASERVER_PORT` and `ECCE_BROKER_PORT`, read
    by the service scripts.
  * **`siteconfig/DataServers`** -- which is where the *apps* learn the data
    server's URL, and it is written at package time with a hardcoded
    `http://localhost:8096/Ecce`.  Moving the port without moving this makes
    the apps talk to whatever is on 8096, i.e. to the real session, which is
    worse than not moving the port at all.  It lives under `$ECCE_HOME`,
    which is root-owned, so it is moved by giving the run its own `ECCE_HOME`
    -- a directory of symlinks to the installed tree with one real
    `siteconfig/` of its own.
  * **`ECCE_HELP`**, hardcoded to port 8096 in the app wrappers for the same
    reason.

Everything is applied to `os.environ`, so it reaches the service scripts,
`ecce-dataserver-adduser` and every app launched afterwards without any of
them needing to know about the suite.
"""

import os
import re
import shutil
import socket

#  Deliberately not 8096/8088.  A suite whose default ports are the real
#  ones is one forgotten flag away from the collision this module exists to
#  prevent.
DEFAULT_DATASERVER_PORT = 8296
DEFAULT_BROKER_PORT = 8288


class IsolationError(Exception):
    pass


def defaultStateDir():
    """Where an isolated run keeps its state when nothing says otherwise.

    A fixed path rather than a fresh temporary directory, so successive runs
    reuse the seeded document root and the synced basis-set library instead
    of paying for them every time.  Never `~/.ECCE`.
    """
    cache = (os.environ.get("XDG_CACHE_HOME")
             or os.path.join(os.path.expanduser("~"), ".cache"))
    return os.path.join(cache, "ecce-apps-suite")


def _portFree(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(0.5)
        return sock.connect_ex(("127.0.0.1", port)) != 0


def _pickPort(name, preferred):
    """A free port, starting from `preferred`.

    An explicitly requested port is used as given: if it is busy, say so
    rather than quietly serving on a different one than the caller arranged
    the rest of their environment for.
    """
    explicit = os.environ.get(name)
    if explicit:
        port = int(explicit)
        if not _portFree(port):
            raise IsolationError(
                "%s=%d, but something is already listening there.  Stop it, "
                "or leave %s unset and a free port will be chosen."
                % (name, port, name))
        return port
    for port in range(preferred, preferred + 40):
        if _portFree(port):
            return port
    raise IsolationError("no free port found near %d for %s"
                         % (preferred, name))


def _link(source, target):
    if os.path.lexists(target):
        return
    os.symlink(source, target)


def homeOverlay(install, state, dataserverPort, brokerPort):
    """An `$ECCE_HOME` that differs from the installed one only in siteconfig.

    `siteconfig/DataServers` names the data server's port and `jndi.
    properties` the broker's, and both live in a root-owned install that a
    test cannot edit.  Symlinking everything else and owning a real
    `siteconfig/` costs a 128K copy and makes the whole of `$ECCE_HOME`
    honest about which instance this run is talking to.
    """
    home = os.path.join(state, "ecce-home")
    siteconfig = os.path.join(home, "siteconfig")
    if os.path.isdir(siteconfig):
        shutil.rmtree(siteconfig)
    os.makedirs(home, exist_ok=True)
    for entry in sorted(os.listdir(install)):
        if entry == "siteconfig":
            continue
        _link(os.path.join(install, entry), os.path.join(home, entry))
    shutil.copytree(os.path.join(install, "siteconfig"), siteconfig,
                    symlinks=True)

    _rewrite(os.path.join(siteconfig, "DataServers"),
             (r"(<(?:Url|BasisSet)>\s*http://[^:<\s]+):\d+",
              r"\1:%d" % dataserverPort),
             expect=":%d" % dataserverPort)
    _rewrite(os.path.join(siteconfig, "jndi.properties"),
             (r"(java\.naming\.provider\.url[ \t]*=[ \t]*tcp://[^:\s]+):\d+",
              r"\1:%d" % brokerPort),
             expect=":%d" % brokerPort)
    return home


def _rewrite(path, substitution, expect):
    """Apply one regex substitution to a config file and CHECK it took.

    A silent no-op here is the worst possible outcome: the run would look
    isolated and the apps would go on talking to port 8096, which is the
    real session.  So the result is verified rather than assumed.
    """
    if not os.path.exists(path):
        raise IsolationError("%s is missing from the installed siteconfig"
                             % path)
    pattern, replacement = substitution
    with open(path) as handle:
        original = handle.read()
    rewritten, count = re.subn(pattern, replacement, original)
    if not count or expect not in rewritten:
        raise IsolationError(
            "could not repoint %s at this run's own ports -- its contents do "
            "not match %r, so the apps would still be talking to the real "
            "session" % (path, pattern))
    with open(path, "w") as handle:
        handle.write(rewritten)


def apply(install, state=None):
    """Redirect this process's environment at a private ECCE instance.

    Returns a dict of what was set, for the run's own log.  Every value goes
    into `os.environ`, so every subprocess inherits it.
    """
    state = state or os.environ.get("ECCE_TEST_STATE") or defaultStateDir()
    state = os.path.abspath(os.path.expanduser(state))

    real = os.path.realpath(os.path.expanduser("~"))
    if os.path.realpath(state) == real:
        raise IsolationError(
            "the isolated state directory is the real home directory (%s); "
            "that is the collision this is meant to prevent" % state)

    dataserverPort = _pickPort("ECCE_DATASERVER_PORT",
                               DEFAULT_DATASERVER_PORT)
    brokerPort = _pickPort("ECCE_BROKER_PORT", DEFAULT_BROKER_PORT)

    os.makedirs(os.path.join(state, ".ECCE"), exist_ok=True)
    home = homeOverlay(install, state, dataserverPort, brokerPort)

    settings = {
        "ECCE_REALUSERHOME": state,
        "ECCE_TEST_STATE": state,
        "ECCE_DATASERVER_PORT": str(dataserverPort),
        "ECCE_BROKER_PORT": str(brokerPort),
        "ECCE_HOME": home,
        "ECCE_HELP": "http://localhost:%d/" % dataserverPort,
    }
    os.environ.update(settings)
    return settings


def describe(settings):
    return ("isolated: state %s, data server :%s, broker :%s"
            % (settings["ECCE_REALUSERHOME"],
               settings["ECCE_DATASERVER_PORT"],
               settings["ECCE_BROKER_PORT"]))
