#!/usr/bin/env python3
"""Tests for quiet.py, each against its own throwaway HOME.

    python3 scripts/test_quiet.py
"""

import os
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path

HERE = os.path.dirname(os.path.abspath(__file__))
RUN = ("import sys; sys.path.insert(0, %r); import quiet; quiet.POLL = 0.1; "
       "sys.exit(quiet.main(sys.argv[1:]))" % HERE)


class Quiet(unittest.TestCase):
    def setUp(self):
        self.home = tempfile.mkdtemp(prefix="quiet-test-")
        self.dir = Path(self.home) / ".cache" / "ta-lib" / "quiet"
        self.env = dict(os.environ, HOME=self.home)
        self.keep = []

    def tearDown(self):
        for p in self.keep:
            if p.poll() is None:
                p.kill()
            p.wait()
            for s in (p.stdout, p.stderr):
                if s:
                    s.close()

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

    def test_malformed_waits_are_usage_errors(self):
        self.assertEqual(self.q("measure", "me", "30", "--queue=x", "--", "true").returncode, 2)
        self.assertEqual(self.q("noisy", "me", "--defer=", "--", "true").returncode, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
