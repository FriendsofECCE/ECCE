# ecce-session-lib.sh: the session id and the names derived from it (#233).
#
# Sourced (bash) by `ecce`, the ecce-<app> wrappers and ecce-gateway-
# {start,stop,status,reap}. A session is identified by ECCE_SESSION_ID,
# 16 lower-case hex characters made by `ecce` and inherited by every
# process of the session; DISPLAY is only what X needs. The key in file
# names and broker topics is <host>_<id>, so this host's files share a
# prefix the reaper can sweep. Ecce::sessionKey() (src/util/genutil/
# Ecce.C) is the C++ copy of ecce_session_key; tests/session checks that
# the two agree byte for byte.
#
# Callers set STATEDIR ($ECCE_REALUSERHOME/.ECCE) before using the file
# names.

ecce_new_session_id() {
  LC_ALL=C od -An -N8 -tx1 /dev/urandom | LC_ALL=C tr -d ' \n'
}

ecce_session_id_valid() {
  case "$1" in
    *[!0-9a-f]* | "") return 1 ;;
  esac
  [ "${#1}" -eq 16 ]
}

ecce_session_host() {
  if [ -n "${ECCE_HOST:-}" ]; then printf '%s' "$ECCE_HOST"
  elif [ -n "${HOST:-}" ]; then printf '%s' "$HOST"
  else hostname
  fi
}

# Anything that is not plainly file name material becomes '_', which also
# keeps the key a valid MQTT topic level (no / + #, never starting with $).
ecce_session_sanitize() {
  LC_ALL=C tr -c 'A-Za-z0-9._-' '_'
}

# <host>_<id> for the given id (default: this process's); fails without a
# well-formed one, as the C++ side does.
ecce_session_key() {
  local id="${1-${ECCE_SESSION_ID:-}}"
  ecce_session_id_valid "$id" || return 1
  printf '%s_%s' "$(ecce_session_host)" "$id" | ecce_session_sanitize
}

# What every key of this host starts with: <host>_
ecce_session_prefix() {
  printf '%s_' "$(ecce_session_host)" | ecce_session_sanitize
}

ecce_broker_file() {
  local key
  key="$(ecce_session_key "$@")" || return 1
  printf '%s/broker_%s' "$STATEDIR" "$key"
}

ecce_auth_file() {
  local key
  key="$(ecce_session_key "$@")" || return 1
  printf '%s/authcache_%s' "$STATEDIR" "$key"
}

# ~/.ECCE/session_<host>: one line, the id of the newest session started
# on this host by this account (ecce-gateway-start writes it under
# .gateway.lock), which an app started without an id joins while it lives.
ecce_session_pointer() {
  printf '%s/session_%s' "$STATEDIR" "$(ecce_session_host | ecce_session_sanitize)"
}

# Keep an inherited id (an app the gateway or `ecce` started); else join
# the newest live session of this account on this host; else make one: an
# app started on its own with no session alive is a session of its own.
ecce_session_ensure() {
  local id
  if ! ecce_session_id_valid "${ECCE_SESSION_ID:-}"; then
    : "${STATEDIR:=${ECCE_REALUSERHOME:-$HOME}/.ECCE}"
    id="$(head -n1 "$(ecce_session_pointer)" 2>/dev/null)"
    if ecce_session_id_valid "$id" && ecce_session_alive "$id"; then
      ECCE_SESSION_ID="$id"
    else
      ECCE_SESSION_ID="$(ecce_new_session_id)"
    fi
  fi
  export ECCE_SESSION_ID
}

# The ECCE_SESSION_ID in a process's environment (Linux /proc), or nothing.
ecce_session_of_pid() {
  tr '\0' '\n' <"/proc/$1/environ" 2>/dev/null |
    sed -n 's/^ECCE_SESSION_ID=//p' | head -n1
}

# Is this resolved executable one of $ECCE_HOME/bin's? Also true when
# bin/<name> is a symlink to it, as in an overlay pointing into a build
# tree -- the rule GatewayApp::otherSessionApps applies too.
ecce_is_ecce_exe() {
  local exe="${1% (deleted)}" name
  : "${_ECCE_BINDIR:=$(readlink -f "$ECCE_HOME/bin" 2>/dev/null || echo "$ECCE_HOME/bin")}"
  case "$exe" in "$_ECCE_BINDIR"/*) return 0 ;; esac
  name="${exe##*/}"
  [ -e "$ECCE_HOME/bin/$name" ] || return 1
  [ "$(readlink -f "$ECCE_HOME/bin/$name" 2>/dev/null)" = "$exe" ]
}

