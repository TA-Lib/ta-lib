#!/usr/bin/env python3
"""The npm package in a real headless browser, through its `default` export condition.

  1. Pack the package as scripts/ts_package_check.py does, install the tarball
     into a throwaway project, and bundle scripts/ts_browser_check/ with the
     tooling root's esbuild for the browser platform: page.mjs (the default
     entry, ta-lib/js and the engine's @internal hooks, code splitting on) and
     jsonly.mjs (ta-lib/js alone).
  2. From the bundles, without a browser: the default entry resolved to
     dist/engine/index.js, not the Node entry; the engine's bytes sit in one
     chunk that only a dynamic import loads; the ta-lib/js bundle holds none of
     them.
  3. Serve the page from a loopback server on an ephemeral port, open it in a
     headless Chrome or Chromium, and read the verdict the page posts back:
     under /plain/ ready() resolves to the WASM engine, compiled
     asynchronously, strict-mode calls are bit-identical to ta-lib/js and each
     entered the module, and a call before ready() did not, nor was the bytes
     chunk fetched before ready(); under /csp/, served with a
     Content-Security-Policy that forbids WebAssembly, ready() resolves to the
     TypeScript engine with its reason, every call answers with the bits of
     ta-lib/js, and nothing throws.

The browser is $TA_BROWSER, else the first Chrome or Chromium found; none is a
failure. The browser runs in its own process group (on Windows, its own tree),
which is killed by the PID started, and the check fails if any of it survives.

Run:  python3 scripts/ts_browser_check.py [--allow-dirty] [--work DIR [--reuse]]

--work keeps the throwaway project in DIR; --reuse bundles and runs the project
already installed there, as edited, without packing or installing.
"""

import argparse
import http.server
import json
import os
import platform
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ts_package_check as pkg

PAGE_DIR = os.path.join(pkg.ROOT, "scripts", "ts_browser_check")
ESBUILD = os.path.join(pkg.TS_ROOT, "node_modules", ".bin", "esbuild.cmd" if os.name == "nt" else "esbuild")
CSP = "script-src 'self'"
TIMEOUT = 90
PLAIN = ["before-ready-call-stayed-in-typescript", "ready-resolves-wasm", "compiled-asynchronously",
         "strict-bits-ema", "strict-bits-t3", "strict-bits-macd", "strict-bits-sma",
         "strict-bits-cdlengulfing", "module-counter-advanced", "plain-nothing-threw"]
CSP_RUN = ["csp-ready-resolves-js-with-reason", "csp-calls-answer-from-typescript",
           "csp-compiled-asynchronously", "csp-nothing-threw"]


def find_browser():
    named = os.environ.get("TA_BROWSER")
    if named:
        return named if os.path.isfile(named) and os.access(named, os.X_OK) else None
    for name in ("google-chrome", "google-chrome-stable", "chromium", "chromium-browser", "chrome"):
        found = shutil.which(name)
        if found:
            return found
    candidates = {
        "Darwin": ["/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
                   "/Applications/Chromium.app/Contents/MacOS/Chromium"],
        "Windows": [os.path.join(os.environ.get(v, ""), "Google", "Chrome", "Application", "chrome.exe")
                    for v in ("PROGRAMFILES", "PROGRAMFILES(X86)", "LOCALAPPDATA") if os.environ.get(v)],
    }.get(platform.system(), [])
    return next((c for c in candidates if os.path.isfile(c)), None)


def install(work, allow_dirty):
    out_dir = tempfile.mkdtemp(prefix="talib-ts-pack-")
    try:
        failures = pkg.pack(out_dir, allow_dirty)
        if failures:
            return failures
        tarball = os.path.join(out_dir, [n for n in os.listdir(out_dir) if n.endswith(".tgz")][0])
        with open(os.path.join(work, "package.json"), "w") as f:
            f.write('{"name":"ta-lib-browser-check","private":true,"type":"module"}\n')
        if pkg.run([pkg.NPM, "install", "--no-audit", "--no-fund", "--ignore-scripts", tarball],
                   cwd=work).returncode:
            return ["the browser project could not install %s" % os.path.basename(tarball)]
        return []
    finally:
        shutil.rmtree(out_dir, ignore_errors=True)


