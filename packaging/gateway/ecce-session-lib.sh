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

# Keep an inherited id, else make one: an app started on its own is a
# session of its own.
ecce_session_ensure() {
  if ! ecce_session_id_valid "${ECCE_SESSION_ID:-}"; then
    ECCE_SESSION_ID="$(ecce_new_session_id)"
  fi
  export ECCE_SESSION_ID
}

# The ECCE_SESSION_ID in a process's environment (Linux /proc), or nothing.
ecce_session_of_pid() {
  tr '\0' '\n' <"/proc/$1/environ" 2>/dev/null |
    sed -n 's/^ECCE_SESSION_ID=//p' | head -n1
}
