/* TA-LIB Copyright (c) 1999-2002, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   First version.
 *
 */

/* Description:
 *     Test all MA (Moving Average) functions.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

#include "trionan.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_Integer doRangeTestFlag;

   TA_Integer unstablePeriod;

   TA_Integer startIdx;
   TA_Integer endIdx;
   TA_Integer optInTimePeriod_0;
   TA_Integer optInMethod_1;
   TA_Integer compatibility;

   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex;
   TA_Real    oneOfTheExpectedOutReal;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test_ma( const TA_History *history,
                               const TA_Test *test );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /*****************************************/
   /*   SMA TEST - CLASSIC/METASTOCK        */
   /*****************************************/

#ifndef TA_FUNC_NO_RANGE_CHECK
   /* Test with invalid parameters */
   { 0, 0, 0, 251, -1, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_BAD_PARAM,  0,   0,  0,  0 },
#endif

   /* Test suppose to succeed. */
   { 1, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,   91.50,  0,  252-0 }, /* First Value */
   { 0, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,   94.81,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 250,  108.75,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 251,  107.87,  0,  252-0 }, /* Last Value */

   { 1, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,   93.15,  1,  252-1  }, /* First Value */
   { 0, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,   94.59,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   2,   94.73,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 250,  108.31,  1,  252-1  }, /* Last Value */

   { 1, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  90.42,  29,  252-29 }, /* First Value */
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,  90.21,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   2,  89.96,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,  29,  87.12,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 221, 107.95,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 222, 108.42,  29,  252-29 }, /* Last Value */

   /* Same test and result as TA_COMPATIBILITY_DEFAULT */
   { 1, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,   91.50,  0,  252-0 }, /* First Value */
   { 0, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,   94.81,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 250,  108.75,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 251,  107.87,  0,  252-0 }, /* Last Value */

   { 1, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,   93.15,  1,  252-1  }, /* First Value */
   { 0, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,   94.59,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   2,   94.73,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 250,  108.31,  1,  252-1  }, /* Last Value */

   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  90.42,  29,  252-29 }, /* First Value */
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  90.21,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   2,  89.96,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,  29,  87.12,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 221, 107.95,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_SMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 222, 108.42,  29,  252-29 }, /* Last Value */


   /*******************************/
   /*   WMA TEST  - CLASSIC       */
   /*******************************/

#ifndef TA_FUNC_NO_RANGE_CHECK
   /* No output value. */
   { 0, 0, 0, 251,  0, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_BAD_PARAM, 0, 0, 0, 0 },
#endif

   /* One value tests. */
   { 0, 0, 0,   0,  1, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  91.50,   0, 1 }, /* First Value */

   { 0, 0, 1,   1,  1, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  94.81,   1, 1 },
   { 0, 0, 251, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0, 107.87, 251, 1 }, /* Last Value */
   { 0, 0, 2,   2,  2, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  94.52,   2, 1 },

   /* Misc tests: period 1, 2, 30 */
   { 1, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,   91.50,  0,  252-0 }, /* First Value */
   { 0, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,   94.81,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 250,  108.75,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 251,  107.87,  0,  252-0 }, /* Last Value */

   { 1, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,   93.71,  1,  252-1  }, /* First Value */
   { 0, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,   94.52,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   2,   94.85,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 250,  108.16,  1,  252-1  }, /* Last Value */

   { 1, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  88.567,  29,  252-29 }, /* First Value */
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,  88.233,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   2,  88.034,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,  29,  87.191,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 221, 109.3413, 29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 222, 109.3466, 29,  252-29 }, /* Last Value */

   /*******************************/
   /*   WMA TEST  - METASTOCK     */
   /*******************************/

   /* No output value. */
   { 0, 0, 1, 1,  14, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 0, 0, 0, 0},
#ifndef TA_FUNC_NO_RANGE_CHECK
   { 0, 0, 0, 251,  0, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_BAD_PARAM, 0, 0, 0, 0 },
#endif

   /* One value tests. */
   { 0, 0, 0,   0,  1, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 1 }, /* First Value */
   { 0, 0, 1,   1,  1, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.81,   1, 1 },
   { 0, 0, 251, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0, 107.87, 251, 1 }, /* Last Value */
   { 0, 0, 2,   2,  2, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.52,   2, 1 },

   /* Misc tests: period 1, 2, 30 */
   { 1, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,   91.50,  0,  252-0 }, /* First Value */
   { 0, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,   94.81,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 250,  108.75,  0,  252-0 },
   { 0, 0, 0, 251,  1, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 251,  107.87,  0,  252-0 }, /* Last Value */

   { 1, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,   93.71,  1,  252-1  }, /* First Value */
   { 0, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,   94.52,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   2,   94.85,  1,  252-1  },
   { 0, 0, 0, 251,  2, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 250,  108.16,  1,  252-1  }, /* Last Value */

   { 1, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  88.567,  29,  252-29 }, /* First Value */
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  88.233,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   2,  88.034,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,  29,  87.191,  29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 221, 109.3413, 29,  252-29 },
   { 0, 0, 0, 251, 30, TA_MA_WMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 222, 109.3466, 29,  252-29 }, /* Last Value */

   /*******************************/
   /*   EMA TEST - Classic        */
   /*******************************/


   /* No output value. */
   { 0, 0, 1, 1,  14, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 0, 0, 0, 0},
