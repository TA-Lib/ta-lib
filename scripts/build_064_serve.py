#!/usr/bin/env python3
"""Build bin/ta_064_serve for ta_regtest --fuzz-064.

Links the frozen v0.6.4 lib (../ta-lib-064 worktree @ tag v0.6.4) behind the
current JSON-RPC transport, shadow-patched (no committed file changes) for
seed-input generation + hash output. Needs the v0.6.4 tag (CI: fetch-depth 0).
See src/tools/ta_regtest/CLAUDE.md.
"""
import os
import re
import subprocess
import sys

REF_TAG = "v0.6.4"


def find_repo_root():
    here = os.path.dirname(os.path.abspath(__file__))
    try:
        out = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True, text=True, check=True, cwd=here,
        )
        return out.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        # Fallback: walk up from this file looking for the repo markers.
        d = os.path.dirname(os.path.abspath(__file__))
        while d != os.path.dirname(d):
            if os.path.isdir(os.path.join(d, "ta_codegen")) and \
               os.path.isfile(os.path.join(d, "CMakeLists.txt")):
                return d
            d = os.path.dirname(d)
        sys.exit("build_064_serve: cannot locate repo root")


def die(msg):
    sys.exit("build_064_serve: " + msg)


def tag_available(root):
    return subprocess.run(
        ["git", "rev-parse", "--verify", "--quiet", f"refs/tags/{REF_TAG}"],
        cwd=root, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    ).returncode == 0


def ensure_worktree_and_lib(root):
    """Return the path to the frozen v0.6.4 libta-lib.a, building it once."""
    ref_root = os.path.join(os.path.dirname(root), "ta-lib-064")
    ref_build = os.path.join(ref_root, "cmake-build")
    lib_a = os.path.join(ref_build, "libta-lib.a")

    if not tag_available(root):
        die(f"tag '{REF_TAG}' unavailable. Fetch tags (git fetch --tags) or use\n"
            f"  actions/checkout with fetch-depth: 0. The 0.6.4 oracle cannot be\n"
            f"  built without the released tag.")

    if not os.path.isdir(ref_root):
        print(f"  Creating v0.6.4 worktree {ref_root}")
        subprocess.run(["git", "worktree", "add", ref_root, REF_TAG],
                       check=True, cwd=root)

    if not os.path.exists(lib_a):
        print("  Building frozen v0.6.4 libta-lib.a (one time)...")
        os.makedirs(ref_build, exist_ok=True)
        if not os.path.exists(os.path.join(ref_build, "CMakeCache.txt")):
            subprocess.run(["cmake", ref_root, "-DCMAKE_BUILD_TYPE=Release"],
                           check=True, cwd=ref_build)
        subprocess.run(["cmake", "--build", ".", "--target", "ta-lib-static",
                        "-j", str(os.cpu_count() or 4)], check=True, cwd=ref_build)
    if not os.path.exists(lib_a):
        die("frozen v0.6.4 libta-lib.a was not produced")
    return lib_a


def include_dirs(root, bin_dir):
    c_out = os.path.join(root, "ta_codegen", "output", "c")
    c_tools = os.path.join(c_out, "tools")
    return [
        bin_dir,                                        # patched ta_abstract_serve.c
        os.path.join(root, "src", "tools", "ta_regtest"),  # fuzz_data.h (shared)
        c_tools,
        os.path.join(root, "include"),
        os.path.join(c_out, "ta_common"),
        os.path.join(c_out, "ta_abstract"),
        os.path.join(c_out, "ta_abstract", "frames"),
        os.path.join(root, "ta_codegen", "generator", "templates", "c"),
        os.path.join(root, "src", "ta_common"),
        os.path.join(root, "src", "ta_func"),
        os.path.join(root, "src"),
        os.path.join(root, "src", "ta_abstract"),
        os.path.join(root, "src", "ta_abstract", "frames"),
    ]


INPUT_HOOK = r'''
   /* [fuzz] seed-based input generation (ta_064_serve differential harness) */
   if( json_find_int(json, "gen_present") ) {
      int fz_shape = json_find_int(json, "gen_shape");
      int fz_seed  = json_find_int(json, "gen_seed");
      int fz_n     = json_find_int(json, "gen_n");
      fuzz_gen(fz_shape, fz_seed, fz_n,
               g_inBuf0, g_inBuf1, g_inBuf2, g_inBuf3, g_inBuf4, g_inBuf5);
      /* Real inputs read buf0,buf1..; match driver (real0=close, real1=volume),
       * incl. mixed real+price funcs (OBV). */
      { unsigned int fz_i; int fz_k, fz_realIdx = 0;
        for( fz_i = 0; fz_i < fi->nbInput; fz_i++ ) {
           const TA_InputParameterInfo *fz_ii;
           TA_GetInputParameterInfo(handle, fz_i, &fz_ii);
           if( fz_ii->type != TA_Input_Real ) continue;
           double *fz_dst = (fz_realIdx == 0) ? g_inBuf0 : (fz_realIdx == 1) ? g_inBuf1
                          : (fz_realIdx == 2) ? g_inBuf2 : g_inBuf3;
           double *fz_src = (fz_realIdx == 1) ? g_inBuf4 : g_inBuf3;
           if( fz_dst != fz_src )
              for( fz_k = 0; fz_k < fz_n; fz_k++ ) fz_dst[fz_k] = fz_src[fz_k];
           fz_realIdx++;
        }
      }
   }
'''

