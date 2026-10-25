#!/usr/bin/env python3
"""Record the C# library's public surface in PublicAPI.Shipped.txt.

Run it once per NuGet publish, on the commit being published, then carry the
two files' change to dev as a patch, so whatever dev changed in them since the
release survives. It adds every public member the file lacks, moves
PublicAPI.Unshipped.txt's entries into it (they shipped too), and never removes a
line: a removed or changed member makes it stop, until its line is deleted by hand.

Run:  python3 scripts/csharp_public_api.py
"""

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB = os.path.join(ROOT, "ta_codegen", "output", "csharp", "library")
SHIPPED = os.path.join(LIB, "PublicAPI.Shipped.txt")
UNSHIPPED = os.path.join(LIB, "PublicAPI.Unshipped.txt")
HEADER = "#nullable enable"


def diagnostics():
    """The RS0016 (unlisted) and RS0017 (listed, gone) symbols, the other errors, and the exit code."""
    # The patterns below read the English messages.
    env = dict(os.environ, TalibApiRefresh="true", DOTNET_CLI_UI_LANGUAGE="en")
    # --no-incremental: a build that skips the compiler reports nothing.
    out = subprocess.run(["dotnet", "build", "TALib.csproj", "-c", "Release", "--nologo",
                          "--no-incremental"],
                         cwd=LIB, env=env, capture_output=True, text=True)
    text = out.stdout + out.stderr
    added = set(re.findall(r"RS0016: Symbol '(.*?)' is not part of the declared public API", text))
    gone = set(re.findall(r"RS0017: Symbol '(.*?)' is part of the declared API", text))
    other = [l for l in text.splitlines()
             if re.search(r": error [A-Z]+\d+:", l) and not re.search(r"RS001[67]:", l)]
    return added, gone, other, out.returncode


def main():
    added, gone, other, code = diagnostics()
    if other:
        print("\n".join(sorted(set(other))))
        print("FAILED: the library does not build for another reason")
        return 1
    if gone:
        print("\n".join(sorted(gone)))
        print("FAILED: %d listed member(s) removed or changed. If intended, delete "
              "their lines from %s and run this again." % (len(gone), os.path.relpath(SHIPPED, ROOT)))
        return 1
    if code != 0 and not added:
        print("FAILED: the build failed with no diagnostic this script reads")
        return 1

    def entries(path):
        with open(path) as f:
            return {l.rstrip("\n") for l in f if l.strip() and l.strip() != HEADER}

    moved = entries(UNSHIPPED)
    removed = sorted(l for l in moved if l.startswith("*REMOVED*"))
    if removed:
        print("\n".join(removed))
        print("FAILED: %s records %d removal(s). Delete those lines and their members' lines "
              "from %s by hand, then run this again."
              % (os.path.basename(UNSHIPPED), len(removed), os.path.relpath(SHIPPED, ROOT)))
        return 1
    merged = sorted(entries(SHIPPED) | moved | added, key=lambda l: (l.lower(), l))
    with open(SHIPPED, "w", newline="\n") as f:
        f.write(HEADER + "\n" + "".join(l + "\n" for l in merged))
    with open(UNSHIPPED, "w", newline="\n") as f:
        f.write(HEADER + "\n")

    added_again, gone_again, other_again, code = diagnostics()
    if added_again or gone_again or other_again or code != 0:
        print("FAILED: the build still disagrees with the file after the update")
        return 1
    print("%s: %d member(s) added, %d moved from %s, %d listed"
          % (os.path.relpath(SHIPPED, ROOT), len(added), len(moved),
             os.path.basename(UNSHIPPED), len(merged)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
