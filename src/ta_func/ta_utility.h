/* Provides common mathematical or analysis functions.
 *
 * These functions are all PRIVATE to ta-lib and should
 * never be called directly by the library users.
 */

#ifndef TA_UTILITY_H
#define TA_UTILITY_H

#ifndef TA_FUNC_H
   #include "ta_func.h"
#endif

#ifndef TA_GLOBAL_H
   #include "ta_global.h"
#endif

/* FMA runtime CPU dispatch (PR #96): mark fused indicators with target_clones so
 * a portable baseline build (e.g. a manylinux wheel) dispatches to a hardware-fma
 * clone at load — no -mfma, no SIGILL on pre-2013 CPUs. glibc-only: target_clones
 * needs GNU ifunc, which musl has in NO version (Alpine gcc hard-errors), so
 * musl/macOS/MSVC fall through to plain software fma(). Do NOT relax to __linux__
 * (breaks the musllinux build; guarded by the nightly `musl-build` job).
 * -ffp-contract=off keeps the clones bit-exact with each other and the
 * Rust/Java backends. */
#if defined( __x86_64__ ) && defined( __GLIBC__ ) && defined( __GNUC__ ) && !defined( __clang__ )
   #define TA_FMA_MULTIVERSION __attribute__((target_clones("default","fma")))
#else
   #define TA_FMA_MULTIVERSION
#endif

/* Provides an equivalent to standard "math.h" functions. */
#define std_floor floor
#define std_ceil  ceil
#define std_fabs  fabs
#define std_atan  atan
#define std_cos   cos
#define std_sin   sin
#define std_sqrt  sqrt
#define std_tanh  tanh
#define std_tan   tan
#define std_sinh  sinh
#define std_log10 log10
#define std_log   log
#define std_exp   exp
#define std_cosh  cosh
#define std_asin  asin
#define std_acos  acos

/* TA_UNROLL(n) - ask the compiler to unroll the loop on the next line n times.
 *
 * Purely advisory: it never changes what is computed, and a compiler that
 * ignores it emits exactly the code it emits today.  Used on the expired-extreme
 * rescan of the rolling min/max functions, whose loop body is two instructions,
 * so loop control is a large share of its cost.
 *
 * Why a hint in the source and not a build flag: neither GCC nor Clang unrolls
 * these loops on its own at any -O level (the trip count is only known at run
 * time, and Clang's runtime unroller is off by default -- -funroll-loops does
 * not turn it on), and our build flags never reach the downstream projects that
 * compile the src/ta_func sources straight out of the release tarball.
 *
 * Enabled only where it was measured to pay (rolling min/max, random-walk
 * inputs, medians over a code-layout sweep):
 *
 *    GCC x86-64        -6% to -8%    enabled
 *    Apple clang arm64 -27% (L1/L2-resident), -1% (larger working set)  enabled
 *    Clang x86-64      -1% to +5%    NOT enabled -- no gain, often a loss
 *
 * Clang on x86-64 already emits a different (partly branchless) shape for these
 * loops, and unrolling it is at best neutral.  Re-measure before widening this;
 * an unroll factor is a per-target tuning constant, not a portable truth.
 */
#if defined(__clang__) && defined(__aarch64__)
   #define TA_UNROLL_STR1(x) #x
   #define TA_UNROLL_STR(x)  TA_UNROLL_STR1(x)
   #define TA_UNROLL(n)      _Pragma(TA_UNROLL_STR(clang loop unroll_count(n)))
#elif defined(__GNUC__) && !defined(__clang__) && (__GNUC__ >= 8)
   #define TA_UNROLL_STR1(x) #x
   #define TA_UNROLL_STR(x)  TA_UNROLL_STR1(x)
   #define TA_UNROLL(n)      _Pragma(TA_UNROLL_STR(GCC unroll n))
#else
   #define TA_UNROLL(n)
#endif

/* Rounding macro for doubles. Works only with positive numbers. */
#define round_pos(x) (std_floor((x)+0.5))

/* Rounding macro for doubles. Works only with negative numbers. */
#define round_neg(x) (std_ceil((x)-0.5))

/* Rounding with a precision of 2 digit after the dot */
#define round_pos_2(x) ((std_floor((x*100.0)+0.5))/100.0)
#define round_neg_2(x) ((std_ceil((x*100.0)-0.5))/100.0)