#ifndef TA_FUNC_NO_RANGE_CHECK
   { 0, 0, 0, 251,  0, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_BAD_PARAM, 0, 0, 0, 0 },
#endif

   /* One value tests. */
   { 0, 0, 0,   0,  1, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  91.50,   0, 1 }, /* First Value */
   { 0, 0, 1,   1,  1, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  94.81,   1, 1 },
   { 0, 0, 251, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0, 107.87, 251, 1 }, /* Last Value */
   { 0, 0, 2,   2,  2, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  94.59,   2, 1 },

   /* Misc tests: period 1, 2, 10 */
   { 1, 0, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  91.50, 0, 252 }, /* First Value */
   { 0, 0, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,  94.81, 0, 252 },
   { 0, 0, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 251, 107.87, 0, 252 }, /* Last Value */

   { 1, 0, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   0,  93.15, 1, 251 }, /* First Value */
   { 0, 0, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   1,  93.96, 1, 251 },
   { 0, 0, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS, 250, 108.21, 1, 251 }, /* Last Value */

   { 1, 0, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,    0,  93.22,  9, 243 }, /* First Value */
   { 0, 0, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,    1,  93.75,  9, 243 },
   { 0, 0, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,   20,  86.46,  9, 243 },
   { 0, 0, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_DEFAULT, TA_SUCCESS,  242, 108.97,  9, 243 }, /* Last Value */

   /*******************************/
   /*   EMA TEST - Metastock      */
   /*******************************/


   /* No output value. */
   { 0, 0, 1, 1,  14, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 0, 0, 0, 0},
#ifndef TA_FUNC_NO_RANGE_CHECK
   { 0, 0, 0, 251,  0, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_BAD_PARAM, 0, 0, 0, 0 },
#endif

   /* One value tests. */
   { 0, 0, 0,   0,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 1 }, /* First Value */
   { 0, 0, 1,   1,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.81,   1, 1 },
   { 0, 0, 251, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0, 107.87, 251, 1 }, /* Last Value */

   /* Test with 1 unstable price bar. Test for period 1, 2, 10 */
   { 1, 1, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.81, 1, 252-1 }, /* First Value */
   { 0, 1, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  94.37, 1, 252-1 },
   { 0, 1, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 251-1, 107.87, 1, 252-1 }, /* Last Value */

   { 1, 1, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.15, 1+1, 251-1 }, /* First Value */
   { 0, 1, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  94.78, 1+1, 251-1 },
   { 0, 1, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 250-1, 108.21, 1+1, 251-1 }, /* Last Value */

   { 1, 1, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    0,  93.24,  9+1, 243-1 }, /* First Value */
   { 0, 1, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    1,  93.97,  9+1, 243-1 },
   { 0, 1, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   20,  86.23,  9+1, 243-1 },
   { 0, 1, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 242-1, 108.97,  9+1, 243-1 }, /* Last Value */

   /* Test with 2 unstable price bar. Test for period 1, 2, 10 */
   { 0, 2, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.37, 0+2, 252-2 }, /* First Value */
   { 0, 2, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  95.09, 0+2, 252-2 },
   { 0, 2, 0, 251,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 251-2, 107.87, 0+2, 252-2 }, /* Last Value */

   { 0, 2, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.78, 1+2, 251-2 }, /* First Value */
   { 0, 2, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  94.11, 1+2, 251-2 },
   { 0, 2, 0, 251,  2, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 250-2, 108.21, 1+2, 251-2 }, /* Last Value */

   { 0, 2, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    0,  93.97,  9+2, 243-2 }, /* First Value */
   { 0, 2, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    1,  94.79,  9+2, 243-2 },
   { 0, 2, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   20,  86.39,  9+2, 243-2 },
   { 0, 2, 0, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,  242-2, 108.97,  9+2, 243-2 }, /* Last Value */

   /* Test some limit case with unstable period. */
   { 0, 1,   0,   0,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 0 }, /* no data */
   { 0, 252, 0,   0,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 0 }, /* no data */
   { 0, 300, 0,   0,  1, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 0 }, /* no data */

   /* Last 3 value with 1 unstable, period 10 */
   { 0, 1, 249, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1, 109.22, 249, 3 },
   { 0, 1, 249, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   2, 108.97, 249, 3 },

   /* Last 3 value with 2 unstable, period 10 */
   { 0, 2, 249, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   2, 108.97, 249, 3 },

   /* Last 3 value with 3 unstable, period 10 */
   { 0, 3, 249, 251,  10, TA_MA_EMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   2, 108.97, 249, 3 },

   /*******************************/
   /*  DEMA TEST - Metastock      */
   /*******************************/

   /* No output value. */
   { 0, 0, 1, 1,  14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 0, 0, 0, 0},
