#ifndef CODEGEN_TOL_H
#define CODEGEN_TOL_H

/* ---- Transcendental tolerance path (shared by --xlang-hash, server_verify
 * and ta_refdata, which holds a committed reference CSV to it on every machine)
 * A call reaching a transcendental (atan/sin/cos/exp/log/...) cannot be
 * bit-compared against a language whose math does not resolve to the same libm
 * the in-process golden uses: Java's fdlibm never does, and .NET's `Math.*` is
 * not guaranteed to (proven host-dependent — see
 * codegen_lang_transcendental_tol), and neither is a JS engine's `Math.*`.
 * Those calls swap the bitwise out_hash path for a narrow element-compare at
 * the language's bound (relative for |v|>1, absolute otherwise). Measured Java
 * drift over both gates' scenarios peaks ~9.7e-15 (HT_DCPHASE), so 1e-9 keeps
 * ~5 orders of margin over libm noise while still failing any real algorithmic
 * regression (orders of magnitude larger). The TypeScript bound is the
 * owner's: when a function's TRANSCENDENTAL MAX goes over it, report the function
 * to the owner rather than widening it. ---- */
#define CODEGEN_TRANSCENDENTAL_TOL    1e-9
#define CODEGEN_TRANSCENDENTAL_TOL_TS 1e-11

#endif
