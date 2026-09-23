/* v0.8.1, the first release with explicit fma().
 *
 * Compared from period 1 and over every MAType, with no waiver. The rows cover
 * the two value changes made since, each sized at 3x its measured maximum. */

#include <stddef.h>

#include "ta_ref.h"

static const TaRefTol TOL[] = {
   /* #411 the hoisted 1/period in the Wilder step: absolute for the
    * oscillators, output-relative for a DM (a running sum of price moves). */
   { "CMO",      TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 8.44e-14 */
   { "PLUS_DI",  TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 4.26e-14 */
   { "MINUS_DI", TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 3.55e-14 */
   { "DX",       TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 5.68e-14 */
   { "ADX",      TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 8.53e-14 */
   { "ADXR",     TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 7.11e-14 */
   { "PLUS_DM",  TA_REF_TOL_REL_OUT, 3e-15, 0.0 }, /* measured 9.53e-16 */
   { "MINUS_DM", TA_REF_TOL_REL_OUT, 3e-15, 0.0 }, /* measured 7.98e-16 */
   /* #434 the sums of squares rebuilt against their peak. Output-relative for
    * the variance family; absolute for CORREL, a coefficient in [-1,1], and for
    * RVI, a bounded oscillator. */
   { "VAR",      TA_REF_TOL_REL_OUT, 2e-9,  0.0 }, /* measured 4.73e-10 */
   { "STDDEV",   TA_REF_TOL_REL_OUT, 8e-10, 0.0 }, /* measured 2.37e-10 */
   { "BBANDS",   TA_REF_TOL_REL_OUT, 3e-10, 0.0 }, /* measured 8.96e-11 */
   { "CORREL",   TA_REF_TOL_ABS,     2e-9,  0.0 }, /* measured 3.89e-10 */
   { "RVI",      TA_REF_TOL_ABS,     3e-11, 0.0 }, /* measured 7.5e-12 */
};

const TaRefMember ta_ref_member = {
   .commit      = "5e766ddf650bbd4f5b49ac3c03489559ee094f28",
   .nbFunctions = 201,
   .tol         = TOL,
   .nbTol       = (int)(sizeof(TOL) / sizeof(TOL[0])),
};
