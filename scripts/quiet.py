#!/usr/bin/env python3
"""Quiet-window coordination for timing runs on a shared machine. Every wait is bounded.

    scripts/quiet.py status                        exit 0 if free with no one queued and the
                                                   machine not loaded, else say which, exit 1
    scripts/quiet.py measure WHO SECS [--queue=MAX] -- CMD...
                                                   run CMD holding the window exclusively, stopped
                                                   after SECS (max 900, exit 124). Without --queue,
                                                   exit 75 at once if the window is taken, anyone is
                                                   queued or the machine is loaded; with it, wait in
                                                   the queue, first come
                                                   first served, at most MAX (<= 900) seconds, then
                                                   exit 75
    scripts/quiet.py noisy WHO [--defer=MAX] -- CMD...
                                                   run CMD; measurers back off while it runs. With
                                                   --defer, first wait at most MAX (<= 60) seconds
                                                   while a measurement runs or is queued

The lock is flock(2) on ~/.cache/ta-lib/quiet/bench.lock plus the `holder`,
`noisy.<pid>` and `want.<pid>` files beside it. Those files are the interface:
every copy of this tool on the machine, in any worktree or language, must keep
that layout and serve the queue in the same order. All of it is kernel state or
backed by it: a measurement is the exclusive flock (`holder` is only its label),
and a `.<pid>` file appears only already locked by its owner and counts while
that lock is held. A crash or a reboot therefore leaves nothing that looks alive:
a bare pid is never trusted. A measured CMD dies with its wrapper (Linux),
and CMD never inherits a lock fd.

`measure` also refuses, or with --queue keeps waiting, while other work keeps
more than max(1.5, CPUs/8) cores busy (at most CPUs - 0.5), whoever started it;
Linux only, as it reads /proc/stat. TA_QUIET_MAX_LOAD sets that limit in cores;
`off` disables the check. CMD gets
TA_QUIET=<kind>:<pid>. Under a live wrapper a nested `noisy` skips its defer, a
`measure` nested in a measure runs its CMD within its own cap, and a `measure`
nested in a noisy job exits 75 at once: it can never get the window there.
Without fcntl (Windows) CMD runs unguarded.
"""

import datetime
import os
import re
import signal
import subprocess
import sys
import threading
import time
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
MAX_QUEUE = 900
MAX_DEFER = 60
POLL = 1
GRACE = (10, 5)
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


class Label:
    """A `<prefix>.<pid>` file that exists only while this process holds its lock: it
    is written under a hidden name, locked, then renamed into place."""

    def __init__(self, prefix, text):
        DIR.mkdir(parents=True, exist_ok=True)
        tmp = DIR / f".{prefix}.{os.getpid()}"
        self.path = DIR / f"{prefix}.{os.getpid()}"
        self.fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        fcntl.flock(self.fd, fcntl.LOCK_EX)
        os.write(self.fd, text.encode())
        os.rename(tmp, self.path)

    def drop(self):
        self.path.unlink(missing_ok=True)
        os.close(self.fd)


def started_before(pid, t):
    """Whether process `pid` was already running at epoch time `t` (Linux /proc)."""
    try:
        with open("/proc/stat") as fh:
            btime = next(int(r.split()[1]) for r in fh if r.startswith("btime"))
        with open(f"/proc/{pid}/stat") as fh:
            fields = fh.read().rsplit(")", 1)[1].split()
        if fields[0] in ("Z", "X"):
            return False
        starttime = int(fields[19])
    except (OSError, StopIteration, ValueError, IndexError):
        return False
    return btime + starttime / os.sysconf("SC_CLK_TCK") <= t + 1


def owned(f):
    """Whether `f`'s owner is alive; a label nobody owns is removed.

    Owned means its lock is held. An unlocked label (written by a copy of this tool
    that predates the locks) still counts while its pid lives and that process was
    already running when the label was written, which a reused pid never was."""
    try:
        fd = os.open(f, os.O_RDONLY)
    except OSError:
        return False
    try:
        if not try_lock(fd, fcntl.LOCK_SH):
            return True
        pid = f.name.rsplit(".", 1)[1]
        if pid.isdigit() and alive(int(pid)) and started_before(int(pid), os.fstat(fd).st_mtime):
            return True
        try:
            if os.fstat(fd).st_ino == os.stat(f).st_ino:
                f.unlink()
        except OSError:
            pass
        return False
    finally:
        os.close(fd)