#ifndef TA_FUNC_NO_RANGE_CHECK
   { 0, 0, 0, 251,  0, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_BAD_PARAM, 0, 0, 0, 0 },
#endif

   /* One value tests. */
   { 0, 0, 0,   0,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 1 }, /* First Value */
   { 0, 0, 1,   1,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.815,   1, 1 },
   { 0, 0, 251, 251,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0, 107.87, 251, 1 }, /* Last Value */

   /* Test with period 14 */
   { 0, 0, 0, 251, 14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  83.785, 26, 252-26 }, /* First Value */
   { 0, 0, 0, 251, 14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  84.768, 26, 252-26 },
   { 0, 0, 0, 251, 14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-27, 109.467, 26, 252-26 }, /* Last Value */

   /* Test with 1 unstable price bar. Test for period 1, 2, 14 */
   { 1, 1, 0, 251,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.37, 2, 252-2 }, /* First Value */
   { 0, 1, 0, 251,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  95.095, 2, 252-2 },
   { 0, 1, 0, 251,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 251-2, 107.87, 2, 252-2 }, /* Last Value */

   { 1, 1, 0, 251,  2, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  93.960, 4, 252-4 }, /* First Value */
   { 0, 1, 0, 251,  2, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  94.522, 4, 252-4 },
   { 0, 1, 0, 251,  2, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-5, 107.94, 4, 252-4 }, /* Last Value */

   { 1, 1, 0, 251,  14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    0,  84.91,  (13*2)+2, 252-((13*2)+2) }, /* First Value */
   { 0, 1, 0, 251,  14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    1,  84.97,  (13*2)+2, 252-((13*2)+2) },
   { 0, 1, 0, 251,  14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    2,  84.80,  (13*2)+2, 252-((13*2)+2) },
   { 0, 1, 0, 251,  14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,    3,  85.14,  (13*2)+2, 252-((13*2)+2) },
   { 0, 1, 0, 251,  14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   20,  89.83,  (13*2)+2, 252-((13*2)+2) },
   { 0, 1, 0, 251,  14, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-((13*2)+2+1), 109.4676, (13*2)+2, 252-((13*2)+2) }, /* Last Value */

   /* Test some limit case with unstable period. */
   { 0, 1,   0,   0,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 0 }, /* no data */
   { 0, 252, 0,   0,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 0 }, /* no data */
   { 0, 300, 0,   0,  1, TA_MA_DEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 0 }, /* no data */

   /*******************************/
   /*  TEMA TEST - Metastock      */
   /*******************************/
   /* No output value. */
   { 0, 0, 1, 1,  14, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 0, 0, 0, 0},
#ifndef TA_FUNC_NO_RANGE_CHECK
   { 0, 0, 0, 251,  0, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_BAD_PARAM, 0, 0, 0, 0 },
#endif

   /* One value tests. */
   { 0, 0, 0,   0,  1, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  91.50,   0, 1 }, /* First Value */
   { 0, 0, 1,   1,  1, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  94.815,   1, 1 },
   { 0, 0, 251, 251,  1, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0, 107.87, 251, 1 }, /* Last Value */

   /* Test with period 14 */
   { 1, 0, 0, 251, 14, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  84.721, 39, 252-39 }, /* First Value */
   { 0, 0, 0, 251, 14, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  84.089, 39, 252-39 },
   { 0, 0, 0, 251, 14, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 252-40, 108.418, 39, 252-39 }, /* Last Value */

   /* Test with 1 unstable price bar. Test for period 1 */
   { 1, 1, 0, 251,  1, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   0,  95.095, 3, 252-3 }, /* First Value */
   { 0, 1, 0, 251,  1, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS,   1,  93.78, 3, 252-3 },
   { 0, 1, 0, 251,  1, TA_MA_TEMA, TA_COMPATIBILITY_METASTOCK, TA_SUCCESS, 251-3, 107.87, 3, 252-3 }, /* Last Value */

};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_ma( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   for( i=0; i < NB_TEST; i++ )
   {

      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "TA_MA Failed Bad Parameter for Test #%d (%d,%d)\n",
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test_ma( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "TA_MA Failed Test #%d (Code=%d)\n", i, retValue );
         return retValue;
      }
   }

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* All test succeed. */
   return 0; 
}

