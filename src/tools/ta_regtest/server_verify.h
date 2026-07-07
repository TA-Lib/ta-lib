#ifndef SERVER_VERIFY_H
#define SERVER_VERIFY_H

#include "ta_error_number.h"
#include "ta_libc.h"
#include "codegen_pipe.h"

#define SV_MAX_PIPES 4

/* Initialize server verification with one or more server pipes.
 * Called from ta_regtest.c when --codegen is active. */
void server_verify_init(CodegenPipe *pipes[], int nbPipes);

/* Shut down server verification (frees buffers). */
void server_verify_shutdown(void);

/* Returns 1 if server verification is active (at least one pipe). */
int server_verify_active(void);

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
