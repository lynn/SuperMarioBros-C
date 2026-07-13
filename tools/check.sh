#!/usr/bin/env bash
#
# Exit 0 if the engine's RAM matches the console's for the first N iterations of
# the movie, non-zero if it diverges. See README.md in this directory.
#
#     tools/check.sh [iterations]        # default 71377, the whole movie
#
# The console's dump is the slow half, so it is recorded once and kept. It is the
# whole movie (~146MB) and any N up to its length is served by slicing it, which
# is why this is cheap to re-run. Point $SMB_REF somewhere else, or delete it, to
# record it again.
#
# It says which phase it is in on stderr as it goes; the comparison's report goes
# to stdout.
#
set -euo pipefail

iterations=${1:-71377}
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"

reference=${SMB_REF:-/tmp/smb-fceux.bin}
movie=smb-allitems.fm2
lag=smb-allitems.lag
rom="build/Super Mario Bros. (JU) (PRG0) [!].nes"

record=2048          # one record is 2KB of NES RAM, one iteration of the game
shift_records=1      # FCEUX records at NMI entry: our record i is their i + 1
needed=$((iterations + shift_records))

started=$SECONDS
log() { printf '[check %5ds] %s\n' "$((SECONDS - started))" "$*" >&2; }

log "comparing $iterations iterations of $movie against the console"
log "engine: build/smbc   rom: $rom"

[ -f "$rom" ] || { echo "no ROM at $rom" >&2; exit 2; }

# The console. Recorded only if we do not already have enough of it.
have=0
[ -f "$reference" ] && have=$(( $(stat -c%s "$reference") / record ))

if [ "$have" -lt "$needed" ]; then
    if [ "$have" -eq 0 ]; then
        log "no console dump at $reference"
    else
        log "console dump at $reference has only $have of the $needed iterations needed"
    fi
    log "phase 1/3: recording the console with FCEUX (the whole movie; this takes a while)"
    log_file=$(mktemp)
    trap 'rm -f "$log_file"' EXIT

    SMB_REF=$reference SDL_AUDIODRIVER=dummy \
        fceux --loadlua tools/nmifull.lua --playmov "$movie" "$rom" >"$log_file" 2>&1 || true

    have=$(( $(stat -c%s "$reference") / record ))
    if [ "$have" -lt "$needed" ]; then
        echo "FCEUX recorded $have iterations, need $needed" >&2
        echo "--- FCEUX output ---" >&2
        cat "$log_file" >&2
        exit 2
    fi
    log "recorded $have iterations to $reference"
else
    log "phase 1/3: reusing the console dump at $reference ($have iterations, need $needed)"
fi

# Ours. The ROM is read from the working directory, hence the subshell.
ours=$(mktemp)
trap 'rm -f "$ours" ${log_file:-}' EXIT

log "phase 2/3: building smbc"
make -C build smbc >/dev/null

# The frames the console lagged on, which the movie holds unread input for. Without
# them the engine has to guess where they are, and the comparison ends at the first
# one it guesses wrong, which is iteration 50,124 of this movie. See tools/README.md.
[ -f "$lag" ] || { echo "no lag frames at $lag; see tools/lagframes.lua" >&2; exit 2; }

log "running the engine for $iterations iterations ($(wc -l <"$lag") lag frames from $lag)"
(cd build && ./smbc ram "../$movie" "$iterations" "$ours" --lag "$(paste -sd, "../$lag")") >/dev/null
log "engine wrote $(( $(stat -c%s "$ours") / record )) iterations"

# Compare, skipping the cells that differ for reasons that are not bugs.
log "phase 3/3: comparing"
if SMB_REF=$reference python3 tools/itercmp.py "$ours"; then
    log "match: the engine and the console agree for all $iterations iterations"
else
    status=$?
    log "MISMATCH: see the divergence above"
    exit $status
fi