/**** Local functions definitions.     ****/
static TA_RetCode rangeTestFunction( 
                              TA_Integer startIdx,
                              TA_Integer endIdx,
                              TA_Real *outputBuffer,
                              TA_Integer *outBegIdx,
                              TA_Integer *outNbElement,
                              TA_Integer *lookback,
                              void *opaqueData,
                              unsigned int outputNb )
{
  TA_RetCode retCode;
  TA_RangeTestParam *testParam;

  (void)outputNb;
    
  testParam = (TA_RangeTestParam *)opaqueData;   

  retCode = TA_MA( startIdx,
                   endIdx,
                   testParam->close,
                   testParam->test->optInTimePeriod_0,
                   testParam->test->optInMethod_1,
                   outBegIdx,
                   outNbElement,
                   outputBuffer );

  *lookback = TA_MA_Lookback( testParam->test->optInTimePeriod_0,
                              testParam->test->optInMethod_1 );

  return retCode;
}

static ErrorNumber do_test_ma( const TA_History *history,
                               const TA_Test *test )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx;
   TA_Integer outNbElement;
   TA_RangeTestParam testParam;
   TA_Integer temp, temp2;

   TA_SetCompatibility( test->compatibility );

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->close, history->nbBars );
   setInputBuffer( 1, history->close, history->nbBars );
   
   /* Set the unstable period requested for that test. */
   retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, test->unstablePeriod );
   if( retCode != TA_SUCCESS )
      return TA_TEST_TFRR_SETUNSTABLE_PERIOD_FAIL;

   /* Make a simple first call. */
   retCode = TA_MA( test->startIdx,
                    test->endIdx,
                    gBuffer[0].in,
                    test->optInTimePeriod_0,
                    test->optInMethod_1,
                    &outBegIdx,
                    &outNbElement,
                    gBuffer[0].out0 );

   errNb = checkDataSame( gBuffer[0].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkExpectedValue( gBuffer[0].out0, 
                               retCode, test->expectedRetCode,
                               outBegIdx, test->expectedBegIdx,
                               outNbElement, test->expectedNbElement,
                               test->oneOfTheExpectedOutReal,
                               test->oneOfTheExpectedOutRealIndex );   
   if( errNb != TA_TEST_PASS )
      return errNb;

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_MA( test->startIdx,
                    test->endIdx,
                    gBuffer[1].in,
                    test->optInTimePeriod_0,
                    test->optInMethod_1,
                    &outBegIdx,
                    &outNbElement,
                    gBuffer[1].in );

   /* The previous call to TA_MA should have the same output
    * as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkExpectedValue( gBuffer[1].in, 
                               retCode, test->expectedRetCode,
                               outBegIdx, test->expectedBegIdx,
                               outNbElement, test->expectedNbElement,
                               test->oneOfTheExpectedOutReal,
                               test->oneOfTheExpectedOutRealIndex );   
   if( errNb != TA_TEST_PASS )
      return errNb;

  /* Verify that the "all-purpose" TA_MA_Lookback is consistent
   * with the corresponding moving average lookback function.
   */
   switch( test->optInMethod_1 )
   {
   case TA_MA_WMA:
      temp = TA_WMA_Lookback( test->optInTimePeriod_0 );
      break;

   case TA_MA_SMA:
      temp = TA_SMA_Lookback( test->optInTimePeriod_0 );
      break;

   case TA_MA_EMA:
      temp = TA_EMA_Lookback( test->optInTimePeriod_0 );
      break;

   case TA_MA_DEMA:
      temp = TA_DEMA_Lookback( test->optInTimePeriod_0 );
      break;

   case TA_MA_TEMA:
      temp = TA_TEMA_Lookback( test->optInTimePeriod_0 );
      break;

   default:
      return TA_TEST_TFRR_BAD_MA_TYPE;
   }

   temp2 = TA_MA_Lookback( test->optInTimePeriod_0, test->optInMethod_1 );

   if( temp != temp2 )
      return TA_TEST_TFFR_BAD_MA_LOOKBACK;

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      switch( test->optInMethod_1 )
      {
      case TA_MA_TEMA:
      case TA_MA_DEMA:
      case TA_MA_EMA:
         errNb = doRangeTest( rangeTestFunction, 
                              TA_FUNC_UNST_EMA,
                              (void *)&testParam, 1, 0 );
         break;
      default:
         errNb = doRangeTest( rangeTestFunction, 
                              TA_FUNC_UNST_NONE,
                              (void *)&testParam, 1, 0 );
      }

      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