def bundle(work):
    for entry in os.listdir(PAGE_DIR):
        shutil.copy(os.path.join(PAGE_DIR, entry), work)
    site = os.path.join(work, "site")
    shutil.rmtree(site, ignore_errors=True)
    if pkg.run([ESBUILD, "page.mjs", "--bundle", "--splitting", "--format=esm", "--platform=browser",
                "--outdir=site", "--entry-names=[name]", "--metafile=page.meta.json"], cwd=work).returncode:
        return ["esbuild could not bundle page.mjs for the browser; see its output above"]
    if pkg.run([ESBUILD, "jsonly.mjs", "--bundle", "--format=esm", "--platform=browser",
                "--outdir=jsonly", "--metafile=jsonly.meta.json"], cwd=work).returncode:
        return ["esbuild could not bundle jsonly.mjs for the browser; see its output above"]
    shutil.copy(os.path.join(work, "index.html"), site)
    return []


def outputs(directory):
    found = {}
    for name in sorted(os.listdir(directory)):
        with open(os.path.join(directory, name), encoding="utf-8") as f:
            found[name] = f.read()
    return found


def static_checks(work, report):
    """The bundle-content assertions; returns the name of the chunk holding the bytes."""
    with open(os.path.join(work, "node_modules", "ta-lib", "dist", "runtime", "wasmBytes.js"),
              encoding="utf-8") as f:
        base64 = f.read().split('"')[1]
    probe = base64[len(base64) // 2:len(base64) // 2 + 64]
    with open(os.path.join(work, "page.meta.json")) as f:
        page_inputs = set(json.load(f)["inputs"])
    with open(os.path.join(work, "jsonly.meta.json")) as f:
        jsonly_inputs = set(json.load(f)["inputs"])
    default_entry = "node_modules/ta-lib/dist/engine/index.js"
    node_entry = "node_modules/ta-lib/dist/engine/node.js"
    report("bundle-resolves-default-condition",
           default_entry in page_inputs and node_entry not in page_inputs,
           "page bundle reads %s, %s" % (default_entry, "and also the Node entry" if node_entry in page_inputs
                                         else "not the Node entry"))

    site = outputs(os.path.join(work, "site"))
    holders = [name for name, text in site.items() if probe in text]
    chunk = holders[0] if len(holders) == 1 else None
    static, dynamic = [], []
    if chunk:
        for name, text in site.items():
            for m in re.finditer(r"(import\s*\(\s*)?[\"']\./%s[\"']" % re.escape(chunk), text):
                (dynamic if m.group(1) else static).append(name)
    report("bytes-in-separately-loaded-chunk",
           chunk is not None and chunk != "page.js" and not static and dynamic,
           "the bytes sit in %s; imported statically by %s, dynamically by %s"
           % (holders or "no output", static or "none", dynamic or "none"))

    jsonly = outputs(os.path.join(work, "jsonly"))
    leaked = [name for name, text in jsonly.items() if probe in text]
    js_sma = "node_modules/ta-lib/dist/func/sma.js"
    report("js-only-bundle-has-no-bytes",
           not leaked and js_sma in jsonly_inputs and holders,
           "ta-lib/js bundle (%s) %s the bytes; it reads %s: %s"
           % (", ".join(jsonly), "carries" if leaked else "does not carry", js_sma, js_sma in jsonly_inputs))
    return chunk


class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, site):
        self.site = site
        self.lock = threading.Lock()
        self.log = []
        self.verdicts = {}
        self.arrived = threading.Condition(self.lock)
        super().__init__(("127.0.0.1", 0), Handler)


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {".js": "text/javascript", ".html": "text/html", "": "application/octet-stream"}

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=args[2].site, **kwargs)

    def log_message(self, *args):
        pass

    def mode(self):
        return self.path.split("/")[1]

    def end_headers(self):
        if self.mode() == "csp":
            self.send_header("Content-Security-Policy", CSP)
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def translate_path(self, path):
        parts = path.split("/", 2)
        return super().translate_path("/" + (parts[2] if len(parts) > 2 else ""))

    def do_GET(self):
        with self.server.lock:
            self.server.log.append(("GET", self.path))
        super().do_GET()

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("content-length") or 0))
        parts = self.path.split("/")
        with self.server.lock:
            self.server.log.append(("POST", self.path))
            if parts[1] == "verdict":
                try:
                    self.server.verdicts[parts[2]] = json.loads(body)
                except ValueError:
                    self.server.verdicts[parts[2]] = {"assertions": [], "unreadable": body[:200].decode(errors="replace")}
                self.server.arrived.notify_all()
        self.send_response(204)
        self.end_headers()


