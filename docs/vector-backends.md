# Vector math kernels for the elementary functions

Where the functions that are one math-library call per element (SIN, COS, EXP, LN
and the like) stand, what was ruled, and what a kernel for another platform has to
respect. It exists so that the x86-64 work does not start from zero.

## What exists

On Apple Silicon macOS the batch tier of these functions calls vForce, the array
routines of Apple's Accelerate framework. `TA_VMATH_MAP` in `src/ta_func/ta_utility.h`
is the loop, and the generator emits that token for a loop of exactly that shape;
`src/ta_common/ta_global.c` binds the routines from one list, and `ta_global.h` holds
the platform switch. It is always on there: no build option and no run-time switch.
Every other platform computes them with scalar libm.

## Ruled (owner, 2026-10)

- **Batch only.** `Update` and `Peek` keep scalar libm.
- **Last bits may differ.** A result that depends on a transcendental function may
  differ in the last bits between batch and streaming, between languages and between
  platforms. The spec states it under "Transcendental functions"
  (`website/src/spec/versions/README.md`); issue #521 proposes a function flag for
  it.
- **A one-bar batch call is not a performance target.** A caller who wants one bar at
  a time uses a stream.
- **NaN in the input is not supported.** A kernel owes nothing to a series that holds
  one; what it owes is that its own scratch never holds a value that can disturb a
  result.
- **No third-party vector math library** for now. See "x86-64" for what was
  evaluated.

## What a kernel has to give

- **One function of one value.** A result may not depend on the element's position,
  on its neighbour, on the length of the range or on whether the output is the
  input. That is what keeps a sub-range equal to the full range, the float twin equal
  to the double call and `OpenAndFill` equal to batch, bit for bit.
  `test_elementary_lanes` (`test_1in_1out.c`) checks it on every platform. No vendor
  documents it, so it has to be measured, with large arguments and special values
  side by side: an in-domain corpus shows nothing.
- **A stated distance from libm.** The tests hold a kernel value to a one-value
  result through one comparator, `fuzz_vmath_near`, wherever `TA_VMATH_KERNEL` is 1.
  A new platform's kernel turns the same macro on; it adds no second tolerance.
- **A report of itself.** A kernel that cannot be loaded gives way to the plain
  loop: every value still passes and only the speed is lost, so no value test can
  tell, and none should try. `TA_Initialize` loads the kernel, and the library
  reports whether it is loaded through
  `TA_GetRuntimeInfo( "vmath.transcendental" )`, which `ta_regtest` asserts on
  every build right after its first `TA_Initialize`. A new kernel for these
  functions reports under the same key; another family of vectorized routines gets
  its own under `vmath`.
- **A test lane that can be proven off the platform.** With `TA_VMATH_KERNEL` at 0
  the lane compiles to nothing, so the usual gates stay green whether it is right or
  dead. Develop it in a scratch tree with the kernel arm forced on and a mock kernel:
  red before the lane, green after with every lane counter above zero, red again with
  the mock moved past the bound. Then run the real platform.

## Measuring a kernel

- **Fresh arguments.** A short array replayed in a loop trains the branch predictor,
  and scalar trigonometry then looks about twice as fast as it is on a series.
- **Two series.** Scalar libm takes about twice as long per element on independent
  values as on a smooth series; a kernel hardly differs. Report both.
- **Every magnitude, decade by decade.** A scalar libm is fastest on tiny, saturated
  and out-of-domain arguments and a kernel takes nearly the same time everywhere, so a
  kernel that wins on an in-domain average can lose on a whole range of real prices.
  The bench tools' input is price-like: it is out of domain for ASIN and ACOS and
  saturated for TANH. Count, per row, the results that are NaN, infinite, saturated or
  equal to the input, so that a row says which case it timed.
- **The shipped function.** Time `TA_<F>` from two libraries, not a loop written for
  the benchmark.

## Apple Silicon: what was measured

On an M2 Pro, macOS 26. Apple has three routines per function: scalar libm, a
two-lane `_simd_<fn>_d2` in the system math library, and vForce.

- **vForce and the two-lane routine returned the same bits** for every value tried,
  subnormal inputs and results included, and both differ from libm in the last bits.
- **Where the result is a computation vForce takes the same time per element at
  every magnitude**, and less than libm, with one exception to both: TAN between
  100,000 and 1,000,000 (5.19 ns against 1.28 below; libm 4.62 on a smooth series).
  Where the result is none it is slower than libm: SINH and COSH overflowing to
  infinity, TANH saturated.
- **The two-lane routine was rejected.** It is slower than libm below 0.1 on a smooth
  series for SIN, COS, SINH, COSH and TANH, which an in-domain measurement had
  hidden, and slower than vForce on every batch of 100.
- **One value per call is slower than libm's**, by 46% to 557% (SIN 5.2 against 18.3
  ns per call): a fixed cost per call, about the same for every function.
- **vForce's one dependence on a neighbour:** a signalling NaN changes the SIN, COS
  or TAN result of a large argument exactly four places away in the same block of
  eight, blocks counted from the start of the array. No other special value and no
  other function showed any.
- **Linking Accelerate costs every program a slower start**, close to a
  millisecond, whether or not it calls these functions. The image that holds vForce
  alone opens in a small fraction of that, so vForce is bound at run
  time and the library has no link dependency. A call through the bound pointer
  timed the same as a linked call.
- **Apple documents none of this.** Its header says results may vary with the chip
  and the OS release.

## x86-64: what is known

- **glibc's libmvec** has two-lane (`_ZGVbN2v_<fn>`) and four-lane AVX2
  (`_ZGVdN4v_<fn>`) kernels for these functions. In domain, called from a wrapper
  and not yet from the shipped functions, they took about half and about a quarter
  of scalar libm's time per element. What stands in the way:
  - Linking it adds a dependency on `libmvec.so.1` and raises the glibc version the
    library needs. A wheel cannot carry `libmvec.so.1`.
  - Binding it at run time: weak symbols do not work; `dlopen` and `dlsym` do, and
    move the minimum glibc of the binary that calls them.
  - Its results change between glibc versions for some of the functions, and each
    two-lane symbol resolves at load time to a body that may call scalar libm twice.
  - musl, Windows and macOS on x86-64 have no counterpart.
- **Four lanes need a CPU dispatch per function**, as the fused multiply-add clones
  have.
- **Vendored kernels that were measured:**
  - SLEEF's fused multiply-add family returns the same bits on every platform and
    took about half of scalar libm's time with four lanes on x86-64. Its two lanes
    are slower than Apple's libm on Apple Silicon for most functions. Its
    unreleased next version generates header-only kernels. It has one maintainer;
    the owner prefers not to depend on it.
  - CORE-MATH is correctly rounded and scalar: the same bits everywhere, slower than
    libm on a smooth series. It is the accuracy reference the measurements used, not
    a kernel.
  - No Rust, Java or C# port of SLEEF exists that the libraries could depend on.
- **A kernel of our own, generated by ta_codegen,** is the route that gives one
  source and the same bits in C, Rust, Java and C#. It needs generator primitives for
  bit reinterpretation, rounding to even and lookup tables, and an accuracy check
  against a correctly rounded reference. C and Rust would vectorize it; Java and C#
  would run it one value at a time.

## Credit

[PR #85](https://github.com/TA-Lib/ta-lib/pull/85) by @ralph-e-boy proposed Apple's
vector math for these functions and measured the first gains.