OUTPUT_HOOK = r'''
   /* [fuzz] hash mode: 64-bit digest of raw output unless full_output. */
   if( json_find_int(json, "gen_present") && !json_find_int(json, "full_output") ) {
      unsigned long long fz_h = fuzz_hash_init();
      if( rc == TA_SUCCESS && outNBElement > 0 ) {
         int fz_rIdx = 0, fz_iIdx = 0; unsigned int fz_o;
         for( fz_o = 0; fz_o < fi->nbOutput && fz_o < 3; fz_o++ ) {
            if( outputIsInteger[fz_o] ) {
               int *fz_b = (fz_iIdx == 0) ? g_outIntBuf0 : g_outIntBuf1;
               fz_h = fuzz_hash_bytes(fz_h, fz_b, (unsigned long)outNBElement * sizeof(int));
               fz_iIdx++;
            } else {
               double *fz_b = (fz_rIdx == 0) ? g_outBuf0 : (fz_rIdx == 1) ? g_outBuf1 : g_outBuf2;
               fz_h = fuzz_hash_bytes(fz_h, fz_b, (unsigned long)outNBElement * sizeof(double));
               fz_rIdx++;
            }
         }
      }
      fz_h = fuzz_hash_fin(fz_h);
      pos += snprintf(resp + pos, resp_size - pos, ",\"out_hash\":\"%016llx\"", fz_h);
      snprintf(resp + pos, resp_size - pos, "}");
      return;
   }
'''


def build(root, bin_dir, lib_a):
    c_out = os.path.join(root, "ta_codegen", "output", "c")
    serve_src = os.path.join(c_out, "tools", "ta_codegen_serve.c")
    abstract_src = os.path.join(root, "ta_codegen", "generator", "templates", "c", "ta_abstract_serve.c")

    # 1. Main transport: strip generated .c includes, add ref headers + init,
    #    emit exact hex-float (%a) for the full_output debug arrays.
    with open(serve_src) as f:
        src = f.read()
    src = re.sub(r'#include "ta_func/[^"]*\.c"\n', '', src)
    src = re.sub(r'#include "ta_common/[^"]*\.c"\n', '', src)
    src = src.replace('#include <stdio.h>',
                      '#include <stdio.h>\n#include "ta_func.h"\n'
                      '#include "ta_memory.h"\n#include "ta_utility.h"\n')
    src = src.replace('int main(void) {',
                      'int main(void) { TA_Initialize(); '
                      'TA_RestoreCandleDefaultSettings(TA_AllCandleSettings);')
    patched = src.replace('snprintf(buf + pos, buf_size - pos, "%.15g", data[i]);',
                          'snprintf(buf + pos, buf_size - pos, "%a", data[i]);')
    if patched == src:
        die("output serializer pattern not found — %a patch NOT applied")
    src = patched
    with open(os.path.join(bin_dir, "_ta_064_serve.c"), "w") as f:
        f.write(src)

    # 2. Shadow-patch ta_abstract_serve.c into bin/ (searched relative to the
    #    includer first, so it wins over the pristine copy on the -I path).
    with open(abstract_src) as f:
        a = f.read()
    a = '#include "fuzz_data.h"\n' + a
    m = re.search(r'int endIdx\s*=\s*json_find_int\(json, "endIdx"\);\n', a)
    if not m:
        die("endIdx anchor not found for input hook")
    a = a[:m.end()] + INPUT_HOOK + a[m.end():]
    anchor = ('"{\\"retCode\\":%d,\\"outBegIdx\\":%d,\\"outNBElement\\":%d,\\"lookback\\":%d",'
              '\n      (int)rc, outBegIdx, outNBElement, (int)lookback);\n')
    idx = a.find(anchor)
    if idx < 0:
        die("response-header anchor not found for output hook")
    a = a[:idx + len(anchor)] + OUTPUT_HOOK + a[idx + len(anchor):]
    with open(os.path.join(bin_dir, "ta_abstract_serve.c"), "w") as f:
        f.write(a)

    # 3. Compile + link against the frozen 0.6.4 lib.
    cmd = ["cc", "-O3", "-flto", "-DNDEBUG", "-DTA_REF_SERVE", "-Wno-everything"]
    cmd += [f"-I{d}" for d in include_dirs(root, bin_dir)]
    cmd += ["-o", os.path.join(bin_dir, "ta_064_serve"),
            os.path.join(bin_dir, "_ta_064_serve.c"), lib_a, "-lm"]
    rc = subprocess.run(cmd).returncode
    for tmp in ("_ta_064_serve.c", "ta_abstract_serve.c"):
        p = os.path.join(bin_dir, tmp)
        if os.path.exists(p):
            os.unlink(p)
    return rc


def main():
    root = find_repo_root()
    bin_dir = os.path.join(root, "bin")
    os.makedirs(bin_dir, exist_ok=True)
    print("=== Building ta_064_serve (frozen v0.6.4 differential oracle) ===")
    lib_a = ensure_worktree_and_lib(root)
    rc = build(root, bin_dir, lib_a)
    print("ta_064_serve:", "OK" if rc == 0 else f"FAILED (exit {rc})")
    sys.exit(rc)


if __name__ == "__main__":
    main()