def live(prefix):
    """The text of every `<prefix>.<pid>` label still held; the rest are removed."""
    for f in DIR.glob(f".{prefix}.*"):
        try:
            if time.time() - f.stat().st_mtime > 60:
                owned(f)
        except OSError:
            pass
    parts = []
    for f in sorted(DIR.glob(f"{prefix}.*")):
        try:
            if owned(f):
                parts.append(f.read_text().strip())
        except OSError:
            pass
    return "; ".join(p for p in parts if p)


def busy_cores(interval=0.5):
    """(busy, cpus): cores kept busy over `interval` and the CPUs counted, from
    /proc/stat; None where it is unavailable."""
    def sample():
        with open("/proc/stat") as fh:
            rows = [r.split() for r in fh if r.startswith("cpu") and r[3:4].isdigit()]
        return [(sum(map(int, r[1:8])), int(r[4]) + int(r[5])) for r in rows]
    try:
        a = sample()
        time.sleep(interval)
        b = sample()
    except (OSError, ValueError, IndexError):
        return None
    busy = sum(1 - (ib - ia) / (tb - ta) for (ta, ia), (tb, ib) in zip(a, b) if tb > ta)
    return round(busy, 1), len(b)


def loaded():
    """A description of the load when it is above the limit, else ''."""
    setting = os.environ.get("TA_QUIET_MAX_LOAD", "")
    if setting == "off":
        return ""
    sample = busy_cores()
    if sample is None:
        return ""
    busy, cpus = sample
    if re.fullmatch(r"[0-9]+(\.[0-9]+)?", setting):
        limit = float(setting)
    else:
        limit = min(max(1.5, cpus / 8), cpus - 0.5)
    return f"{busy:.1f} cores busy (limit {limit:g})" if busy > limit else ""


def reasons(load=""):
    """Why the window is not free: its holders, the queue and the load."""
    queued = live("want")
    parts = [who_holds(""), queued and f"queued: {queued}", load]
    return "; ".join(p for p in parts if p) or "unknown holder"


def who_holds(fallback="unknown holder"):
    parts = []
    try:
        line = HOLDER.read_text().strip()
        if line and measuring():
            parts.append(line)
    except OSError:
        pass
    parts.append(live("noisy"))
    return "; ".join(p for p in parts if p) or fallback


def with_queue(text):
    queued = live("want")
    return f"{text}; queued: {queued}" if queued else text


def older_waiter(pid):
    """Whether a live queued measurer other than `pid` came first; any at all for pid 0.

    First come is the `want` file's mtime in whole seconds, then the lower pid. The
    shell copy orders by that same key, which is what lets both serve one queue.
    """
    try:
        mine = (int((DIR / f"want.{pid}").stat().st_mtime), pid) if pid else None
    except OSError:
        mine = (0, pid)
    for f in DIR.glob("want.*"):
        other = f.name.split(".", 1)[1]
        if not other.isdigit() or int(other) == pid or not owned(f):
            continue
        if mine is None:
            return True
        try:
            if (int(f.stat().st_mtime), int(other)) < mine:
                return True
        except OSError:
            pass
    return False


def measuring():
    """Whether a measurement holds the window now: only an exclusive holder refuses
    a shared probe."""
    fd = open_lock()
    try:
        return not try_lock(fd, fcntl.LOCK_SH)
    finally:
        os.close(fd)


def parent_kind():
    """The kind of the live quiet wrapper this process runs under, else None."""
    kind, _, pid = os.environ.get("TA_QUIET", "").partition(":")
    return kind if pid.isdigit() and alive(int(pid)) else None


def marked(kind):
    return dict(os.environ, TA_QUIET=f"{kind}:{os.getpid()}")


def run_plain(cmd, env=None):
    p, rc = start(cmd, env=env)
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


def stop_group(p, first, grace):
    """SIGINT first so Python tools run their cleanup, then TERM, then KILL."""
    for sig, grace in ((first, grace[0]), (signal.SIGTERM, grace[1]), (signal.SIGKILL, None)):
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


def run_bounded(cmd, secs, grace=None):
    p, rc = start(cmd, start_new_session=True, preexec_fn=die_with(os.getpid()),
                  env=marked("measure"))
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
    stop_group(p, signal.SIGINT, grace or GRACE)
    return rc


def acquire(fd, pid):
    """'' once this process holds the window exclusively, else why it cannot yet."""
    ahead = older_waiter(pid)
    load = loaded()
    if not ahead and not load and try_lock(fd, fcntl.LOCK_EX):
        return ""
    return reasons(load)


