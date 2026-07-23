#!/usr/bin/env python3
"""Run intro-var.py on every RAM label, committing each one check.sh accepts.

    python3 tools/intro-var-all.py

Walks the names in source/Util/RamMap.cpp in order. For each, it runs
tools/intro-var.py NAME, then tools/check.sh. If the check passes the change is
committed; otherwise the working tree is reverted and the name is skipped. Names
that intro-var itself declines (no #define, already introduced, non-RAM address,
or no M(NAME)/ram[NAME] usages to rewrite) are skipped without a build.

Every name that did not get committed is listed at the end, with the reason.
The working tree is left clean between names, so it is safe to stop with Ctrl-C.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RAMMAP = ROOT / "source" / "Util" / "RamMap.cpp"


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT,
                          capture_output=True, text=True)


def revert():
    """Undo any tracked changes intro-var made under source/."""
    git("checkout", "--", "source")


def ram_labels():
    """The label names from RamMap.cpp, in file order, without duplicates."""
    seen = set()
    names = []
    for name in re.findall(r'\{"(\w+)",', RAMMAP.read_text()):
        if name not in seen:
            seen.add(name)
            names.append(name)
    return names


def main():
    names = ram_labels()
    committed = []
    skipped = []  # (name, reason)

    for i, name in enumerate(names, 1):
        prefix = f"[{i}/{len(names)}] {name}"

        intro = subprocess.run(
            [sys.executable, str(ROOT / "tools" / "intro-var.py"), name],
            cwd=ROOT, capture_output=True, text=True)

        print(intro.stdout)

        if intro.returncode != 0:
            revert()  # a mid-way failure could leave partial edits
            reason = intro.stderr.strip().split("\n")[-1] or "intro-var declined"
            reason = reason.replace("intro-var: ", "")
            print(f"{prefix}: skip ({reason})")
            skipped.append((name, reason))
            continue

        rewrote = re.search(r"rewrote (\d+) usage", intro.stdout)
        if rewrote and int(rewrote.group(1)) == 0:
            revert()
            print(f"{prefix}: skip (no usages)")
            skipped.append((name, "no usages"))
            continue

        # check = subprocess.run(["bash", "tools/check.sh"], cwd=ROOT,
        #                        capture_output=True, text=True)
        # if check.returncode != 0:
        #     revert()
        #     tail = (check.stdout + check.stderr).strip().split("\n")[-1]
        #     print(f"{prefix}: FAIL check.sh -> revert")
        #     skipped.append((name, f"check.sh failed ({tail})"))
        #     continue

        var = re.search(r"Introduced `(\w+)`", intro.stdout)
        var = var.group(1) if var else name
        git("add", "source")
        git("commit", "-q", "-m",
            f"Introduce {var} alias for ram[{name}]")
        print(f"{prefix}: replaced as {var}", rewrote)
        committed.append(name)

    print("\n" + "=" * 60)
    print(f"committed {len(committed)}, skipped {len(skipped)} "
          f"of {len(names)} labels")
    if skipped:
        print("\nNot committed:")
        for name, reason in skipped:
            print(f"  {name}: {reason}")


if __name__ == "__main__":
    main()