/* In the context of TA-Lib, floating point are often
 * compared within an acceptable error range.
 *
 * As an example,a TA oscillator ranging from 0 to 100 can
 * fairly be considered equal if their difference is less
 * than 0.000001.
 *
 * Ranging around zero also allows to work around limit
 * cases where floating point minimal step (EPSILON) causes
 * unexpected cummulative effect (ending with "negative zero"
 * being one example).
 *
 * FLT_EPSILON == 1.192092896e-07 for float type on intel with msvc.
 * DBL_EPSILON == 2.2204460492503131e-016 for the double type.
 *
 * Warning: These macro are not intended as "general purpose" floating
 * point comparison. TA_REAL_EQ is not even transitive. The "ep" parameter
 * must be carefully choosen to work in the domain of the tested values.
 * Do a search on Google for a more generalize algo.
 *
 * Bounds are inclusive: below half a ULP of "v", v-ep and v+ep round back to
 * v, and strict bounds would then reject bit-identical operands.
 */
#define TA_EPSILON (0.00000000000001)
#define TA_REAL_EQ(x,v,ep)   (((v-ep)<=x)&&(x<=(v+ep)))
#define TA_IS_ZERO(v)        (((-TA_EPSILON)<v)&&(v<TA_EPSILON))
#define TA_IS_ZERO_OR_NEG(v) (v<TA_EPSILON)

/* Scale-aware zero test (issue #107): treats v as zero within TA_EPSILON of the
 * operands' magnitude ('scale' = |a|+|b|) — a ~90-ULP relative dead-zone. Use
 * when v is a DIFFERENCE of comparable magnitudes; a fixed band (TA_IS_ZERO)
 * misses the tie once the operands grow past ~1.0. Keep the multiply-compare
 * form: `fabs(v) - E*scale <= 0` would contract to an FMA and diverge per-backend. */
#define TA_IS_ZERO_SCALED(v,scale) (fabs(v) <= (TA_EPSILON*(scale)))

/* True when v is a usable number: neither NaN nor +/-Inf. Unlike the macros
 * above this is exact, not an epsilon band. Used by the running-product indices
 * (NVI, PVI), which have no upper bound and would otherwise hand the caller an
 * Inf that poisons every later arithmetic. */
#define TA_IS_FINITE(v) (isfinite(v))

/* The following macros are being used to do
 * the Hilbert Transform logic as documented
 * in John Ehlers books "Rocket Science For Traders".
 */
#define HILBERT_VARIABLES(varName) \
   ARRAY_LOCAL(varName##_Odd,3); \
   ARRAY_LOCAL(varName##_Even, 3); \
   double varName; \
   double prev_##varName##_Odd; \
   double prev_##varName##_Even; \
   double prev_##varName##_input_Odd; \
   double prev_##varName##_input_Even

#define INIT_HILBERT_VARIABLES(varName) { \
      varName##_Odd [0] = 0.0; \
      varName##_Odd [1] = 0.0; \
      varName##_Odd [2] = 0.0; \
      varName##_Even[0] = 0.0; \
      varName##_Even[1] = 0.0; \
      varName##_Even[2] = 0.0; \
      varName = 0.0; \
      prev_##varName##_Odd        = 0.0; \
      prev_##varName##_Even       = 0.0; \
      prev_##varName##_input_Odd  = 0.0; \
      prev_##varName##_input_Even = 0.0; \
      }

#define DO_HILBERT_TRANSFORM(varName,input,OddOrEvenId) {\
         hilbertTempReal = a * input; \
         varName = -varName##_##OddOrEvenId[hilbertIdx]; \
         varName##_##OddOrEvenId[hilbertIdx] = hilbertTempReal; \
         varName += hilbertTempReal; \
         varName -= prev_##varName##_##OddOrEvenId; \
         prev_##varName##_##OddOrEvenId = b * prev_##varName##_input_##OddOrEvenId; \
         varName += prev_##varName##_##OddOrEvenId; \
         prev_##varName##_input_##OddOrEvenId = input; \
         varName *= adjustedPrevPeriod; \
         }

#define DO_HILBERT_ODD(varName,input)  DO_HILBERT_TRANSFORM(varName,input,Odd)
#define DO_HILBERT_EVEN(varName,input) DO_HILBERT_TRANSFORM(varName,input,Even)

/* Convert a period into the equivalent k:
 *
 *    k = 2 / (period + 1)
 *
 * Useful to calculate the smoothing factor 'k' of an EMA.
 */
#define PER_TO_K( per ) ((double)2.0 / ((double)(per + 1)))

/* Math Constants and Functions */
#define PI 3.14159265358979323846

#ifndef min
   #define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
   #define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

/* Candlestick macros (used by candlestick functions, where the parameters are always called inClose, inOpen, etc.
 * Don't use i++ or func(i) with these macros !
 */

/* The subtraction operands are widened to double so these compute in double even
 * when the arrays are const float[] (the TA_S_ bodies) — a no-op for the double
 * variants, PR #33's "double throughout" for the single-precision ones (#138).
 * The >= tests need no cast: float->double is order-preserving. */