def measure(who, secs, queue, cmd):
    secs = min(secs, MAX_SECS)
    signal.signal(signal.SIGTERM, interrupt)
    signal.signal(signal.SIGHUP, interrupt)
    parent = parent_kind()
    if parent == "measure":
        # Half the parent's grace, so this finishes stopping its own session before
        # the parent's escalation reaches this wrapper.
        return run_bounded(cmd, secs, tuple(g / 2 for g in GRACE))
    if parent == "noisy":
        print("quiet: busy: running inside a noisy job, which holds the window", file=sys.stderr)
        return BUSY
    if fcntl is None:
        try:
            return subprocess.call(cmd, timeout=secs)
        except subprocess.TimeoutExpired:
            return 124
    fd = open_lock()
    why = acquire(fd, 0)
    if why:
        if queue <= 0:
            print(f"quiet: busy: {why}", file=sys.stderr)
            os.close(fd)
            return BUSY
        want = Label("want", f"WAITING to measure: {who} pid={os.getpid()} since={now()} for<={secs}s\n")
        print(f"quiet: queued, waiting up to {queue}s: {why}", file=sys.stderr)
        try:
            deadline = time.monotonic() + queue
            while why:
                if time.monotonic() >= deadline:
                    print(f"quiet: gave up after {queue}s: {why}", file=sys.stderr)
                    os.close(fd)
                    return BUSY
                time.sleep(POLL)
                why = acquire(fd, os.getpid())
        finally:
            want.drop()
    HOLDER.write_text(f"MEASURING by {who} pid={os.getpid()} since={now()} until<={now(secs)}\n")
    try:
        return run_bounded(cmd, secs)
    finally:
        HOLDER.write_text("")
        os.close(fd)


def noisy(who, defer, cmd):
    if fcntl is None:
        return run_plain(cmd)
    if parent_kind():
        defer = 0
    deadline = time.monotonic() + defer
    # The queue first: a queued measurer drops its `want` only once it holds the lock.
    if defer and (live("want") or measuring()):
        reasons = [who_holds(""), live("want") and f"queued: {live('want')}"]
        print(f"quiet: deferring up to {defer}s: {'; '.join(r for r in reasons if r)}",
              file=sys.stderr)
    while time.monotonic() < deadline and (live("want") or measuring()):
        time.sleep(POLL)
    fd = open_lock()
    marks = []
    done = threading.Lock()

    def register(blocking):
        fcntl.flock(fd, fcntl.LOCK_SH | (0 if blocking else fcntl.LOCK_NB))
        with done:
            if marks is not None:
                marks.append(Label("noisy", f"NOISY by {who} pid={os.getpid()} since={now()}\n"))

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
    queued = live("want")
    if queued:
        print(f"quiet: note, queued measurement: {queued}", file=sys.stderr)
    try:
        return run_plain(cmd, env=marked("noisy"))
    finally:
        with done:
            for m in marks:
                m.drop()
            marks = None


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
            queued = live("want")
            free = try_lock(fd, fcntl.LOCK_EX)
            if free:
                fcntl.flock(fd, fcntl.LOCK_UN)
            load = loaded()
            if not free:
                print(reasons(load))
                return 1
            print("free" + (f" (queued: {queued})" if queued else "") + (f", but {load}" if load else ""))
            return 1 if queued or load else 0
        finally:
            os.close(fd)
    if verb == "measure" and len(rest) >= 3:
        who, secs, cmd = rest[0], rest[1], rest[2:]
        if not secs.isdigit() or int(secs) == 0:
            return usage()
        queue, cmd = option(cmd, "--queue=")
        if queue is None:
            return usage()
        cmd = cmd[1:] if cmd and cmd[0] == "--" else cmd
        return measure(who, int(secs), min(queue, MAX_QUEUE), cmd) if cmd else usage()
    if verb == "noisy" and len(rest) >= 2:
        who, cmd = rest[0], rest[1:]
        defer, cmd = option(cmd, "--defer=")
        if defer is None:
            return usage()
        cmd = cmd[1:] if cmd and cmd[0] == "--" else cmd
        return noisy(who, min(defer, MAX_DEFER), cmd) if cmd else usage()
    return usage()


def option(args, flag):
    """`(seconds, rest)` for a leading `<flag><seconds>`: 0 when absent, None when malformed."""
    if not args or not args[0].startswith(flag):
        return 0, args
    value = args[0][len(flag):]
    return (int(value), args[1:]) if value.isdigit() else (None, args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
