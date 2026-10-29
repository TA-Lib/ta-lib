import os
import subprocess
import sys

# A C program built against an installed package, statically and through the
# shared library, which must link the version under test.

CONSUMER_C = r'''
#include <ta-lib/ta_libc.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv )
{
   double in[10] = {1,2,3,4,5,6,7,8,9,10}, out[10];
   int begIdx, nbElement;
   const char *version;

   if( TA_Initialize() != TA_SUCCESS ) { printf("FAIL: TA_Initialize\n"); return 1; }
   if( TA_SMA(0,9,in,3,&begIdx,&nbElement,out) != TA_SUCCESS ) { printf("FAIL: TA_SMA\n"); return 1; }
   if( begIdx != 2 || nbElement != 8 || out[0] != 2.0 )
   {
      printf("FAIL: SMA begIdx=%d nbElement=%d out[0]=%f\n", begIdx, nbElement, out[0]);
      return 1;
   }

   version = TA_GetVersionString();
   if( argc > 1 && strncmp(version, argv[1], strlen(argv[1])) != 0 )
   {
      printf("FAIL: linked TA-Lib %s, expected %s*\n", version, argv[1]);
      return 1;
   }
   printf("consumer ok: %s\n", version);

   TA_Shutdown();
   return 0;
}
'''


def run_or_die(cmd, step: str, cwd: str = None, env: dict = None):
    print(f"  [{step}] {' '.join(str(a) for a in cmd)}")
    result = subprocess.run(cmd, cwd=cwd, env=env)
    if result.returncode != 0:
        print(f"FAIL: {step} exited with {result.returncode}")
        sys.exit(1)


def run_consumer(exe: str, version: str, step: str, env: dict = None):
    print(f"  [{step}] {exe} {version}")
    result = subprocess.run([exe, version], capture_output=True, text=True, env=env)
    print(result.stdout.strip() or result.stderr.strip())
    if result.returncode != 0:
        print(f"FAIL: {step} exited with {result.returncode}")
        sys.exit(1)


def write_consumer(path: str):
    with open(path, "w", encoding="ascii") as f:
        f.write(CONSUMER_C)


def consumer_linux(temp_dir: str, version: str, tag: str,
                   include_dir: str, lib_dir: str, search_paths: bool):
    """search_paths=False compiles the way a user does after `make install`:
    no -I/-L, so the compiler's default paths must find the install."""
    consumer_c = os.path.join(temp_dir, f"{tag}_consumer.c")
    write_consumer(consumer_c)
    flags = ["-I", include_dir] if search_paths else []

    shared_exe = os.path.join(temp_dir, f"{tag}_consumer_shared")
    shared_link = (["-L", lib_dir, f"-Wl,-rpath,{lib_dir}"] if search_paths else []) + ["-lta-lib"]
    run_or_die(["gcc", consumer_c] + flags + shared_link + ["-o", shared_exe],
               f"compile consumer ({tag}, shared)")
    run_consumer(shared_exe, version, f"run consumer ({tag}, shared)")

    static_exe = os.path.join(temp_dir, f"{tag}_consumer_static")
    run_or_die(["gcc", consumer_c] + flags +
               [os.path.join(lib_dir, "libta-lib.a"), "-lm", "-o", static_exe],
               f"compile consumer ({tag}, static)")
    run_consumer(static_exe, version, f"run consumer ({tag}, static)")


def consumer_windows(root_dir: str, temp_dir: str, version: str, tag: str,
                     include_dir: str, lib_dir: str, bin_dir: str):
    # Imported here: utilities.windows is Windows-only.
    from utilities.windows import call_vcvarsall
    call_vcvarsall(root_dir, ["amd64"])
    # /MD: the C runtime CMake builds both libraries with by default.

    work_dir = os.path.join(temp_dir, f"{tag}_consumer")
    os.makedirs(work_dir, exist_ok=True)
    consumer_c = os.path.join(work_dir, "consumer.c")
    write_consumer(consumer_c)

    static_exe = os.path.join(work_dir, "consumer_static.exe")
    run_or_die(["cl", "/nologo", "/MD", consumer_c, f"/I{include_dir}", f"/Fe{static_exe}",
                "/link", os.path.join(lib_dir, "ta-lib-static.lib")],
               f"compile consumer ({tag}, static)", cwd=work_dir)
    run_consumer(static_exe, version, f"run consumer ({tag}, static)")

    # Through the import library; the DLL is found on PATH, as an installed
    # application would find it.
    shared_exe = os.path.join(work_dir, "consumer_shared.exe")
    run_or_die(["cl", "/nologo", "/MD", consumer_c, f"/I{include_dir}", f"/Fe{shared_exe}",
                "/link", os.path.join(lib_dir, "ta-lib.lib")],
               f"compile consumer ({tag}, DLL)", cwd=work_dir)
    env = os.environ.copy()
    env["PATH"] = bin_dir + os.pathsep + env.get("PATH", "")
    run_consumer(shared_exe, version, f"run consumer ({tag}, DLL)", env=env)
