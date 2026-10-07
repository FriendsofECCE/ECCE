#!/bin/bash
# Thin wrapper kept for existing callers: run-offload.sh pinned to radium.
exec "$(dirname "$0")/run-offload.sh" --host radium "$@"