def launch(browser, url, profile, log):
    # Relative paths: Chrome binds its singleton socket under $TMPDIR, and a
    # Unix socket path longer than about 100 bytes makes it abort at start.
    parent, name = os.path.split(profile)
    os.mkdir(os.path.join(profile, "tmp"))
    env = dict(os.environ, TMPDIR=os.path.join(name, "tmp"))
    cmd = [browser, "--headless=new", "--no-sandbox", "--no-first-run", "--no-default-browser-check",
           "--disable-extensions", "--disable-background-networking", "--disable-component-update",
           "--disable-sync", "--disable-gpu", "--user-data-dir=" + name, url]
    if os.name == "nt":
        return subprocess.Popen(cmd, cwd=parent, env=env, stdout=log, stderr=subprocess.STDOUT,
                                creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
    return subprocess.Popen(cmd, cwd=parent, env=env, stdout=log, stderr=subprocess.STDOUT,
                            start_new_session=True)


def group_alive(pid):
    try:
        os.killpg(pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def stop(proc, profile):
    """Kills everything the browser started; returns what survived, if anything."""
    if os.name == "nt":
        subprocess.run(["taskkill", "/T", "/F", "/PID", str(proc.pid)], capture_output=True, check=False)
        proc.wait(timeout=30)
        return None
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(proc.pid, sig)
        except ProcessLookupError:
            break
        deadline = time.monotonic() + 10
        while group_alive(proc.pid) and time.monotonic() < deadline:
            if proc.poll() is None:
                try:
                    proc.wait(timeout=0.2)
                except subprocess.TimeoutExpired:
                    pass
            else:
                time.sleep(0.2)
        if not group_alive(proc.pid):
            break
    proc.wait(timeout=30)
    if group_alive(proc.pid):
        return "process group %d" % proc.pid
    # A helper that left the group still names the profile on its command line.
    if os.path.isdir("/proc"):
        for entry in os.listdir("/proc"):
            if entry.isdigit():
                try:
                    with open("/proc/%s/cmdline" % entry, "rb") as f:
                        if os.path.basename(profile).encode() in f.read():
                            return "pid %s outside the process group" % entry
                except OSError:
                    pass
    return None


def browse(browser, server, mode, work):
    """Opens /<mode>/index.html; returns (verdict or None, failure or None)."""
    profile = tempfile.mkdtemp(prefix="talib-browser-profile-")
    log_path = os.path.join(work, "browser-%s.log" % mode)
    url = "http://127.0.0.1:%d/%s/index.html" % (server.server_address[1], mode)
    with open(log_path, "w") as log:
        proc = launch(browser, url, profile, log)
        try:
            with server.lock:
                server.arrived.wait_for(lambda: mode in server.verdicts or proc.poll() is not None,
                                        timeout=TIMEOUT)
                verdict = server.verdicts.get(mode)
        finally:
            survivor = stop(proc, profile)
            shutil.rmtree(profile, ignore_errors=True)
    if survivor:
        return verdict, "the browser left %s running after it was killed" % survivor
    if verdict is None:
        with open(log_path, errors="replace") as f:
            tail = f.read()[-2000:]
        return None, "no verdict from %s within %d s; the browser's output ends:\n%s" % (url, TIMEOUT, tail)
    return verdict, None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--allow-dirty", action="store_true",
                        help="pack a library directory with local edits or stray files")
    parser.add_argument("--work", help="keep the throwaway project in this directory")
    parser.add_argument("--reuse", action="store_true",
                        help="with --work: bundle and run the project already installed there")
    args = parser.parse_args()
    if args.reuse and not args.work:
        parser.error("--reuse needs --work")

    failures = []
    results = []

    def report(name, ok, detail):
        results.append(name)
        print("%s %s: %s" % ("PASS" if ok else "FAIL", name, detail), flush=True)
        if not ok:
            failures.append("%s: %s" % (name, detail))

    browser = find_browser()
    if browser is None:
        where = ("TA_BROWSER=%s is not an executable file" % os.environ["TA_BROWSER"]
                 if os.environ.get("TA_BROWSER") else
                 "no Chrome or Chromium found on PATH or in the usual install locations")
        print("\n=== BROWSER CHECK FAILED ===\n  %s; install one or point TA_BROWSER at it" % where)
        return 1
    version = subprocess.run([browser, "--version"], capture_output=True, text=True, check=False).stdout.strip()
    print("--- browser: %s (%s)" % (browser, version or "version not printed"), flush=True)

    work = os.path.abspath(args.work) if args.work else tempfile.mkdtemp(prefix="talib-ts-browser-")
    try:
        if args.reuse:
            if not os.path.isdir(os.path.join(work, "node_modules", "ta-lib")):
                failures.append("--reuse: %s holds no installed ta-lib" % work)
        else:
            if os.path.isdir(work) and os.listdir(work):
                failures.append("%s is not empty" % work)
            else:
                os.makedirs(work, exist_ok=True)
                failures += install(work, args.allow_dirty)
        if not failures and not os.path.isfile(ESBUILD):
            if not pkg.ensure_tooling() or not os.path.isfile(ESBUILD):
                failures.append("no esbuild at %s after npm ci in %s" % (ESBUILD, pkg.TS_REL))
        if not failures:
            failures += bundle(work)
        if not failures:
            chunk = static_checks(work, report)
            server = Server(os.path.join(work, "site"))
            serving = threading.Thread(target=server.serve_forever, daemon=True)
            serving.start()
            try:
                for mode, expected in (("plain", PLAIN), ("csp", CSP_RUN)):
                    verdict, failure = browse(browser, server, mode, work)
                    if failure:
                        failures.append("%s: %s" % (mode, failure))
                    if verdict is None:
                        continue
                    print("--- %s run, %s" % (mode, verdict.get("userAgent")), flush=True)
                    seen = []
                    for a in verdict.get("assertions", []):
                        seen.append(a.get("name"))
                        report(a.get("name"), a.get("ok") is True, a.get("detail"))
                    missing = [name for name in expected if name not in seen]
                    if missing:
                        failures.append("%s: the page reported no %s" % (mode, ", ".join(missing)))
                    if mode == "plain":
                        with server.lock:
                            log = list(server.log)
                        marker = ("POST", "/mark/plain/before-ready")
                        fetched = [i for i, (verb, path) in enumerate(log)
                                   if verb == "GET" and chunk and path == "/plain/" + chunk]
                        at = log.index(marker) if marker in log else None
                        report("bytes-fetched-only-after-ready",
                               at is not None and fetched and min(fetched) > at,
                               "the bytes chunk %s was fetched %d time(s), %s"
                               % (chunk, len(fetched), "first after ready() was called" if at is not None
                                  and fetched and min(fetched) > at else "before ready(), or never"))
            finally:
                server.shutdown()
                server.server_close()
    finally:
        if not args.work:
            shutil.rmtree(work, ignore_errors=True)

    if failures:
        print("\n=== BROWSER CHECK FAILED ===")
        for failure in failures:
            print("  " + failure)
        return 1
    print("\n=== BROWSER CHECK PASSED (%d assertions) ===" % len(results))
    return 0


if __name__ == "__main__":
    sys.exit(main())
