#!/usr/bin/env python3
"""Tests for quiet.py, each against its own throwaway HOME.

    python3 scripts/test_quiet.py
"""

import os
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path

HERE = os.path.dirname(os.path.abspath(__file__))
RUN = ("import sys; sys.path.insert(0, %r); import quiet; quiet.POLL = 0.1; "
       "quiet.GRACE = (0.5, 0.5); sys.exit(quiet.main(sys.argv[1:]))" % HERE)
CAPPED = RUN.replace("quiet.GRACE", "quiet.MAX_DEFER = quiet.MAX_QUEUE = 2; quiet.GRACE")


class Quiet(unittest.TestCase):
    def setUp(self):
        self.home = tempfile.mkdtemp(prefix="quiet-test-")
        self.dir = Path(self.home) / ".cache" / "ta-lib" / "quiet"
        self.env = dict(os.environ, HOME=self.home)
        self.env.pop("TA_QUIET", None)
        self.keep = []

    def tearDown(self):
        for p in self.keep:
            if p.poll() is None:
                p.kill()
            p.wait()
            for s in (p.stdout, p.stderr):
                if s:
                    s.close()
        shutil.rmtree(self.home, ignore_errors=True)

    def q(self, *args):
        return subprocess.run([sys.executable, "-c", RUN, *args], env=self.env,
                              capture_output=True, text=True, timeout=60)

    def spawn(self, *args):
        p = subprocess.Popen([sys.executable, "-c", RUN, *args], env=self.env,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.keep.append(p)
        return p

    def sleeper(self):
        p = subprocess.Popen(["sleep", "60"])
        self.keep.append(p)
        return p

    def until(self, cond, timeout=10):
        end = time.time() + timeout
        while time.time() < end:
            if cond():
                return
            time.sleep(0.05)
        self.fail("condition never became true")

    def holder(self):
        try:
            return (self.dir / "holder").read_text()
        except OSError:
            return ""

    def measuring(self):
        return "MEASURING" in self.holder()

    def fake_waiter(self, age=0):
        pid = self.sleeper().pid
        self.dir.mkdir(parents=True, exist_ok=True)
        f = self.dir / f"want.{pid}"
        f.write_text(f"WAITING to measure: other pid={pid} since=then for<=60s\n")
        if age:
            t = time.time() - age
            os.utime(f, (t, t))
        return f

    # --- the window -----------------------------------------------------------

    def test_free_at_first(self):
        r = self.q("status")
        self.assertEqual((r.returncode, r.stdout.strip()), (0, "free"))

    def test_measure_holds_the_window_while_it_runs(self):
        a = self.spawn("measure", "me", "30", "--", "sleep", "1")
        self.until(self.measuring)
        self.assertEqual(self.q("status").returncode, 1)
        self.assertEqual(a.wait(), 0)
        self.assertEqual(self.holder(), "")
        self.assertEqual(self.q("status").returncode, 0)

    def test_exit_code_passes_through(self):
        self.assertEqual(self.q("measure", "me", "30", "--", "sh", "-c", "exit 3").returncode, 3)

    def test_second_measure_is_busy_at_once(self):
        self.spawn("measure", "me", "30", "--", "sleep", "3")
        self.until(self.measuring)
        t = time.time()
        r = self.q("measure", "other", "30", "--", "true")
        self.assertEqual(r.returncode, 75)
        self.assertLess(time.time() - t, 2)
        self.assertIn("busy", r.stderr)

    def test_noisy_job_turns_measurers_away(self):
        self.spawn("noisy", "me", "--", "sleep", "3")
        self.until(lambda: list(self.dir.glob("noisy.*")))
        self.assertEqual(self.q("measure", "other", "30", "--", "true").returncode, 75)

    def test_noisy_runs_anyway_during_a_measurement(self):
        self.spawn("measure", "me", "30", "--", "sleep", "3")
        self.until(self.measuring)
        r = self.q("noisy", "other", "--", "echo", "ran")
        self.assertEqual((r.returncode, r.stdout.strip()), (0, "ran"))
        self.assertIn("running anyway", r.stderr)

    def test_cap_stops_a_long_measurement(self):
        r = self.q("measure", "me", "1", "--", "sleep", "10")
        self.assertEqual(r.returncode, 124)

    def test_killed_wrapper_frees_the_window_and_stops_its_command(self):
        marker = Path(self.home) / "survived"
        a = self.spawn("measure", "me", "60", "--", "sh", "-c", f"sleep 3; touch {marker}")
        self.until(self.measuring)
        a.send_signal(signal.SIGKILL)
        a.wait()
        self.assertEqual(self.q("measure", "other", "30", "--", "true").returncode, 0)
        time.sleep(3.5)
        self.assertFalse(marker.exists(), "the measured command outlived its wrapper")

    # --- the queue ------------------------------------------------------------

    def test_queued_measure_runs_when_the_window_frees(self):
        self.spawn("measure", "me", "30", "--", "sleep", "1.5")
        self.until(self.measuring)
        b = self.spawn("measure", "other", "30", "--queue=20", "--", "true")
        self.until(lambda: list(self.dir.glob("want.*")))
        r = self.q("status")
        self.assertEqual(r.returncode, 1)
        self.assertIn("queued: WAITING to measure: other", r.stdout)
        self.assertEqual(b.wait(), 0)
        self.assertEqual(list(self.dir.glob("want.*")), [])

    def test_queue_wait_is_bounded(self):
        self.spawn("measure", "me", "30", "--", "sleep", "5")
        self.until(self.measuring)
        t = time.time()
        r = self.q("measure", "other", "30", "--queue=1", "--", "true")
        self.assertEqual(r.returncode, 75)
        self.assertLess(time.time() - t, 4)
        self.assertIn("gave up", r.stderr)
        self.assertEqual(list(self.dir.glob("want.*")), [])

    def test_plain_measure_does_not_jump_the_queue(self):
        self.fake_waiter()
        r = self.q("measure", "me", "30", "--", "true")
        self.assertEqual(r.returncode, 75)
        self.assertIn("queued", r.stderr)
        s = self.q("status")
        self.assertEqual(s.returncode, 1)
        self.assertTrue(s.stdout.startswith("free (queued:"), s.stdout)

    def test_first_come_is_served_first(self):
        older = self.fake_waiter(age=10)
        r = self.q("measure", "me", "30", "--queue=1", "--", "true")
        self.assertEqual(r.returncode, 75, "a later waiter took the window ahead of an earlier one")
        older.unlink()
        self.assertEqual(self.q("measure", "me", "30", "--queue=1", "--", "true").returncode, 0)

    def test_dead_waiter_is_dropped(self):
        p = subprocess.Popen(["true"])
        p.wait()
        self.dir.mkdir(parents=True, exist_ok=True)
        stale = self.dir / f"want.{p.pid}"
        stale.write_text("WAITING to measure: gone\n")
        self.assertEqual(self.q("measure", "me", "30", "--", "true").returncode, 0)
        self.assertFalse(stale.exists())

    def test_deferred_noisy_waits_for_a_measurement(self):
        self.spawn("measure", "me", "30", "--", "sleep", "1.5")
        self.until(self.measuring)
        t = time.time()
        r = self.q("noisy", "other", "--defer=20", "--", "true")
        self.assertEqual(r.returncode, 0)
        self.assertGreater(time.time() - t, 0.8)
        self.assertNotIn("running anyway", r.stderr)

    def test_deferred_noisy_waits_for_the_queue_but_not_forever(self):
        self.fake_waiter()
        t = time.time()
        r = self.q("noisy", "other", "--defer=1", "--", "true")
        self.assertEqual(r.returncode, 0)
        self.assertGreater(time.time() - t, 0.8)
        self.assertIn("queued measurement", r.stderr)

    def test_plain_noisy_does_not_wait(self):
        self.fake_waiter()
        t = time.time()
        self.assertEqual(self.q("noisy", "other", "--", "true").returncode, 0)
        self.assertLess(time.time() - t, 2)

    # --- stale state and nesting ----------------------------------------------

    def stale_holder(self):
        """A label with no lock behind it, naming a live pid as a reused one would."""
        self.dir.mkdir(parents=True, exist_ok=True)
        (self.dir / "holder").write_text(f"MEASURING by gone pid={self.sleeper().pid} since=then\n")

    def test_a_stale_holder_does_not_hold_up_a_deferred_noisy(self):
        self.stale_holder()
        self.spawn("noisy", "build", "--", "sleep", "5")
        self.until(lambda: list(self.dir.glob("noisy.*")))
        t = time.time()
        self.assertEqual(self.q("noisy", "other", "--defer=20", "--", "true").returncode, 0)
        self.assertLess(time.time() - t, 2)
        s = self.q("status")
        self.assertNotIn("MEASURING", s.stdout)
        self.assertIn("NOISY by build", s.stdout)

    def nested(self, *args):
        return [sys.executable, "-c", RUN, *args]

    def test_nested_noisy_skips_its_defer(self):
        self.fake_waiter()
        t = time.time()
        r = self.q("noisy", "outer", "--", *self.nested("noisy", "inner", "--defer=20", "--", "true"))
        self.assertEqual(r.returncode, 0)
        self.assertNotIn("deferring", r.stderr)
        self.assertLess(time.time() - t, 10)

    def test_nested_noisy_that_outlives_its_parent_stays_registered(self):
        self.q("noisy", "outer", "--", "sh", "-c",
               shlex.join(self.nested("noisy", "bg", "--", "sleep", "4")) + " >/dev/null 2>&1 & sleep 1")
        self.assertEqual(self.q("measure", "later", "30", "--", "true").returncode, 75)

    def test_nested_measure_keeps_its_own_cap(self):
        r = self.q("measure", "outer", "30", "--", *self.nested("measure", "inner", "1", "--", "sleep", "10"))
        self.assertEqual(r.returncode, 124)

    def test_nested_measure_stops_its_command_on_sigterm(self):
        pid_file, mark = Path(self.home) / "inner_pid", Path(self.home) / "survived"
        self.spawn("measure", "outer", "60", "--", *self.nested(
            "measure", "inner", "60", "--", "sh", "-c",
            f"echo $PPID > {pid_file}; sh -c 'sleep 2; touch {mark}'; true"))
        self.until(lambda: pid_file.exists() and pid_file.read_text().strip())
        os.kill(int(pid_file.read_text()), signal.SIGTERM)
        time.sleep(3)
        self.assertFalse(mark.exists(), "a SIGTERMed nested measure left its command running")

    def test_nested_measure_is_stopped_before_its_parent_gives_up(self):
        mark = Path(self.home) / "survived"
        r = self.q("measure", "outer", "1", "--", *self.nested(
            "measure", "inner", "60", "--", "sh", "-c",
            f"trap '' INT TERM; sh -c 'sleep 3; touch {mark}'; true"))
        self.assertEqual(r.returncode, 124)
        time.sleep(3.5)
        self.assertFalse(mark.exists(), "the nested command outlived the outer cap")

    def test_measure_inside_a_noisy_job_is_busy_at_once(self):
        t = time.time()
        r = self.q("noisy", "outer", "--", *self.nested("measure", "inner", "30", "--queue=20", "--", "true"))
        self.assertEqual(r.returncode, 75)
        self.assertLess(time.time() - t, 10)
        self.assertEqual(list(self.dir.glob("want.*")), [])

    def test_nested_measure_runs_inside_its_parent(self):
        r = self.q("measure", "outer", "30", "--",
                   *self.nested("measure", "inner", "30", "--", "sh", "-c", "exit 4"))
        self.assertEqual(r.returncode, 4)

    def test_a_marker_from_a_dead_wrapper_is_ignored(self):
        p = subprocess.Popen(["true"])
        p.wait()
        self.fake_waiter()
        env = dict(self.env, TA_QUIET=f"noisy:{p.pid}")
        t = time.time()
        r = subprocess.run([sys.executable, "-c", RUN, "noisy", "me", "--defer=1", "--", "true"],
                           env=env, capture_output=True, text=True, timeout=60)
        self.assertEqual(r.returncode, 0)
        self.assertGreater(time.time() - t, 0.8, "a dead parent's marker skipped the defer")

    def test_noisy_started_during_a_measurement_registers_once_it_ends(self):
        self.spawn("measure", "me", "30", "--", "sleep", "3")
        self.until(self.measuring)
        late = self.spawn("noisy", "late", "--", "sleep", "8")
        self.assertIn("running anyway", late.stderr.readline())
        self.until(lambda: list(self.dir.glob("noisy.*")))
        self.assertEqual(self.q("measure", "other", "30", "--", "true").returncode, 75)

    def test_equal_arrival_goes_to_the_lower_pid(self):
        a, b = sorted((self.sleeper().pid, self.sleeper().pid))
        self.dir.mkdir(parents=True, exist_ok=True)
        for pid in (a, b):
            f = self.dir / f"want.{pid}"
            f.write_text("WAITING\n")
            os.utime(f, (1_000_000, 1_000_000))
        probe = ("import sys; sys.path.insert(0, %r); import quiet; "
                 "print(quiet.older_waiter(%d), quiet.older_waiter(%d))" % (HERE, a, b))
        r = subprocess.run([sys.executable, "-c", probe], env=self.env, capture_output=True,
                           text=True, timeout=60)
        self.assertEqual(r.stdout.split(), ["False", "True"])

    # --- exit codes -------------------------------------------------------------

    def test_missing_command_is_127(self):
        self.assertEqual(self.q("measure", "me", "30", "--", "no-such-command-437").returncode, 127)
        self.assertEqual(self.q("noisy", "me", "--", "no-such-command-437").returncode, 127)

    def test_command_killed_by_a_signal_is_128_plus_n(self):
        self.assertEqual(self.q("measure", "me", "30", "--", "sh", "-c", "kill -TERM $$").returncode,
                         128 + signal.SIGTERM)

    def test_cap_stops_with_int_first_then_term(self):
        int_mark, term_mark = Path(self.home) / "int", Path(self.home) / "term"
        r = self.q("measure", "me", "1", "--", "sh", "-c", f"trap 'touch {int_mark}; exit 0' INT; sleep 30 & wait")
        self.assertEqual(r.returncode, 124)
        self.assertTrue(int_mark.exists(), "the cap did not send SIGINT first")
        r = self.q("measure", "me", "1", "--", "sh", "-c",
                   f"trap '' INT; trap 'touch {term_mark}; exit 0' TERM; sleep 30 & wait")
        self.assertEqual(r.returncode, 124)
        self.assertTrue(term_mark.exists(), "the cap did not follow SIGINT with SIGTERM")

    def test_cap_escalates_to_kill_when_int_and_term_are_ignored(self):
        t = time.time()
        r = self.q("measure", "me", "1", "--", "sh", "-c", "trap '' INT TERM; sleep 30")
        self.assertEqual(r.returncode, 124)
        self.assertLess(time.time() - t, 10)

    def capped(self, *args):
        return subprocess.run([sys.executable, "-c", CAPPED, *args], env=self.env,
                              capture_output=True, text=True, timeout=60)

    def test_waits_are_capped_and_announced(self):
        self.fake_waiter()
        t = time.time()
        r = self.capped("noisy", "other", "--defer=600", "--", "true")
        self.assertEqual(r.returncode, 0)
        self.assertLess(time.time() - t, 5, "--defer was not capped")
        self.assertIn("deferring up to 2s", r.stderr)
        t = time.time()
        r = self.capped("measure", "me", "30", "--queue=600", "--", "true")
        self.assertEqual(r.returncode, 75)
        self.assertLess(time.time() - t, 5, "--queue was not capped")
        self.assertIn("queued, waiting up to 2s", r.stderr)

    def test_malformed_waits_are_usage_errors(self):
        self.assertEqual(self.q("measure", "me", "30", "--queue=x", "--", "true").returncode, 2)
        self.assertEqual(self.q("noisy", "me", "--defer=", "--", "true").returncode, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