# Which evidence says an ECCE program is alive: its session lease
# (SessionLease.H) and, where there is a /proc, the process table.
# ECCE_SESSION_LIVENESS=lease or =proc uses one only.
ecce_use_lease() { [ "${ECCE_SESSION_LIVENESS:-}" != proc ]; }
ecce_use_proc() {
  [ "${ECCE_SESSION_LIVENESS:-}" != lease ] && [ -r /proc/self/environ ]
}

# ecce_try_lock FILE COMMAND...: runs COMMAND holding an exclusive lock on
# FILE, without waiting; status 99 when someone else holds it.
ecce_try_lock() { ecce_flock -n -E 99 "$@"; }

# flock(1), or ECCE's own subset of it where util-linux is absent (macOS).
_ECCE_LIBDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd)"
ecce_flock() {
  if command -v flock >/dev/null 2>&1; then
    flock "$@"
  elif [ -x "$_ECCE_LIBDIR/ecce-flock" ]; then
    "$_ECCE_LIBDIR/ecce-flock" "$@"
  else
    "$ECCE_HOME/bin/ecce-flock" "$@"
  fi
}

# The live leases of this host's sessions in $STATEDIR: lines
# "<pid> <name> <session id>". A lease whose lock can be taken is stale
# (its program has ended) and is removed.
ecce_session_leases() {
  local prefix d f base sid
  prefix="$(ecce_session_prefix)"
  for d in "$STATEDIR/leases/$prefix"*/; do
    [ -d "$d" ] || continue
    sid="$(basename "$d")"
    sid="${sid#"$prefix"}"
    ecce_session_id_valid "$sid" || continue
    for f in "$d"*.*; do
      [ -e "$f" ] || continue
      ecce_try_lock "$f" rm -f "$f"
      if [ $? -eq 99 ]; then
        base="${f##*/}"
        echo "${base##*.} ${base%.*} $sid"
      fi
    done
    rmdir "$d" 2>/dev/null
  done
}

# This user's running ECCE programs: lines "<pid> <name> <session id>"
# (the id empty for a program without one), each pid once.
ecce_session_procs() {
  local pid exe
  {
    if ecce_use_proc; then
      for pid in $(ls /proc 2>/dev/null | grep -E '^[0-9]+$'); do
        [ -O "/proc/$pid" ] || continue
        exe="$(readlink "/proc/$pid/exe" 2>/dev/null)" || continue
        ecce_is_ecce_exe "$exe" || continue
        [ -r "/proc/$pid/environ" ] || continue
        exe="${exe% (deleted)}"
        echo "$pid ${exe##*/} $(ecce_session_of_pid "$pid")"
      done
    fi
    if ecce_use_lease; then ecce_session_leases; fi
  } | awk '!seen[$1]++'
}

# The running ECCE programs that still use this state directory's data
# server and broker apart from the given session (default: this
# process's): every program of another session (or of none), and a job
# store or job master of any session, since a job reports through both.
# Lines "<pid> <name> <session id>". SessionLease::othersUsingServices()
# is the C++ copy, which the Quit dialogs use to decide whether to offer
# Quit and Stop Server.
ecce_others_using_services() {
  local own="${1-${ECCE_SESSION_ID:-}}" pid name sid home
  while read -r pid name sid; do
    case "$name" in
      eccejobstore | eccejobmaster) ;;
      *) [ -n "$own" ] && [ "$sid" = "$own" ] && continue ;;
    esac
    # A /proc entry may be another state directory's (lease entries are
    # this one's, and their programs carry the same environment).
    if [ -r "/proc/$pid/environ" ]; then
      home="$(tr '\0' '\n' <"/proc/$pid/environ" 2>/dev/null |
              sed -n 's/^ECCE_REALUSERHOME=//p' | head -n1)"
      [ "${home:-$HOME}" = "${ECCE_REALUSERHOME:-$HOME}" ] || continue
    fi
    echo "$pid $name $sid"
  done < <(ecce_session_procs)
}

# Is an ECCE program of this session running?
ecce_session_alive() {
  local pid name sid
  while read -r pid name sid; do
    [ "$sid" = "$1" ] && return 0
  done < <(ecce_session_procs)
  return 1
}
