#!/usr/bin/env bash
#
# Exit 0 if the engine's RAM matches the console's for the first N iterations of
# the movie, non-zero if it diverges. See README.md in this directory.
#
#     tools/check.sh [iterations]        # default 5000
#
# The console's dump is the slow half, so it is recorded once and kept. It is the
# whole movie (~146MB) and any N up to its length is served by slicing it, which
# is why this is cheap to re-run. Point $SMB_REF somewhere else, or delete it, to
# record it again.
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

[ -f "$rom" ] || { echo "no ROM at $rom" >&2; exit 2; }

# The console. Recorded only if we do not already have enough of it.
have=0
[ -f "$reference" ] && have=$(( $(stat -c%s "$reference") / record ))

if [ "$have" -lt "$needed" ]; then
    echo "recording the console to $reference (the whole movie; this takes a while)"
    log=$(mktemp)
    trap 'rm -f "$log"' EXIT

    SMB_REF=$reference SDL_AUDIODRIVER=dummy \
        fceux --loadlua tools/nmifull.lua --playmov "$movie" "$rom" >"$log" 2>&1 || true

    have=$(( $(stat -c%s "$reference") / record ))
    if [ "$have" -lt "$needed" ]; then
        echo "FCEUX recorded $have iterations, need $needed" >&2
        exit 2
    fi
fi

# Ours. The ROM is read from the working directory, hence the subshell.
ours=$(mktemp)
trap 'rm -f "$ours" ${log:-}' EXIT

make -C build smbc >/dev/null

# The frames the console lagged on, which the movie holds unread input for. Without
# them the engine has to guess where they are, and the comparison ends at the first
# one it guesses wrong, which is iteration 50,124 of this movie. See tools/README.md.
[ -f "$lag" ] || { echo "no lag frames at $lag; see tools/lagframes.lua" >&2; exit 2; }

(cd build && ./smbc ram "../$movie" "$iterations" "$ours" --lag "$(paste -sd, "../$lag")") >/dev/null

# Compare, skipping the cells that differ for reasons that are not bugs.
SMB_REF=$reference python3 tools/itercmp.py "$ours"