#define TA_REALBODY(IDX)        ( std_fabs( (double)inClose[IDX] - (double)inOpen[IDX] ) )
#define TA_UPPERSHADOW(IDX)     ( (double)inHigh[IDX] - ( inClose[IDX] >= inOpen[IDX] ? (double)inClose[IDX] : (double)inOpen[IDX] ) )
#define TA_LOWERSHADOW(IDX)     ( ( inClose[IDX] >= inOpen[IDX] ? (double)inOpen[IDX] : (double)inClose[IDX] ) - (double)inLow[IDX] )
#define TA_HIGHLOWRANGE(IDX)    ( (double)inHigh[IDX] - (double)inLow[IDX] )
#define TA_CANDLECOLOR(IDX)     ( inClose[IDX] >= inOpen[IDX] ? 1 : -1 )

#define TA_CANDLERANGETYPE(SET) (TA_Globals->candleSettings[TA_##SET].rangeType)
#define TA_CANDLEAVGPERIOD(SET) (TA_Globals->candleSettings[TA_##SET].avgPeriod)
#define TA_CANDLEFACTOR(SET)    (TA_Globals->candleSettings[TA_##SET].factor)

#define TA_CANDLERANGE(SET,IDX) \
    ( TA_CANDLERANGETYPE(SET) == TA_RangeType_RealBody ? TA_REALBODY(IDX) : \
    ( TA_CANDLERANGETYPE(SET) == TA_RangeType_HighLow  ? TA_HIGHLOWRANGE(IDX) : \
    ( TA_CANDLERANGETYPE(SET) == TA_RangeType_Shadows  ? TA_UPPERSHADOW(IDX) + TA_LOWERSHADOW(IDX) : \
      0 ) ) )
#define TA_CANDLEAVERAGE(SET,SUM,IDX) \
    ( TA_CANDLEFACTOR(SET) \
        * ( TA_CANDLEAVGPERIOD(SET) != 0.0? SUM / TA_CANDLEAVGPERIOD(SET) : TA_CANDLERANGE(SET,IDX) ) \
        / ( TA_CANDLERANGETYPE(SET) == TA_RangeType_Shadows ? 2.0 : 1.0 ) \
    )
#define TA_REALBODYGAPUP(IDX2,IDX1)     ( min(inOpen[IDX2],inClose[IDX2]) > max(inOpen[IDX1],inClose[IDX1]) )
#define TA_REALBODYGAPDOWN(IDX2,IDX1)   ( max(inOpen[IDX2],inClose[IDX2]) < min(inOpen[IDX1],inClose[IDX1]) )
#define TA_CANDLEGAPUP(IDX2,IDX1)       ( inLow[IDX2] > inHigh[IDX1] )
#define TA_CANDLEGAPDOWN(IDX2,IDX1)     ( inHigh[IDX2] < inLow[IDX1] )

/* Scalar-argument candle macros for the generated streaming API.
 * These MUST mirror TA_CANDLERANGE/TA_CANDLEAVERAGE above exactly (same
 * operations in the same order) with the bar's OHLC supplied as scalar
 * expressions instead of array reads — the stream transition has no input
 * arrays.  Any change to the batch macros must be mirrored here, or the
 * streams lose bit-exactness with batch on the affected range type.
 */
#define TA_STREAM_REALBODY(O,H,L,C)     ( std_fabs( (double)(C) - (double)(O) ) )
#define TA_STREAM_UPPERSHADOW(O,H,L,C)  ( (double)(H) - ( (C) >= (O) ? (double)(C) : (double)(O) ) )
#define TA_STREAM_LOWERSHADOW(O,H,L,C)  ( ( (C) >= (O) ? (double)(O) : (double)(C) ) - (double)(L) )
#define TA_STREAM_HIGHLOWRANGE(O,H,L,C) ( (double)(H) - (double)(L) )
#define TA_STREAM_CANDLERANGE(SET,O,H,L,C) \
    ( TA_CANDLERANGETYPE(SET) == TA_RangeType_RealBody ? TA_STREAM_REALBODY(O,H,L,C) : \
    ( TA_CANDLERANGETYPE(SET) == TA_RangeType_HighLow  ? TA_STREAM_HIGHLOWRANGE(O,H,L,C) : \
    ( TA_CANDLERANGETYPE(SET) == TA_RangeType_Shadows  ? TA_STREAM_UPPERSHADOW(O,H,L,C) + TA_STREAM_LOWERSHADOW(O,H,L,C) : \
      0 ) ) )
#define TA_STREAM_CANDLEAVERAGE(SET,SUM,O,H,L,C) \
    ( TA_CANDLEFACTOR(SET) \
        * ( TA_CANDLEAVGPERIOD(SET) != 0.0? (SUM) / TA_CANDLEAVGPERIOD(SET) : TA_STREAM_CANDLERANGE(SET,O,H,L,C) ) \
        / ( TA_CANDLERANGETYPE(SET) == TA_RangeType_Shadows ? 2.0 : 1.0 ) \
    )

#endif
