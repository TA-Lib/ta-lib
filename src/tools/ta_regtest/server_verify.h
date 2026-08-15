#ifndef SERVER_VERIFY_H
#define SERVER_VERIFY_H

#include "ta_error_number.h"
#include "ta_libc.h"
#include "codegen_pipe.h"

#define SV_MAX_PIPES 4

/* Initialize server verification with one or more server pipes.
 * `langs[i]` is the language token of pipes[i] ("c"/"rust"/"java"/"csharp"),
 * used to pick the per-pipe comparison policy (bitwise vs the Java-transcendental
 * tolerance). Called from ta_regtest.c when --codegen is active. */
void server_verify_init(CodegenPipe *pipes[], const char *langs[], int nbPipes);

/* Shut down server verification (frees buffers). */
void server_verify_shutdown(void);

/* Returns 1 if server verification is active (at least one pipe). */
int server_verify_active(void);

/* How many candle-setting changes have been pushed to servers, summed over all
 * pipes (#215). A caller that varies the settings asserts this moved: a sweep
 * whose settings never reached the servers compares every language at the
 * defaults and passes without testing anything it claims to test. */
int server_verify_candle_syncs(void);

/* Comparisons whose verdict was actually taken, summed over pipes. Require this
 * to ADVANCE across a gate: "no failure reported" and "nothing was ever
 * compared" are otherwise the same observation, which is how a server erroring
 * every request produced a byte-identical green run. */
int server_verify_comparisons(void);

/* Verify a C function call against all active servers.
 *
 * Uses ta_abstract metadata to build JSON-RPC requests internally.
 * Test authors only need to pass the same data they already have.
 *
 * funcName:         Function name WITHOUT "TA_" prefix (e.g. "RSI", "BBANDS")
 * startIdx/endIdx:  Range passed to the C function
 * nbBars:           Length of input arrays
 * crefRetCode:      Return code from C reference call
 * crefOutBegIdx:    outBegIdx from C reference call
 * crefOutNbElement: outNbElement from C reference call
 * inputs:           NULL-terminated array of input data pointers in signature order.
 *                   For Price inputs, pass components in OHLCV+OI order (only used ones).
 *                   For Real inputs, pass each array in order.
 * optParams:        Optional parameter values in signature order, all as double.
 *                   Integer params are truncated internally based on ta_abstract type info.
 *                   NULL if no optional params.
 * nbOptParams:      Number of elements in optParams (0 if none).
 * outReal:          NULL-terminated array of C real output buffers, or NULL if none.
 * outInteger:       NULL-terminated array of C integer output buffers, or NULL if none.
 *
 * Returns TA_TEST_PASS on success (or if no servers are active).
 */
ErrorNumber server_verify(
    const char       *funcName,
    TA_Integer        startIdx,
    TA_Integer        endIdx,
    int               nbBars,
    TA_RetCode        crefRetCode,
    TA_Integer        crefOutBegIdx,
    TA_Integer        crefOutNbElement,
    const TA_Real    *inputs[],
    const double      optParams[],
    int               nbOptParams,
    const TA_Real    *outReal[],
    const TA_Integer *outInteger[]
);

#endif
