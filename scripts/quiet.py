#!/usr/bin/env python3
"""Quiet-window coordination for timing runs on a shared machine. Never waits.

    scripts/quiet.py status                        exit 0 if free, else name the holder, exit 1
    scripts/quiet.py measure WHO SECS -- CMD...    run CMD holding the window exclusively, stopped
                                                   after SECS (max 900, exit 124); exit 75 at once
                                                   if it is taken by a measurement or a noisy job
    scripts/quiet.py noisy WHO -- CMD...           run CMD now; measurers back off while it runs

The lock is flock(2) on ~/.cache/ta-lib/quiet/bench.lock plus the `holder` and
`noisy.<pid>` files beside it. Those files are the interface: every copy of this
tool on the machine, in any worktree or language, must keep that layout. The
kernel drops a flock when its holder dies, a measured CMD dies with its wrapper
(Linux), and CMD never inherits the lock fd, so no crash leaves the window stuck
or a measurement running outside it. Without fcntl (Windows) CMD runs unguarded.
"""

import datetime
import os
import re
import signal
import subprocess
import sys
import threading
from pathlib import Path

try:
    import fcntl
except ImportError:
    fcntl = None

try:
    import ctypes
    LIBC = ctypes.CDLL(None) if sys.platform.startswith("linux") else None
except (ImportError, OSError):
    LIBC = None

DIR = Path.home() / ".cache" / "ta-lib" / "quiet"
LOCK = DIR / "bench.lock"
HOLDER = DIR / "holder"
BUSY = 75
MAX_SECS = 900
PR_SET_PDEATHSIG = 1


def usage():
    print(__doc__.split("\n\n")[1], file=sys.stderr)
    return 2


def now(delta=0):
    t = datetime.datetime.now().astimezone() + datetime.timedelta(seconds=delta)
    return t.isoformat(timespec="seconds")


def alive(pid):
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        pass
    return True


def open_lock():
    DIR.mkdir(parents=True, exist_ok=True)
    return os.open(LOCK, os.O_WRONLY | os.O_APPEND | os.O_CREAT, 0o644)


def try_lock(fd, mode):
    try:
        fcntl.flock(fd, mode | fcntl.LOCK_NB)
        return True
    except OSError:
        return False


def who_holds():
    parts = []
    try:
        line = HOLDER.read_text().strip()
        m = re.search(r"\bpid=(\d+)", line)
        if line and (m is None or alive(int(m.group(1)))):
            parts.append(line)
    except OSError:
        pass
    for f in DIR.glob("noisy.*"):
        pid = f.name.split(".", 1)[1]
        try:
            if pid.isdigit() and alive(int(pid)):
                parts.append(f.read_text().strip())
            else:
                f.unlink(missing_ok=True)
        except OSError:
            pass
    return "; ".join(p for p in parts if p) or "unknown holder"


def exit_code(rc):
    return 128 - rc if rc < 0 else rc


def start(cmd, **kw):
    try:
        return subprocess.Popen(cmd, **kw), None
    except FileNotFoundError:
        print(f"quiet: {cmd[0]}: command not found", file=sys.stderr)
        return None, 127
    except PermissionError:
        print(f"quiet: {cmd[0]}: permission denied", file=sys.stderr)
        return None, 126


class Stopped(Exception):
    def __init__(self, signum):
        self.signum = signum


def interrupt(signum, _frame):
    raise Stopped(signum)


def die_with(parent):
    def hook():
        if LIBC is not None:
            LIBC.prctl(PR_SET_PDEATHSIG, signal.SIGKILL)
            if os.getppid() != parent:
                os._exit(1)
    return hook


def stop_group(p, first):
    """SIGINT first so Python tools run their cleanup, then TERM, then KILL."""
    for sig, grace in ((first, 10), (signal.SIGTERM, 5), (signal.SIGKILL, None)):
        try:
            os.killpg(p.pid, sig)
        except ProcessLookupError:
            break
        try:
            p.wait(timeout=grace)
            break
        except subprocess.TimeoutExpired:
            pass
    try:
        os.killpg(p.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    p.wait()


def run_bounded(cmd, secs):
    p, rc = start(cmd, start_new_session=True, preexec_fn=die_with(os.getpid()))
    if p is None:
        return rc
    try:
        return exit_code(p.wait(timeout=secs))
    except subprocess.TimeoutExpired:
        print(f"quiet: {secs}s cap reached, stopping the measurement", file=sys.stderr)
        rc = 124
    except KeyboardInterrupt:
        rc = 130
    except Stopped as e:
        rc = 128 + e.signum
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, signal.SIG_IGN)
    stop_group(p, signal.SIGINT)
    return rc


def measure(who, secs, cmd):
    secs = min(secs, MAX_SECS)
    if fcntl is None:
        try:
            return subprocess.call(cmd, timeout=secs)
        except subprocess.TimeoutExpired:
            return 124
    fd = open_lock()
    if not try_lock(fd, fcntl.LOCK_EX):
        print(f"quiet: busy: {who_holds()}", file=sys.stderr)
        return BUSY
    signal.signal(signal.SIGTERM, interrupt)
    signal.signal(signal.SIGHUP, interrupt)
    HOLDER.write_text(f"MEASURING by {who} pid={os.getpid()} since={now()} until<={now(secs)}\n")
    try:
        return run_bounded(cmd, secs)
    finally:
        HOLDER.write_text("")
        os.close(fd)


def noisy(who, cmd):
    if fcntl is None:
        return subprocess.call(cmd)
    fd = open_lock()
    mark = DIR / f"noisy.{os.getpid()}"
    done = threading.Event()

    def register(blocking):
        fcntl.flock(fd, fcntl.LOCK_SH | (0 if blocking else fcntl.LOCK_NB))
        if not done.is_set():
            mark.write_text(f"NOISY by {who} pid={os.getpid()} since={now()}\n")

    def register_later():
        try:
            register(True)
        except OSError:
            pass

    try:
        register(False)
    except OSError:
        print(f"quiet: note, a measurement is running ({who_holds()}); running anyway",
              file=sys.stderr)
        threading.Thread(target=register_later, daemon=True).start()
    p, rc = start(cmd)
    try:
        if p is None:
            return rc
        try:
            return exit_code(p.wait())
        except KeyboardInterrupt:
            # CMD shares our process group, so it got the same SIGINT: let it clean up.
            try:
                p.wait(timeout=30)
            except (subprocess.TimeoutExpired, KeyboardInterrupt):
                p.kill()
                p.wait()
            return 130
    finally:
        done.set()
        mark.unlink(missing_ok=True)


def main(argv):
    if not argv:
        return usage()
    verb, rest = argv[0], argv[1:]
    if verb == "status" and not rest:
        if fcntl is None:
            print("free (no locking on this platform)")
            return 0
        fd = open_lock()
        try:
            if try_lock(fd, fcntl.LOCK_EX):
                print("free")
                return 0
            print(who_holds())
            return 1
        finally:
            os.close(fd)
    if verb == "measure" and len(rest) >= 3:
        who, secs, cmd = rest[0], rest[1], rest[2:]
        if not secs.isdigit() or int(secs) == 0:
            return usage()
        cmd = cmd[1:] if cmd[0] == "--" else cmd
        return measure(who, int(secs), cmd) if cmd else usage()
    if verb == "noisy" and len(rest) >= 2:
        who, cmd = rest[0], rest[1:]
        cmd = cmd[1:] if cmd[0] == "--" else cmd
        return noisy(who, cmd) if cmd else usage()
    return usage()


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
