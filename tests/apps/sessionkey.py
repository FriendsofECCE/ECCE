"""The session key and file names (#233), written out independently of the
product so a test can check what packaging/gateway/ecce-session-lib.sh and
Ecce::sessionKey() name. tests/session checks those two against each other.
"""

import os
import re
import socket


def host(env=None):
    env = os.environ if env is None else env
    return env.get("ECCE_HOST") or env.get("HOST") or socket.gethostname()


def key(sid, env=None):
    raw = ("%s_%s" % (host(env), sid)).encode()
    return re.sub(rb"[^A-Za-z0-9._-]", b"_", raw).decode()


def brokerFile(statedir, sid, env=None):
    return os.path.join(statedir, "broker_" + key(sid, env))


def authFile(statedir, sid, env=None):
    return os.path.join(statedir, "authcache_" + key(sid, env))
