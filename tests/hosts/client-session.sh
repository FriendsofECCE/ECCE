#!/bin/bash
# Runs inside a client container as the account under test (#223).
#   client-session.sh start   Xvfb :9 and `ecce -remote`; wait for the login
#                             dialog (exit 0 once it is up)
#   client-session.sh close   close that dialog like a window manager's close
#                             button; exit 0 once `ecce` has returned
export DISPLAY=:9 HOST="$(hostname)"
LOG="$HOME/ecce.log"
case "${1:-}" in
  start)
    setsid nohup Xvfb :9 -screen 0 1280x1024x24 >/dev/null 2>&1 </dev/null &
    sleep 2
    : >"$LOG"
    setsid bash -c "ecce -remote >>'$LOG' 2>&1; echo exit=\$? >>'$LOG'" \
      >/dev/null 2>&1 </dev/null &
    for _ in $(seq 90); do
      xwininfo -root -tree 2>/dev/null | grep -q '"ECCE Authentication"' && exit 0
      grep -q '^exit=' "$LOG" && exit 1
      sleep 1
    done
    exit 1 ;;
  close)
    # A close sent before wx runs its event loop is lost; ask again.
    for attempt in 1 2 3; do
      python3 /usr/local/bin/closewin.py :9 "ECCE Authentication" ||
        { echo "no login dialog on :9"; xwininfo -root -tree | grep '"'; tail -3 "$LOG"; exit 2; }
      for _ in $(seq 15); do
        grep -q '^exit=' "$LOG" && exit 0
        sleep 1
      done
    done
    echo "ecce did not return after three closes"; tail -5 "$LOG"; exit 1 ;;
esac
