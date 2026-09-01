/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
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

/* Frozen v0.6.4 reference values, over the frozen 252-bar ta_regtest series.
 * Every value carries the full 17 significant digits v0.6.4 produced, so it
 * round-trips to the exact double and transcription is not an error source.
 * GENERATED -- do not hand-edit values. The COMMENTS here and the tolerance
 * table in ta_test_legacy.c are hand-maintained, and are where the maintenance
 * lives. `src/tools/ta_regtest/CLAUDE.md` states what this group is for.
 *
 * SCOPE. Deliberately absent: every function that post-dates v0.6.4, which has
 *   no answer to freeze (they carry their own tests); STOCHRSI, which diverges
 *   on purpose and is pinned by test_stoch.c; and everything whose value depends
 *   on the host libm. That last group is the 12 pure passthroughs (ACOS..TANH),
 *   since atan/sin/cos/exp/log are not correctly rounded and a frozen value
 *   would assert "your libm matches the machine that generated this table", plus
 *   the 6 Hilbert functions, whose atan becomes an INTEGER loop bound
 *   (period = 360/(atan(Im/Re)*rad2Deg), then (int)DCPeriod) so one ULP of libm
 *   difference near a boundary changes the iteration count and the output moves
 *   discontinuously. sqrt/ceil/floor are correctly rounded and stay in scope.
 *   This bites here and not in --fuzz-064 because that gate compares two
 *   binaries on ONE host with ONE libm, while a frozen table is read on every
 *   host; the cross-implementation coverage is --xlang-hash's tolerance lane.
 *
 * ONE-SIDED CASES. A candlestick pattern that never fires anywhere on this
 *   series is pinned at 0 at every sample, which fails a pattern that STARTS
 *   firing but cannot see one that STOPS. Those patterns are listed in the table
 *   below by their all-zero rows; two-sided coverage for them is
 *   test_candlestick.c's predicate / MC-DC gate. The pins still cover
 *   retCode/outBegIdx/outNbElement, so a lookback or shape regression fails.
 *
 * SAMPLING. First, middle and last of each output; every INTEGER output
 *   additionally pins the first, middle and last occurrence of both its minimum
 *   and its maximum. Pinning only the non-zero bars reads backwards any discrete
 *   output whose rare arm is zero -- a function sitting at 1 for most of the
 *   series gives 1, 1, 1 and pins nothing.
 *
 * INPUTS. A `real` input takes close, a second `real` input takes high, and
 *   price inputs take the OHLCV components ta_abstract declares.
 *   ta_test_legacy.c's legacy_setup_inputs mirrors this; the two must not drift.
 *
 * RANGE. Every case is the full call, startIdx 0 to endIdx 251. The
 *   partial-range dimension belongs to doRangeTest.
 *
 * PARAMETERS. Each function contributes its ta_abstract defaults, plus one
 *   non-default set where it has parameters. Two things a regeneration must not
 *   lose: integer periods are never 1 (that territory belongs to the
 *   PERIOD1/BOUNDARY group) and MAType never exceeds 8 (9+ post-date the freeze).
 *
 * RE-FREEZE. Regenerate against the new reference, then DELETE the rows in
 *   ta_test_legacy.c's LEGACY_TOL -- do not set them to 0.0, the gate rejects a
 *   zero row on purpose, since absence already means exact. The libm floors are
 *   the one part that may need to survive.
 *
 *   One trap a regeneration must not fall into: the server names real inputs
 *   POSITIONALLY (inReal, or inReal0/inReal1), NOT by the ta_abstract input
 *   name. MAVP is the one function where those differ -- its inputs are inReal
 *   and inPeriods. Send the declared name and the server finds no array and
 *   computes on whatever the PREVIOUS request left in its buffer, with no error.
 *   It stayed undetected until a case existed whose periods did not saturate to
 *   optInMaxPeriod, because saturated garbage and saturated `high` give the same
 *   answer.
 */

#ifndef TA_TEST_LEGACY_DATA_H
#define TA_TEST_LEGACY_DATA_H

#define TA_LEGACY_MAX_OPT     8
#define TA_LEGACY_MAX_SAMPLE  13

typedef struct
{
   int    outputNb;   /* which output, in ta_abstract order */
   int    index;      /* index into that output array */
   double value;      /* what v0.6.4 returned there */
} TA_LegacySample;

typedef struct
{
   const char     *func;       /* ta_abstract name, no "TA_" prefix */
   unsigned int    nbOpt;      /* optional params, in ta_abstract order */
   double          opt[TA_LEGACY_MAX_OPT];
   TA_RetCode      expectedRetCode;
   int             expectedBegIdx;
   int             expectedNbElement;
   unsigned int    nbSample;
   TA_LegacySample sample[TA_LEGACY_MAX_SAMPLE];
} TA_LegacyCase;

static const TA_LegacyCase TA_LEGACY_CASE[] = {

/* ---- ACCBANDS ---------------------------------------------------------- */
{ "ACCBANDS", 1, { 20 },
  TA_SUCCESS, 19, 233, 9, {   /* default: TimePeriod=20 */
     { 0,   0, 100.37355556565595       },
     { 0, 116, 139.07621085293724       },
     { 0, 232, 119.65651267719167       },
     { 1,   0, 92.890999999999991       },
     { 1, 116, 130.58699999999999       },
     { 1, 232, 110.56999999999996       },
     { 2,   0, 85.451055565655963       },
     { 2, 116, 122.2137108529372        },
     { 2, 232, 101.85401267719163       },
  } },
{ "ACCBANDS", 1, { 25 },
  TA_SUCCESS, 24, 228, 9, {   /* alt: TimePeriod=25 */
     { 0,   0, 98.963467313168863       },
     { 0, 114, 138.13084218373623       },
     { 0, 227, 117.65924977522471       },
     { 1,   0, 91.403999999999996       },
     { 1, 114, 129.35199999999998       },
     { 1, 227, 109.29359999999996       },
     { 2,   0, 84.118467313168864       },
     { 2, 114, 120.91284218373632       },
     { 2, 227, 101.22524977522482       },
  } },

/* ---- AD ---------------------------------------------------------------- */
{ "AD", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, -1631000.0               },
     { 0, 126, 25051948.032613546       },
     { 0, 251, 8328944.5405968949       },
  } },

/* ---- ADD --------------------------------------------------------------- */
{ "ADD", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 184.75                   },
     { 0, 126, 264.88                   },
     { 0, 251, 217.37                   },
  } },

/* ---- ADOSC ------------------------------------------------------------- */
{ "ADOSC", 2, { 3, 10 },
  TA_SUCCESS, 9, 243, 3, {   /* default: FastPeriod=3, SlowPeriod=10 */
     { 0,   0, 841238.32545381016       },
     { 0, 121, 4262927.9335192032       },
     { 0, 242, -1139932.7295189183      },
  } },
{ "ADOSC", 2, { 8, 15 },
  TA_SUCCESS, 14, 238, 3, {   /* alt: FastPeriod=8, SlowPeriod=15 */
     { 0,   0, 1683120.1242520297       },
     { 0, 119, 2572586.4607280716       },
     { 0, 237, 1163428.280132452        },
  } },

/* ---- ADX --------------------------------------------------------------- */
{ "ADX", 1, { 14 },
  TA_SUCCESS, 27, 225, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 22.999310364894765       },
     { 0, 112, 37.145378856660109       },
     { 0, 224, 15.526058354703411       },
  } },
{ "ADX", 1, { 19 },
  TA_SUCCESS, 37, 215, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 17.221011456722092       },
     { 0, 107, 26.024574088520382       },
     { 0, 214, 14.15782969169627        },
  } },

/* ---- ADXR -------------------------------------------------------------- */
{ "ADXR", 1, { 14 },
  TA_SUCCESS, 40, 212, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 19.866610345997181       },
     { 0, 106, 37.084038397716817       },
     { 0, 211, 20.492087787561218       },
  } },
{ "ADXR", 1, { 19 },
  TA_SUCCESS, 55, 197, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 15.523288684349961       },
     { 0,  98, 29.025761058780667       },
     { 0, 196, 17.120368275581896       },
  } },

/* ---- APO --------------------------------------------------------------- */
{ "APO", 3, { 12, 26, 1 },
  TA_SUCCESS, 25, 227, 3, {   /* default: FastPeriod=12, SlowPeriod=26, MAType=1 */
     { 0,   0, -4.1102909376869263      },
     { 0, 113, 2.6111556619528642       },
     { 0, 226, 0.90400929950131115      },
  } },
{ "APO", 3, { 17, 31, 2 },
  TA_SUCCESS, 30, 222, 3, {   /* alt: FastPeriod=17, SlowPeriod=31, MAType=2 */
     { 0,   0, -2.2257776591819294      },
     { 0, 111, 0.32297780940334064      },
     { 0, 221, -0.1581456883826462      },
  } },

/* ---- AROON ------------------------------------------------------------- */
{ "AROON", 1, { 14 },
  TA_SUCCESS, 14, 238, 6, {   /* default: TimePeriod=14 */
     { 0,   0, 100.0                    },
     { 0, 119, 0.0                      },
     { 0, 237, 21.428571428571431       },
     { 1,   0, 78.571428571428569       },
     { 1, 119, 85.714285714285722       },
     { 1, 237, 7.1428571428571432       },
  } },
{ "AROON", 1, { 19 },
  TA_SUCCESS, 19, 233, 6, {   /* alt: TimePeriod=19 */
     { 0,   0, 73.684210526315795       },
     { 0, 116, 15.789473684210527       },
     { 0, 232, 42.10526315789474        },
     { 1,   0, 57.894736842105267       },
     { 1, 116, 78.94736842105263        },
     { 1, 232, 21.05263157894737        },
  } },

/* ---- AROONOSC ---------------------------------------------------------- */
{ "AROONOSC", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, -21.428571428571431      },
     { 0, 119, 85.714285714285722       },
     { 0, 237, -14.285714285714286      },
  } },
{ "AROONOSC", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, -15.789473684210527      },
     { 0, 116, 63.15789473684211        },
     { 0, 232, -21.05263157894737       },
  } },

/* ---- ATR --------------------------------------------------------------- */
{ "ATR", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 3.5782142857142856       },
     { 0, 119, 3.5096125755031076       },
     { 0, 237, 3.2608094811681352       },
  } },
{ "ATR", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 3.3684210526315788       },
     { 0, 116, 3.5856357546538407       },
     { 0, 232, 3.4311527034444986       },
  } },

/* ---- AVGDEV ------------------------------------------------------------ */
{ "AVGDEV", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 1.9689285714285714       },
     { 0, 119, 4.6725510204081662       },
     { 0, 238, 0.69591836734693602      },
  } },
{ "AVGDEV", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 2.1228531855955701       },
     { 0, 117, 5.0357340720221595       },
     { 0, 233, 2.3435457063711955       },
  } },

/* ---- AVGPRICE ---------------------------------------------------------- */
{ "AVGPRICE", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 92.0                     },
     { 0, 126, 132.04750000000001       },
     { 0, 251, 108.295                  },
  } },

/* ---- BBANDS ------------------------------------------------------------ */
{ "BBANDS", 4, { 20, 2.0, 2.0, 0 },
  TA_SUCCESS, 19, 233, 9, {   /* default: TimePeriod=20, NbDevUp=2.0, NbDevDn=2.0, MAType=0 */
     { 0,   0, 98.073394813211024       },
     { 0, 116, 142.10578135915463       },
     { 0, 232, 116.59999170812411       },
     { 1,   0, 92.890999999999991       },
     { 1, 116, 130.58699999999999       },
     { 1, 232, 110.56999999999996       },
     { 2,   0, 87.708605186788958       },
     { 2, 116, 119.06821864084534       },
     { 2, 232, 104.54000829187582       },
  } },
{ "BBANDS", 4, { 25, 3.0, 3.0, 1 },
  TA_SUCCESS, 24, 228, 9, {   /* alt: TimePeriod=25, NbDevUp=3.0, NbDevDn=3.0, MAType=1 */
     { 0,   0, 103.08387422877504       },
     { 0, 114, 146.57112497383235       },
     { 0, 227, 119.34409318915593       },
     { 1,   0, 91.403999999999996       },
     { 1, 114, 128.73947827477426       },
     { 1, 227, 108.14455072707256       },
     { 2,   0, 79.724125771224948       },
     { 2, 114, 110.90783157571614       },
     { 2, 227, 96.945008264989184       },
  } },

/* ---- BETA -------------------------------------------------------------- */
{ "BETA", 1, { 5 },
  TA_SUCCESS, 5, 247, 3, {   /* default: TimePeriod=5 */
     { 0,   0, 0.25828085507777881      },
     { 0, 123, -0.58454681710679568     },
     { 0, 246, 0.18100995915139045      },
  } },
{ "BETA", 1, { 10 },
  TA_SUCCESS, 10, 242, 3, {   /* alt: TimePeriod=10 */
     { 0,   0, 0.45084814752019886      },
     { 0, 121, 0.72373086756882399      },
     { 0, 241, 0.16541245026710413      },
  } },

/* ---- BOP --------------------------------------------------------------- */
{ "BOP", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, -0.40000000000000002     },
     { 0, 126, -0.48859934853420306     },
     { 0, 251, -0.45833333333333171     },
  } },

/* ---- CCI --------------------------------------------------------------- */
{ "CCI", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, -123.06431971659491      },
     { 0, 119, 96.274509803921774       },
     { 0, 238, -98.359503987074376      },
  } },
{ "CCI", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, -65.445323007701873      },
     { 0, 117, 65.957502939668956       },
     { 0, 233, -68.398198581248593      },
  } },

/* ---- CDL2CROWS --------------------------------------------------------- */
{ "CDL2CROWS", 0, { 0 },
  TA_SUCCESS, 12, 240, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDL3BLACKCROWS ---------------------------------------------------- */
{ "CDL3BLACKCROWS", 0, { 0 },
  TA_SUCCESS, 13, 239, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 119, 0.0                      },
     { 0, 238, 0.0                      },
  } },

/* ---- CDL3INSIDE -------------------------------------------------------- */
{ "CDL3INSIDE", 0, { 0 },
  TA_SUCCESS, 12, 240, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   3, 100.0                    },
     { 0, 120, 0.0                      },
     { 0, 169, -100.0                   },
     { 0, 178, 100.0                    },
     { 0, 196, 100.0                    },
     { 0, 239, 0.0                      },
  } },

/* ---- CDL3LINESTRIKE ---------------------------------------------------- */
{ "CDL3LINESTRIKE", 0, { 0 },
  TA_SUCCESS, 8, 244, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 122, 0.0                      },
     { 0, 243, 0.0                      },
  } },

/* ---- CDL3OUTSIDE ------------------------------------------------------- */
{ "CDL3OUTSIDE", 0, { 0 },
  TA_SUCCESS, 3, 249, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  24, 100.0                    },
     { 0,  55, 100.0                    },
     { 0,  74, 100.0                    },
     { 0, 124, 0.0                      },
     { 0, 174, -100.0                   },
     { 0, 207, -100.0                   },
     { 0, 247, -100.0                   },
     { 0, 248, 0.0                      },
  } },

/* ---- CDL3STARSINSOUTH -------------------------------------------------- */
{ "CDL3STARSINSOUTH", 0, { 0 },
  TA_SUCCESS, 12, 240, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDL3WHITESOLDIERS ------------------------------------------------- */
{ "CDL3WHITESOLDIERS", 0, { 0 },
  TA_SUCCESS, 12, 240, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLABANDONEDBABY -------------------------------------------------- */
{ "CDLABANDONEDBABY", 1, { 0.29999999999999999 },
  TA_SUCCESS, 12, 240, 3, {   /* default: Penetration=0.29999999999999999 */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },
{ "CDLABANDONEDBABY", 1, { 0.44999999999999996 },
  TA_SUCCESS, 12, 240, 3, {   /* alt: Penetration=0.44999999999999996 */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLADVANCEBLOCK --------------------------------------------------- */
{ "CDLADVANCEBLOCK", 0, { 0 },
  TA_SUCCESS, 12, 240, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLBELTHOLD ------------------------------------------------------- */
{ "CDLBELTHOLD", 0, { 0 },
  TA_SUCCESS, 10, 242, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   6, -100.0                   },
     { 0,   8, 100.0                    },
     { 0, 106, 100.0                    },
     { 0, 121, 100.0                    },
     { 0, 172, -100.0                   },
     { 0, 204, 100.0                    },
     { 0, 239, -100.0                   },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLBREAKAWAY ------------------------------------------------------ */
{ "CDLBREAKAWAY", 0, { 0 },
  TA_SUCCESS, 14, 238, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 119, 0.0                      },
     { 0, 237, 0.0                      },
  } },

/* ---- CDLCLOSINGMARUBOZU ------------------------------------------------ */
{ "CDLCLOSINGMARUBOZU", 0, { 0 },
  TA_SUCCESS, 10, 242, 8, {   /* default: no parameters */
     { 0,   0, 100.0                    },
     { 0,   6, -100.0                   },
     { 0,  80, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 166, -100.0                   },
     { 0, 212, 100.0                    },
     { 0, 240, -100.0                   },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLCONCEALBABYSWALL ----------------------------------------------- */
{ "CDLCONCEALBABYSWALL", 0, { 0 },
  TA_SUCCESS, 13, 239, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 119, 0.0                      },
     { 0, 238, 0.0                      },
  } },

/* ---- CDLCOUNTERATTACK -------------------------------------------------- */
{ "CDLCOUNTERATTACK", 0, { 0 },
  TA_SUCCESS, 11, 241, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLDARKCLOUDCOVER ------------------------------------------------- */
{ "CDLDARKCLOUDCOVER", 1, { 0.5 },
  TA_SUCCESS, 11, 241, 6, {   /* default: Penetration=0.5 */
     { 0,   0, 0.0                      },
     { 0,   8, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 206, -100.0                   },
     { 0, 214, -100.0                   },
     { 0, 240, 0.0                      },
  } },
{ "CDLDARKCLOUDCOVER", 1, { 0.75 },
  TA_SUCCESS, 11, 241, 6, {   /* alt: Penetration=0.75 */
     { 0,   0, 0.0                      },
     { 0,   8, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 206, -100.0                   },
     { 0, 214, -100.0                   },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLDOJI ----------------------------------------------------------- */
{ "CDLDOJI", 0, { 0 },
  TA_SUCCESS, 10, 242, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   1, 100.0                    },
     { 0,  91, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 125, 0.0                      },
     { 0, 238, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLDOJISTAR ------------------------------------------------------- */
{ "CDLDOJISTAR", 0, { 0 },
  TA_SUCCESS, 11, 241, 7, {   /* default: no parameters */
     { 0,   0, -100.0                   },
     { 0,   1, 0.0                      },
     { 0,  67, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 121, 0.0                      },
     { 0, 145, -100.0                   },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLDRAGONFLYDOJI -------------------------------------------------- */
{ "CDLDRAGONFLYDOJI", 0, { 0 },
  TA_SUCCESS, 10, 242, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   4, 100.0                    },
     { 0,  74, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 122, 0.0                      },
     { 0, 237, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLENGULFING ------------------------------------------------------ */
{ "CDLENGULFING", 0, { 0 },
  TA_SUCCESS, 2, 250, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   2, -100.0                   },
     { 0,  24, 100.0                    },
     { 0,  74, 100.0                    },
     { 0, 125, 80.0                     },
     { 0, 189, -100.0                   },
     { 0, 233, 100.0                    },
     { 0, 247, -100.0                   },
     { 0, 249, 0.0                      },
  } },

/* ---- CDLEVENINGDOJISTAR ------------------------------------------------ */
{ "CDLEVENINGDOJISTAR", 1, { 0.29999999999999999 },
  TA_SUCCESS, 12, 240, 5, {   /* default: Penetration=0.29999999999999999 */
     { 0,   0, 0.0                      },
     { 0,  67, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 145, -100.0                   },
     { 0, 239, 0.0                      },
  } },
{ "CDLEVENINGDOJISTAR", 1, { 0.44999999999999996 },
  TA_SUCCESS, 12, 240, 5, {   /* alt: Penetration=0.44999999999999996 */
     { 0,   0, 0.0                      },
     { 0,  67, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 145, -100.0                   },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLEVENINGSTAR ---------------------------------------------------- */
{ "CDLEVENINGSTAR", 1, { 0.29999999999999999 },
  TA_SUCCESS, 12, 240, 5, {   /* default: Penetration=0.29999999999999999 */
     { 0,   0, 0.0                      },
     { 0,  67, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 145, -100.0                   },
     { 0, 239, 0.0                      },
  } },
{ "CDLEVENINGSTAR", 1, { 0.44999999999999996 },
  TA_SUCCESS, 12, 240, 5, {   /* alt: Penetration=0.44999999999999996 */
     { 0,   0, 0.0                      },
     { 0,  67, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 145, -100.0                   },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLGAPSIDESIDEWHITE ----------------------------------------------- */
{ "CDLGAPSIDESIDEWHITE", 0, { 0 },
  TA_SUCCESS, 7, 245, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 122, 0.0                      },
     { 0, 244, 0.0                      },
  } },

/* ---- CDLGRAVESTONEDOJI ------------------------------------------------- */
{ "CDLGRAVESTONEDOJI", 0, { 0 },
  TA_SUCCESS, 10, 242, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  73, 100.0                    },
     { 0,  98, 100.0                    },
     { 0, 110, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 122, 0.0                      },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLHAMMER --------------------------------------------------------- */
{ "CDLHAMMER", 0, { 0 },
  TA_SUCCESS, 11, 241, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  25, 100.0                    },
     { 0,  83, 100.0                    },
     { 0, 120, 0.0                      },
     { 0, 121, 0.0                      },
     { 0, 229, 100.0                    },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLHANGINGMAN ----------------------------------------------------- */
{ "CDLHANGINGMAN", 0, { 0 },
  TA_SUCCESS, 11, 241, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  63, -100.0                   },
     { 0, 119, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 145, -100.0                   },
     { 0, 231, -100.0                   },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLHARAMI --------------------------------------------------------- */
{ "CDLHARAMI", 0, { 0 },
  TA_SUCCESS, 11, 241, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   3, 100.0                    },
     { 0,  17, -100.0                   },
     { 0,  89, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 169, -100.0                   },
     { 0, 178, 100.0                    },
     { 0, 235, 100.0                    },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLHARAMICROSS ---------------------------------------------------- */
{ "CDLHARAMICROSS", 0, { 0 },
  TA_SUCCESS, 11, 241, 8, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   3, 100.0                    },
     { 0,  13, 100.0                    },
     { 0,  34, -100.0                   },
     { 0,  52, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 207, 100.0                    },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLHIGHWAVE ------------------------------------------------------- */
{ "CDLHIGHWAVE", 0, { 0 },
  TA_SUCCESS, 10, 242, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   1, -100.0                   },
     { 0,  14, 100.0                    },
     { 0,  73, -100.0                   },
     { 0, 121, 0.0                      },
     { 0, 146, 100.0                    },
     { 0, 224, -100.0                   },
     { 0, 237, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLHIKKAKE -------------------------------------------------------- */
{ "CDLHIKKAKE", 0, { 0 },
  TA_SUCCESS, 5, 247, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  25, -200.0                   },
     { 0,  87, 100.0                    },
     { 0, 123, 0.0                      },
     { 0, 146, 100.0                    },
     { 0, 187, -200.0                   },
     { 0, 197, -200.0                   },
     { 0, 235, 100.0                    },
     { 0, 246, 0.0                      },
  } },

/* ---- CDLHIKKAKEMOD ----------------------------------------------------- */
{ "CDLHIKKAKEMOD", 0, { 0 },
  TA_SUCCESS, 10, 242, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 121, 0.0                      },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLHOMINGPIGEON --------------------------------------------------- */
{ "CDLHOMINGPIGEON", 0, { 0 },
  TA_SUCCESS, 11, 241, 6, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   3, 100.0                    },
     { 0, 120, 0.0                      },
     { 0, 207, 100.0                    },
     { 0, 235, 100.0                    },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLIDENTICAL3CROWS ------------------------------------------------ */
{ "CDLIDENTICAL3CROWS", 0, { 0 },
  TA_SUCCESS, 12, 240, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLINNECK --------------------------------------------------------- */
{ "CDLINNECK", 0, { 0 },
  TA_SUCCESS, 11, 241, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLINVERTEDHAMMER ------------------------------------------------- */
{ "CDLINVERTEDHAMMER", 0, { 0 },
  TA_SUCCESS, 11, 241, 5, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  87, 100.0                    },
     { 0, 120, 0.0                      },
     { 0, 129, 100.0                    },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLKICKING -------------------------------------------------------- */
{ "CDLKICKING", 0, { 0 },
  TA_SUCCESS, 11, 241, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLKICKINGBYLENGTH ------------------------------------------------ */
{ "CDLKICKINGBYLENGTH", 0, { 0 },
  TA_SUCCESS, 11, 241, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLLADDERBOTTOM --------------------------------------------------- */
{ "CDLLADDERBOTTOM", 0, { 0 },
  TA_SUCCESS, 14, 238, 4, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  85, 100.0                    },
     { 0, 119, 0.0                      },
     { 0, 237, 0.0                      },
  } },

/* ---- CDLLONGLEGGEDDOJI ------------------------------------------------- */
{ "CDLLONGLEGGEDDOJI", 0, { 0 },
  TA_SUCCESS, 10, 242, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   1, 100.0                    },
     { 0,  91, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 125, 0.0                      },
     { 0, 238, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLLONGLINE ------------------------------------------------------- */
{ "CDLLONGLINE", 0, { 0 },
  TA_SUCCESS, 10, 242, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   6, -100.0                   },
     { 0,   8, 100.0                    },
     { 0, 112, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 143, -100.0                   },
     { 0, 223, 100.0                    },
     { 0, 239, -100.0                   },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLMARUBOZU ------------------------------------------------------- */
{ "CDLMARUBOZU", 0, { 0 },
  TA_SUCCESS, 10, 242, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   6, -100.0                   },
     { 0,  16, 100.0                    },
     { 0,  80, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 186, -100.0                   },
     { 0, 204, 100.0                    },
     { 0, 239, -100.0                   },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLMATCHINGLOW ---------------------------------------------------- */
{ "CDLMATCHINGLOW", 0, { 0 },
  TA_SUCCESS, 6, 246, 6, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,  23, 100.0                    },
     { 0,  74, 100.0                    },
     { 0, 123, 0.0                      },
     { 0, 128, 100.0                    },
     { 0, 245, 0.0                      },
  } },

/* ---- CDLMATHOLD -------------------------------------------------------- */
{ "CDLMATHOLD", 1, { 0.5 },
  TA_SUCCESS, 14, 238, 3, {   /* default: Penetration=0.5 */
     { 0,   0, 0.0                      },
     { 0, 119, 0.0                      },
     { 0, 237, 0.0                      },
  } },
{ "CDLMATHOLD", 1, { 0.75 },
  TA_SUCCESS, 14, 238, 3, {   /* alt: Penetration=0.75 */
     { 0,   0, 0.0                      },
     { 0, 119, 0.0                      },
     { 0, 237, 0.0                      },
  } },

/* ---- CDLMORNINGDOJISTAR ------------------------------------------------ */
{ "CDLMORNINGDOJISTAR", 1, { 0.29999999999999999 },
  TA_SUCCESS, 12, 240, 3, {   /* default: Penetration=0.29999999999999999 */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },
{ "CDLMORNINGDOJISTAR", 1, { 0.44999999999999996 },
  TA_SUCCESS, 12, 240, 3, {   /* alt: Penetration=0.44999999999999996 */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLMORNINGSTAR ---------------------------------------------------- */
{ "CDLMORNINGSTAR", 1, { 0.29999999999999999 },
  TA_SUCCESS, 12, 240, 4, {   /* default: Penetration=0.29999999999999999 */
     { 0,   0, 0.0                      },
     { 0,  87, 100.0                    },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },
{ "CDLMORNINGSTAR", 1, { 0.44999999999999996 },
  TA_SUCCESS, 12, 240, 4, {   /* alt: Penetration=0.44999999999999996 */
     { 0,   0, 0.0                      },
     { 0,  87, 100.0                    },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLONNECK --------------------------------------------------------- */
{ "CDLONNECK", 0, { 0 },
  TA_SUCCESS, 11, 241, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLPIERCING ------------------------------------------------------- */
{ "CDLPIERCING", 0, { 0 },
  TA_SUCCESS, 11, 241, 4, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 172, 100.0                    },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLRICKSHAWMAN ---------------------------------------------------- */
{ "CDLRICKSHAWMAN", 0, { 0 },
  TA_SUCCESS, 10, 242, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   1, 100.0                    },
     { 0,  78, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 123, 0.0                      },
     { 0, 238, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLRISEFALL3METHODS ----------------------------------------------- */
{ "CDLRISEFALL3METHODS", 0, { 0 },
  TA_SUCCESS, 14, 238, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 119, 0.0                      },
     { 0, 237, 0.0                      },
  } },

/* ---- CDLSEPARATINGLINES ------------------------------------------------ */
{ "CDLSEPARATINGLINES", 0, { 0 },
  TA_SUCCESS, 11, 241, 4, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 177, -100.0                   },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLSHOOTINGSTAR --------------------------------------------------- */
{ "CDLSHOOTINGSTAR", 0, { 0 },
  TA_SUCCESS, 11, 241, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLSHORTLINE ------------------------------------------------------ */
{ "CDLSHORTLINE", 0, { 0 },
  TA_SUCCESS, 10, 242, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   5, 100.0                    },
     { 0,   7, -100.0                   },
     { 0,  86, -100.0                   },
     { 0, 121, 0.0                      },
     { 0, 165, 100.0                    },
     { 0, 232, -100.0                   },
     { 0, 238, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLSPINNINGTOP ---------------------------------------------------- */
{ "CDLSPINNINGTOP", 0, { 0 },
  TA_SUCCESS, 10, 242, 9, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   1, -100.0                   },
     { 0,  14, 100.0                    },
     { 0,  73, -100.0                   },
     { 0, 121, 0.0                      },
     { 0, 144, 100.0                    },
     { 0, 224, -100.0                   },
     { 0, 238, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLSTALLEDPATTERN ------------------------------------------------- */
{ "CDLSTALLEDPATTERN", 0, { 0 },
  TA_SUCCESS, 12, 240, 4, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 118, -100.0                   },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLSTICKSANDWICH -------------------------------------------------- */
{ "CDLSTICKSANDWICH", 0, { 0 },
  TA_SUCCESS, 7, 245, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 122, 0.0                      },
     { 0, 244, 0.0                      },
  } },

/* ---- CDLTAKURI --------------------------------------------------------- */
{ "CDLTAKURI", 0, { 0 },
  TA_SUCCESS, 10, 242, 7, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   4, 100.0                    },
     { 0,  74, 100.0                    },
     { 0, 121, 0.0                      },
     { 0, 122, 0.0                      },
     { 0, 237, 100.0                    },
     { 0, 241, 0.0                      },
  } },

/* ---- CDLTASUKIGAP ------------------------------------------------------ */
{ "CDLTASUKIGAP", 0, { 0 },
  TA_SUCCESS, 7, 245, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 122, 0.0                      },
     { 0, 244, 0.0                      },
  } },

/* ---- CDLTHRUSTING ------------------------------------------------------ */
{ "CDLTHRUSTING", 0, { 0 },
  TA_SUCCESS, 11, 241, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 240, 0.0                      },
  } },

/* ---- CDLTRISTAR -------------------------------------------------------- */
{ "CDLTRISTAR", 0, { 0 },
  TA_SUCCESS, 12, 240, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLUNIQUE3RIVER --------------------------------------------------- */
{ "CDLUNIQUE3RIVER", 0, { 0 },
  TA_SUCCESS, 12, 240, 4, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0,   3, 100.0                    },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLUPSIDEGAP2CROWS ------------------------------------------------ */
{ "CDLUPSIDEGAP2CROWS", 0, { 0 },
  TA_SUCCESS, 12, 240, 3, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 120, 0.0                      },
     { 0, 239, 0.0                      },
  } },

/* ---- CDLXSIDEGAP3METHODS ----------------------------------------------- */
{ "CDLXSIDEGAP3METHODS", 0, { 0 },
  TA_SUCCESS, 2, 250, 5, {   /* default: no parameters */
     { 0,   0, 0.0                      },
     { 0, 124, 0.0                      },
     { 0, 125, 0.0                      },
     { 0, 150, -100.0                   },
     { 0, 249, 0.0                      },
  } },

/* ---- CEIL -------------------------------------------------------------- */
{ "CEIL", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 92.0                     },
     { 0, 126, 131.0                    },
     { 0, 251, 108.0                    },
  } },

/* ---- CMO --------------------------------------------------------------- */
{ "CMO", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, -1.7053206002728454      },
     { 0, 119, 44.071868319639371       },
     { 0, 237, -0.73579585826489025     },
  } },
{ "CMO", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, -4.1650647186979377      },
     { 0, 116, 32.600240413257694       },
     { 0, 232, 0.62919519963876669      },
  } },

/* ---- CORREL ------------------------------------------------------------ */
{ "CORREL", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 0.9630061431354805       },
     { 0, 111, 0.98163857198941229      },
     { 0, 222, 0.9512567157323556       },
  } },
{ "CORREL", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 0.96255105690408804      },
     { 0, 109, 0.98394091161018216      },
     { 0, 217, 0.97517399005452199      },
  } },

/* ---- DEMA -------------------------------------------------------------- */
{ "DEMA", 1, { 30 },
  TA_SUCCESS, 58, 194, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 86.480778269998567       },
     { 0,  97, 125.39145588551717       },
     { 0, 193, 109.1417434640549        },
  } },
{ "DEMA", 1, { 35 },
  TA_SUCCESS, 68, 184, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 89.989731906627142       },
     { 0,  92, 125.90380370228142       },
     { 0, 183, 108.29853496190978       },
  } },

/* ---- DIV --------------------------------------------------------------- */
{ "DIV", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 0.98123324396782841      },
     { 0, 126, 0.97848819838661494      },
     { 0, 251, 0.98511415525114154      },
  } },

/* ---- DX ---------------------------------------------------------------- */
{ "DX", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 19.368900553569542       },
     { 0, 119, 52.616534591083344       },
     { 0, 237, 0.47222724151905549      },
  } },
{ "DX", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 10.933753265810088       },
     { 0, 116, 40.105680780155566       },
     { 0, 232, 1.6860260375380551       },
  } },

/* ---- EMA --------------------------------------------------------------- */
{ "EMA", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 90.426333333333332       },
     { 0, 111, 127.02132157397463       },
     { 0, 222, 107.85795476990275       },
  } },
{ "EMA", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 89.954857142857136       },
     { 0, 109, 126.03787081803146       },
     { 0, 217, 107.70445078798345       },
  } },

/* ---- FLOOR ------------------------------------------------------------- */
{ "FLOOR", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 91.0                     },
     { 0, 126, 131.0                    },
     { 0, 251, 107.0                    },
  } },

/* ---- IMI --------------------------------------------------------------- */
{ "IMI", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 48.083801737353092       },
     { 0, 119, 78.766204738489051       },
     { 0, 238, 36.418359668924047       },
  } },
{ "IMI", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 45.810663764961909       },
     { 0, 117, 67.192660550458726       },
     { 0, 233, 34.692442882249594       },
  } },

/* ---- KAMA -------------------------------------------------------------- */
{ "KAMA", 1, { 30 },
  TA_SUCCESS, 30, 222, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 86.233912864954362       },
     { 0, 111, 128.54110443430994       },
     { 0, 221, 106.08281246764713       },
  } },
{ "KAMA", 1, { 35 },
  TA_SUCCESS, 35, 217, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 88.452421724503139       },
     { 0, 108, 126.48050971422572       },
     { 0, 216, 104.33902618113997       },
  } },

/* ---- LINEARREG --------------------------------------------------------- */
{ "LINEARREG", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 94.332285714285661       },
     { 0, 119, 139.64685714285727       },
     { 0, 238, 108.84628571428561       },
  } },
{ "LINEARREG", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 91.580763157894694       },
     { 0, 117, 139.56515789473696       },
     { 0, 233, 107.01968421052629       },
  } },

/* ---- LINEARREG_ANGLE --------------------------------------------------- */
{ "LINEARREG_ANGLE", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 4.1776910305174297       },
     { 0, 119, 53.182774998761928       },
     { 0, 238, -1.5182974801286493      },
  } },
{ "LINEARREG_ANGLE", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, -9.270034142490049       },
     { 0, 117, 43.933212352225887       },
     { 0, 233, -21.150317456341909      },
  } },

/* ---- LINEARREG_INTERCEPT ----------------------------------------------- */
{ "LINEARREG_INTERCEPT", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 93.382714285714329       },
     { 0, 119, 122.28028571428561       },
     { 0, 238, 109.19085714285721       },
  } },
{ "LINEARREG_INTERCEPT", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 94.518710526315814       },
     { 0, 117, 122.22326315789468       },
     { 0, 233, 113.98347368421051       },
  } },

/* ---- LINEARREG_SLOPE --------------------------------------------------- */
{ "LINEARREG_SLOPE", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 0.073043956043948186     },
     { 0, 119, 1.3358901098901275       },
     { 0, 238, -0.026505494505507664    },
  } },
{ "LINEARREG_SLOPE", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, -0.16321929824561748     },
     { 0, 117, 0.96343859649123686      },
     { 0, 233, -0.3868771929824566      },
  } },

/* ---- MA ---------------------------------------------------------------- */
{ "MA", 2, { 30, 0 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30, MAType=0 */
     { 0,   0, 90.426333333333332       },
     { 0, 111, 127.78899999999997       },
     { 0, 222, 108.42366666666662       },
  } },
{ "MA", 2, { 35, 1 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35, MAType=1 */
     { 0,   0, 89.954857142857136       },
     { 0, 109, 126.03787081803146       },
     { 0, 217, 107.70445078798345       },
  } },

/* ---- MACD -------------------------------------------------------------- */
{ "MACD", 3, { 12, 26, 9 },
  TA_SUCCESS, 33, 219, 9, {   /* default: FastPeriod=12, SlowPeriod=26, SignalPeriod=9 */
     { 0,   0, -1.9738314311425853      },
     { 0, 109, 0.80899202102777679      },
     { 0, 218, 0.90400929950131115      },
     { 1,   0, -2.7071077420416256      },
     { 1, 109, 2.3917207283433704       },
     { 1, 218, 1.4045942301721954       },
     { 2,   0, 0.73327631089904033      },
     { 2, 109, -1.5827287073155936      },
     { 2, 218, -0.50058493067088428     },
  } },
{ "MACD", 3, { 17, 31, 14 },
  TA_SUCCESS, 43, 209, 9, {   /* alt: FastPeriod=17, SlowPeriod=31, SignalPeriod=14 */
     { 0,   0, -1.4158491192931706      },
     { 0, 104, 0.11748121083766705      },
     { 0, 208, 0.93735776099732959      },
     { 1,   0, -2.1639464994938717      },
     { 1, 104, 2.0541055790082861       },
     { 1, 208, 1.0419405270760878       },
     { 2,   0, 0.74809738020070116      },
     { 2, 104, -1.936624368170619       },
     { 2, 208, -0.10458276607875816     },
  } },

/* ---- MACDEXT ----------------------------------------------------------- */
{ "MACDEXT", 6, { 12, 0, 26, 0, 9, 0 },
  TA_SUCCESS, 33, 219, 9, {   /* default: FastPeriod=12, FastMAType=0, SlowPeriod=26, SlowMAType=0, SignalPeriod=9, SignalMAType=0 */
     { 0,   0, -3.3537820512820247      },
     { 0, 109, 0.85346153846160178      },
     { 0, 218, -0.16673076923072472     },
     { 1,   0, -3.5571331908831811      },
     { 1, 109, 4.5708262108262812       },
     { 1, 218, 1.967713675213719        },
     { 2,   0, 0.20335113960115647      },
     { 2, 109, -3.7173646723646794      },
     { 2, 218, -2.1344444444444437      },
  } },
{ "MACDEXT", 6, { 17, 1, 31, 1, 14, 1 },
  TA_SUCCESS, 43, 209, 9, {   /* alt: FastPeriod=17, FastMAType=1, SlowPeriod=31, SlowMAType=1, SignalPeriod=14, SignalMAType=1 */
     { 0,   0, -1.4158491192931706      },
     { 0, 104, 0.11748121083766705      },
     { 0, 208, 0.93735776099732959      },
     { 1,   0, -2.1639464994938717      },
     { 1, 104, 2.0541055790082861       },
     { 1, 208, 1.0419405270760878       },
     { 2,   0, 0.74809738020070116      },
     { 2, 104, -1.936624368170619       },
     { 2, 208, -0.10458276607875816     },
  } },

/* ---- MACDFIX ----------------------------------------------------------- */
{ "MACDFIX", 1, { 9 },
  TA_SUCCESS, 33, 219, 9, {   /* default: SignalPeriod=9 */
     { 0,   0, -1.9519080159730606      },
     { 0, 109, 0.7933856862779578       },
     { 0, 218, 0.87683755130029795      },
     { 1,   0, -2.6864648571209497      },
     { 1, 109, 2.2977711738777948       },
     { 1, 218, 1.3532906188355625       },
     { 2,   0, 0.73455684114788911      },
     { 2, 109, -1.504385487599837       },
     { 2, 218, -0.47645306753526451     },
  } },
{ "MACDFIX", 1, { 14 },
  TA_SUCCESS, 38, 214, 9, {   /* alt: SignalPeriod=14 */
     { 0,   0, -1.655657667612445       },
     { 0, 107, -0.16209494945300662     },
     { 0, 213, 0.87683755130029795      },
     { 1,   0, -2.3025697033524497      },
     { 1, 107, 1.8903514901383578       },
     { 1, 213, 1.4009314039322047       },
     { 2,   0, 0.64691203574000467      },
     { 2, 107, -2.0524464395913644      },
     { 2, 213, -0.52409385263190678     },
  } },

/* ---- MAMA -------------------------------------------------------------- */
{ "MAMA", 2, { 0.5, 0.050000000000000003 },
  TA_SUCCESS, 32, 220, 6, {   /* default: FastLimit=0.5, SlowLimit=0.050000000000000003 */
     { 0,   0, 85.361871216994729       },
     { 0, 110, 126.14383735934065       },
     { 0, 219, 109.28725104217645       },
     { 1,   0, 81.348514544263026       },
     { 1, 110, 128.327671967756         },
     { 1, 219, 108.55415389370368       },
  } },
{ "MAMA", 2, { 0.75, 0.075000000000000011 },
  TA_SUCCESS, 32, 220, 6, {   /* alt: FastLimit=0.75, SlowLimit=0.075000000000000011 */
     { 0,   0, 85.544683418285786       },
     { 0, 110, 124.3830767248932        },
     { 0, 219, 108.9195259642503        },
     { 1,   0, 84.860894033377562       },
     { 1, 110, 127.85840277656153       },
     { 1, 219, 109.57744893067945       },
  } },

/* ---- MAVP -------------------------------------------------------------- */
{ "MAVP", 3, { 2, 30, 0 },
  TA_SUCCESS, 29, 223, 3, {   /* default: MinPeriod=2, MaxPeriod=30, MAType=0 */
     { 0,   0, 90.426333333333332       },
     { 0, 111, 127.78899999999997       },
     { 0, 222, 108.42366666666662       },
  } },
{ "MAVP", 3, { 7, 35, 1 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: MinPeriod=7, MaxPeriod=35, MAType=1 */
     { 0,   0, 89.954857142857136       },
     { 0, 109, 126.03787081803146       },
     { 0, 217, 107.70445078798345       },
  } },
{ "MAVP", 3, { 2, 200, 0 },
  TA_SUCCESS, 199, 53, 3, {   /* wide-periods: MinPeriod=2, MaxPeriod=200, MAType=0 */
     { 0,   0, 122.9899082568808        },
     { 0,  26, 118.22091743119275       },
     { 0,  52, 113.32449541284421       },
  } },

/* ---- MAX --------------------------------------------------------------- */
{ "MAX", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 98.5                     },
     { 0, 111, 137.88                   },
     { 0, 222, 118.28                   },
  } },
{ "MAX", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 98.5                     },
     { 0, 109, 137.88                   },
     { 0, 217, 118.28                   },
  } },

/* ---- MAXINDEX ---------------------------------------------------------- */
{ "MAXINDEX", 1, { 30 },
  TA_SUCCESS, 29, 223, 7, {   /* default: TimePeriod=30 */
     { 0,   0, 12.0                     },
     { 0,   6, 12.0                     },
     { 0,  12, 12.0                     },
     { 0, 111, 131.0                    },
     { 0, 206, 235.0                    },
     { 0, 214, 235.0                    },
     { 0, 222, 235.0                    },
  } },
{ "MAXINDEX", 1, { 35 },
  TA_SUCCESS, 34, 218, 7, {   /* alt: TimePeriod=35 */
     { 0,   0, 12.0                     },
     { 0,   6, 12.0                     },
     { 0,  12, 12.0                     },
     { 0, 109, 131.0                    },
     { 0, 201, 235.0                    },
     { 0, 209, 235.0                    },
     { 0, 217, 235.0                    },
  } },

/* ---- MEDPRICE ---------------------------------------------------------- */
{ "MEDPRICE", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 92.0                     },
     { 0, 126, 132.345                  },
     { 0, 251, 108.06                   },
  } },

/* ---- MFI --------------------------------------------------------------- */
{ "MFI", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 42.892339191984441       },
     { 0, 119, 78.780045284268795       },
     { 0, 237, 53.199678850628665       },
  } },
{ "MFI", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 43.30472306792165        },
     { 0, 116, 68.641625309093286       },
     { 0, 232, 52.15830810481372        },
  } },

/* ---- MIDPOINT ---------------------------------------------------------- */
{ "MIDPOINT", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 94.1875                  },
     { 0, 119, 130.22                   },
     { 0, 238, 108.56                   },
  } },
{ "MIDPOINT", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 93.827500000000001       },
     { 0, 117, 130.22                   },
     { 0, 233, 112.64                   },
  } },

/* ---- MIDPRICE ---------------------------------------------------------- */
{ "MIDPRICE", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 94.219999999999999       },
     { 0, 119, 130.375                  },
     { 0, 238, 108.625                  },
  } },
{ "MIDPRICE", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 93.1875                  },
     { 0, 117, 130.375                  },
     { 0, 233, 113.31                   },
  } },

/* ---- MIN --------------------------------------------------------------- */
{ "MIN", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 81.375                   },
     { 0, 111, 115.5                    },
     { 0, 222, 98.0                     },
  } },
{ "MIN", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 81.375                   },
     { 0, 109, 114.31                   },
     { 0, 217, 93.75                    },
  } },

/* ---- MININDEX ---------------------------------------------------------- */
{ "MININDEX", 1, { 30 },
  TA_SUCCESS, 29, 223, 5, {   /* default: TimePeriod=30 */
     { 0,   0, 25.0                     },
     { 0,  13, 25.0                     },
     { 0,  25, 25.0                     },
     { 0, 111, 111.0                    },
     { 0, 222, 222.0                    },
  } },
{ "MININDEX", 1, { 35 },
  TA_SUCCESS, 34, 218, 6, {   /* alt: TimePeriod=35 */
     { 0,   0, 25.0                     },
     { 0,  13, 25.0                     },
     { 0,  25, 25.0                     },
     { 0, 109, 110.0                    },
     { 0, 216, 221.0                    },
     { 0, 217, 221.0                    },
  } },

/* ---- MINMAX ------------------------------------------------------------ */
{ "MINMAX", 1, { 30 },
  TA_SUCCESS, 29, 223, 6, {   /* default: TimePeriod=30 */
     { 0,   0, 81.375                   },
     { 0, 111, 115.5                    },
     { 0, 222, 98.0                     },
     { 1,   0, 98.5                     },
     { 1, 111, 137.88                   },
     { 1, 222, 118.28                   },
  } },
{ "MINMAX", 1, { 35 },
  TA_SUCCESS, 34, 218, 6, {   /* alt: TimePeriod=35 */
     { 0,   0, 81.375                   },
     { 0, 109, 114.31                   },
     { 0, 217, 93.75                    },
     { 1,   0, 98.5                     },
     { 1, 109, 137.88                   },
     { 1, 217, 118.28                   },
  } },

/* ---- MINMAXINDEX ------------------------------------------------------- */
{ "MINMAXINDEX", 1, { 30 },
  TA_SUCCESS, 29, 223, 12, {   /* default: TimePeriod=30 */
     { 0,   0, 25.0                     },
     { 0,  13, 25.0                     },
     { 0,  25, 25.0                     },
     { 0, 111, 111.0                    },
     { 0, 222, 222.0                    },
     { 1,   0, 12.0                     },
     { 1,   6, 12.0                     },
     { 1,  12, 12.0                     },
     { 1, 111, 131.0                    },
     { 1, 206, 235.0                    },
     { 1, 214, 235.0                    },
     { 1, 222, 235.0                    },
  } },
{ "MINMAXINDEX", 1, { 35 },
  TA_SUCCESS, 34, 218, 13, {   /* alt: TimePeriod=35 */
     { 0,   0, 25.0                     },
     { 0,  13, 25.0                     },
     { 0,  25, 25.0                     },
     { 0, 109, 110.0                    },
     { 0, 216, 221.0                    },
     { 0, 217, 221.0                    },
     { 1,   0, 12.0                     },
     { 1,   6, 12.0                     },
     { 1,  12, 12.0                     },
     { 1, 109, 131.0                    },
     { 1, 201, 235.0                    },
     { 1, 209, 235.0                    },
     { 1, 217, 235.0                    },
  } },

/* ---- MINUS_DI ---------------------------------------------------------- */
{ "MINUS_DI", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 30.168496492833185       },
     { 0, 119, 9.8710449414645822       },
     { 0, 237, 21.198823355925374       },
  } },
{ "MINUS_DI", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 26.847986141186659       },
     { 0, 116, 12.288358962316218       },
     { 0, 232, 21.931097487509426       },
  } },

/* ---- MINUS_DM ---------------------------------------------------------- */
{ "MINUS_DM", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 12.995000000000005       },
     { 0, 119, 3.403129261000382        },
     { 0, 238, 9.6775453701607042       },
  } },
{ "MINUS_DM", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 17.219999999999999       },
     { 0, 117, 8.3709487684756301       },
     { 0, 233, 14.297296909810765       },
  } },

/* ---- MOM --------------------------------------------------------------- */
{ "MOM", 1, { 10 },
  TA_SUCCESS, 10, 242, 3, {   /* default: TimePeriod=10 */
     { 0,   0, 4.625                    },
     { 0, 121, 15.319999999999993       },
     { 0, 241, -1.3199999999999932      },
  } },
{ "MOM", 1, { 15 },
  TA_SUCCESS, 15, 237, 3, {   /* alt: TimePeriod=15 */
     { 0,   0, 1.3149999999999977       },
     { 0, 118, 13.370000000000005       },
     { 0, 236, -5.5                     },
  } },

/* ---- MULT -------------------------------------------------------------- */
{ "MULT", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 8532.375                 },
     { 0, 126, 17538.279999999999       },
     { 0, 251, 11811.765000000001       },
  } },

/* ---- NATR -------------------------------------------------------------- */
{ "NATR", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 3.9321036106750391       },
     { 0, 119, 2.5747286152909599       },
     { 0, 237, 3.0229067221360295       },
  } },
{ "NATR", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 3.7478954688529389       },
     { 0, 116, 2.6633259709231529       },
     { 0, 232, 3.1808220111657537       },
  } },

/* ---- OBV --------------------------------------------------------------- */
{ "OBV", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 4077500.0                },
     { 0, 126, 35517100.0               },
     { 0, 251, -55167600.0              },
  } },

/* ---- PLUS_DI ----------------------------------------------------------- */
{ "PLUS_DI", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 20.378164074412933       },
     { 0, 119, 31.793467589554485       },
     { 0, 237, 20.999551136095821       },
  } },
{ "PLUS_DI", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 21.555651797314859       },
     { 0, 116, 28.745111731995603       },
     { 0, 232, 22.6833079801876         },
  } },

/* ---- PLUS_DM ----------------------------------------------------------- */
{ "PLUS_DM", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 10.280000000000001       },
     { 0, 119, 16.823081230043559       },
     { 0, 238, 9.5865749461878984       },
  } },
{ "PLUS_DM", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 13.530000000000001       },
     { 0, 117, 19.581447644111471       },
     { 0, 233, 14.787677145392683       },
  } },

/* ---- PPO --------------------------------------------------------------- */
{ "PPO", 3, { 12, 26, 1 },
  TA_SUCCESS, 25, 227, 3, {   /* default: FastPeriod=12, SlowPeriod=26, MAType=1 */
     { 0,   0, -4.515896613311364       },
     { 0, 113, 2.0321762887438566       },
     { 0, 226, 0.83644598985405227      },
  } },
{ "PPO", 3, { 17, 31, 2 },
  TA_SUCCESS, 30, 222, 3, {   /* alt: FastPeriod=17, SlowPeriod=31, MAType=2 */
     { 0,   0, -2.5189777859432989      },
     { 0, 111, 0.24878237759049424      },
     { 0, 221, -0.14475000892743711     },
  } },

/* ---- ROC --------------------------------------------------------------- */
{ "ROC", 1, { 10 },
  TA_SUCCESS, 10, 242, 3, {   /* default: TimePeriod=10 */
     { 0,   0, 5.0546448087431584       },
     { 0, 121, 12.5                     },
     { 0, 241, -1.2089019140946955      },
  } },
{ "ROC", 1, { 15 },
  TA_SUCCESS, 15, 237, 3, {   /* alt: TimePeriod=15 */
     { 0,   0, 1.4371584699453432       },
     { 0, 118, 10.875223686351077       },
     { 0, 236, -4.8513716150657142      },
  } },

/* ---- ROCP -------------------------------------------------------------- */
{ "ROCP", 1, { 10 },
  TA_SUCCESS, 10, 242, 3, {   /* default: TimePeriod=10 */
     { 0,   0, 0.050546448087431695     },
     { 0, 121, 0.12499999999999994      },
     { 0, 241, -0.012089019140946912    },
  } },
{ "ROCP", 1, { 15 },
  TA_SUCCESS, 15, 237, 3, {   /* alt: TimePeriod=15 */
     { 0,   0, 0.014371584699453528     },
     { 0, 118, 0.1087522368635107       },
     { 0, 236, -0.048513716150657138    },
  } },

/* ---- ROCR -------------------------------------------------------------- */
{ "ROCR", 1, { 10 },
  TA_SUCCESS, 10, 242, 3, {   /* default: TimePeriod=10 */
     { 0,   0, 1.0505464480874316       },
     { 0, 121, 1.125                    },
     { 0, 241, 0.98791098085905305      },
  } },
{ "ROCR", 1, { 15 },
  TA_SUCCESS, 15, 237, 3, {   /* alt: TimePeriod=15 */
     { 0,   0, 1.0143715846994534       },
     { 0, 118, 1.1087522368635108       },
     { 0, 236, 0.95148628384934286      },
  } },

/* ---- ROCR100 ----------------------------------------------------------- */
{ "ROCR100", 1, { 10 },
  TA_SUCCESS, 10, 242, 3, {   /* default: TimePeriod=10 */
     { 0,   0, 105.05464480874316       },
     { 0, 121, 112.5                    },
     { 0, 241, 98.791098085905304       },
  } },
{ "ROCR100", 1, { 15 },
  TA_SUCCESS, 15, 237, 3, {   /* alt: TimePeriod=15 */
     { 0,   0, 101.43715846994535       },
     { 0, 118, 110.87522368635108       },
     { 0, 236, 95.148628384934284       },
  } },

/* ---- RSI --------------------------------------------------------------- */
{ "RSI", 1, { 14 },
  TA_SUCCESS, 14, 238, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 49.147339699863579       },
     { 0, 119, 72.035934159819675       },
     { 0, 237, 49.632102070867553       },
  } },
{ "RSI", 1, { 19 },
  TA_SUCCESS, 19, 233, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 47.917467640651026       },
     { 0, 116, 66.300120206628847       },
     { 0, 232, 50.314597599819386       },
  } },

/* ---- SAR --------------------------------------------------------------- */
{ "SAR", 2, { 0.02, 0.20000000000000001 },
  TA_SUCCESS, 1, 251, 3, {   /* default: Acceleration=0.02, Maximum=0.20000000000000001 */
     { 0,   0, 90.75                    },
     { 0, 125, 122.59436752238587       },
     { 0, 250, 112.80333920351487       },
  } },
{ "SAR", 2, { 0.029999999999999999, 0.30000000000000004 },
  TA_SUCCESS, 1, 251, 3, {   /* alt: Acceleration=0.029999999999999999, Maximum=0.30000000000000004 */
     { 0,   0, 90.75                    },
     { 0, 125, 126.0248                 },
     { 0, 250, 110.5                    },
  } },

/* ---- SAREXT ------------------------------------------------------------ */
{ "SAREXT", 8, { 0.0, 0.0, 0.02, 0.02, 0.20000000000000001, 0.02, 0.02, 0.20000000000000001 },
  TA_SUCCESS, 1, 251, 3, {   /* default: StartValue=0.0, OffsetOnReverse=0.0, AccelerationInitLong=0.02, AccelerationLong=0.02, AccelerationMaxLong=0.20000000000000001, AccelerationInitShort=0.02, AccelerationShort=0.02, AccelerationMaxShort=0.20000000000000001 */
     { 0,   0, 90.75                    },
     { 0, 125, 122.59436752238587       },
     { 0, 250, -112.80333920351487      },
  } },
{ "SAREXT", 8, { 0.5, 0.5, 0.029999999999999999, 0.029999999999999999, 0.30000000000000004, 0.029999999999999999, 0.029999999999999999, 0.30000000000000004 },
  TA_SUCCESS, 1, 251, 3, {   /* alt: StartValue=0.5, OffsetOnReverse=0.5, AccelerationInitLong=0.029999999999999999, AccelerationLong=0.029999999999999999, AccelerationMaxLong=0.30000000000000004, AccelerationInitShort=0.029999999999999999, AccelerationShort=0.029999999999999999, AccelerationMaxShort=0.30000000000000004 */
     { 0,   0, 0.5                      },
     { 0, 125, -146.69275946050925      },
     { 0, 250, -128.51092131715103      },
  } },

/* ---- SMA --------------------------------------------------------------- */
{ "SMA", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 90.426333333333332       },
     { 0, 111, 127.78899999999997       },
     { 0, 222, 108.42366666666662       },
  } },
{ "SMA", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 89.954857142857136       },
     { 0, 109, 126.94942857142864       },
     { 0, 217, 106.45742857142855       },
  } },

/* ---- SQRT -------------------------------------------------------------- */
{ "SQRT", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 9.5655632348544959       },
     { 0, 126, 11.445523142259598       },
     { 0, 251, 10.3860483341837         },
  } },

/* ---- STDDEV ------------------------------------------------------------ */
{ "STDDEV", 2, { 5, 1.0 },
  TA_SUCCESS, 4, 248, 3, {   /* default: TimePeriod=5, NbDev=1.0 */
     { 0,   0, 1.2856461410533266       },
     { 0, 124, 1.1396385391835246       },
     { 0, 247, 0.7143556537094935       },
  } },
{ "STDDEV", 2, { 10, 1.5 },
  TA_SUCCESS, 9, 243, 3, {   /* alt: TimePeriod=10, NbDev=1.5 */
     { 0,   0, 2.2418814782459453       },
     { 0, 121, 6.9584700904741066       },
     { 0, 242, 1.1493720024360499       },
  } },

/* ---- STOCH ------------------------------------------------------------- */
{ "STOCH", 5, { 5, 3, 0, 3, 0 },
  TA_SUCCESS, 8, 244, 6, {   /* default: FastK_Period=5, SlowK_Period=3, SlowK_MAType=0, SlowD_Period=3, SlowD_MAType=0 */
     { 0,   0, 24.012183760036368       },
     { 0, 122, 89.442072783260755       },
     { 0, 243, 30.194784270958792       },
     { 1,   0, 36.254789455084072       },
     { 1, 122, 86.576879027781914       },
     { 1, 243, 43.694644908807732       },
  } },
{ "STOCH", 5, { 10, 8, 1, 8, 1 },
  TA_SUCCESS, 23, 229, 6, {   /* alt: FastK_Period=10, SlowK_Period=8, SlowK_MAType=1, SlowD_Period=8, SlowD_MAType=1 */
     { 0,   0, 18.509439805132899       },
     { 0, 114, 52.79345995690614        },
     { 0, 228, 49.189268471702469       },
     { 1,   0, 35.014763145155925       },
     { 1, 114, 72.010020568852468       },
     { 1, 228, 48.591626751270987       },
  } },

/* ---- STOCHF ------------------------------------------------------------ */
{ "STOCHF", 3, { 5, 3, 0 },
  TA_SUCCESS, 6, 246, 6, {   /* default: FastK_Period=5, FastD_Period=3, FastD_MAType=0 */
     { 0,   0, 12.114285714285741       },
     { 0, 123, 99.244332493702757       },
     { 0, 245, 30.266343825665892       },
     { 1,   0, 43.589894925106201       },
     { 1, 123, 89.33100602927145        },
     { 1, 245, 30.194784270958792       },
  } },
{ "STOCHF", 3, { 10, 8, 1 },
  TA_SUCCESS, 16, 236, 6, {   /* alt: FastK_Period=10, FastD_Period=8, FastD_MAType=1 */
     { 0,   0, 18.679611650485445       },
     { 0, 118, 69.659442724458231       },
     { 0, 235, 28.15315315315317        },
     { 1,   0, 51.790513553831218       },
     { 1, 118, 82.151532183999421       },
     { 1, 235, 49.189268471702469       },
  } },

/* ---- SUB --------------------------------------------------------------- */
{ "SUB", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, -1.75                    },
     { 0, 126, -2.8799999999999955      },
     { 0, 251, -1.6299999999999955      },
  } },

/* ---- SUM --------------------------------------------------------------- */
{ "SUM", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 2712.79                  },
     { 0, 111, 3833.6699999999992       },
     { 0, 222, 3252.7099999999987       },
  } },
{ "SUM", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 3148.4199999999996       },
     { 0, 109, 4443.2300000000023       },
     { 0, 217, 3726.0099999999993       },
  } },

/* ---- T3 ---------------------------------------------------------------- */
{ "T3", 2, { 5, 0.69999999999999996 },
  TA_SUCCESS, 24, 228, 3, {   /* default: TimePeriod=5, VFactor=0.69999999999999996 */
     { 0,   0, 85.729875673717913       },
     { 0, 114, 132.08334163347217       },
     { 0, 227, 108.87915000449271       },
  } },
{ "T3", 2, { 10, 0.19999999999999996 },
  TA_SUCCESS, 54, 198, 3, {   /* alt: TimePeriod=10, VFactor=0.19999999999999996 */
     { 0,   0, 88.101761728618953       },
     { 0,  99, 125.32527601230549       },
     { 0, 197, 109.32589889108408       },
  } },

/* ---- TEMA -------------------------------------------------------------- */
{ "TEMA", 1, { 30 },
  TA_SUCCESS, 87, 165, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 107.49479711901382       },
     { 0,  82, 123.88000383913092       },
     { 0, 164, 111.44001466843056       },
  } },
{ "TEMA", 1, { 35 },
  TA_SUCCESS, 102, 150, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 118.49683849064921       },
     { 0,  75, 130.33875790675987       },
     { 0, 149, 111.2320640987503        },
  } },

/* ---- TRANGE ------------------------------------------------------------ */
{ "TRANGE", 0, { 0 },
  TA_SUCCESS, 1, 251, 3, {   /* default: no parameters */
     { 0,   0, 3.5349999999999966       },
     { 0, 125, 3.0699999999999932       },
     { 0, 250, 2.8799999999999955       },
  } },

/* ---- TRIMA ------------------------------------------------------------- */
{ "TRIMA", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 90.961083333333349       },
     { 0, 111, 129.84841666666662       },
     { 0, 222, 109.69416666666655       },
  } },
{ "TRIMA", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 89.872283950617287       },
     { 0, 109, 129.38259259259274       },
     { 0, 217, 108.45666666666706       },
  } },

/* ---- TRIX -------------------------------------------------------------- */
{ "TRIX", 1, { 30 },
  TA_SUCCESS, 88, 164, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 0.23167407033994891      },
     { 0,  82, 0.10393844289935394      },
     { 0, 163, -0.064987193469434601    },
  } },
{ "TRIX", 1, { 35 },
  TA_SUCCESS, 103, 149, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 0.45672794273943218      },
     { 0,  74, 0.15039335437074808      },
     { 0, 148, -0.12557499945300421     },
  } },

/* ---- TSF --------------------------------------------------------------- */
{ "TSF", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, 94.405329670329607       },
     { 0, 119, 140.9827472527474        },
     { 0, 238, 108.8197802197801        },
  } },
{ "TSF", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, 91.417543859649086       },
     { 0, 117, 140.52859649122817       },
     { 0, 233, 106.63280701754384       },
  } },

/* ---- TYPPRICE ---------------------------------------------------------- */
{ "TYPPRICE", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 91.833333333333329       },
     { 0, 126, 131.89666666666668       },
     { 0, 251, 107.99666666666667       },
  } },

/* ---- ULTOSC ------------------------------------------------------------ */
{ "ULTOSC", 3, { 7, 14, 28 },
  TA_SUCCESS, 28, 224, 3, {   /* default: TimePeriod1=7, TimePeriod2=14, TimePeriod3=28 */
     { 0,   0, 47.171335893612849       },
     { 0, 112, 37.65236554762523        },
     { 0, 223, 40.08540297699178        },
  } },
{ "ULTOSC", 3, { 12, 19, 33 },
  TA_SUCCESS, 33, 219, 3, {   /* alt: TimePeriod1=12, TimePeriod2=19, TimePeriod3=33 */
     { 0,   0, 48.371551360549795       },
     { 0, 109, 47.507885713593886       },
     { 0, 218, 49.428436753229477       },
  } },

/* ---- VAR --------------------------------------------------------------- */
{ "VAR", 2, { 5, 1.0 },
  TA_SUCCESS, 4, 248, 3, {   /* default: TimePeriod=5, NbDev=1.0 */
     { 0,   0, 1.6528860000053101       },
     { 0, 124, 1.2987759999923583       },
     { 0, 247, 0.51030399998671783      },
  } },
{ "VAR", 2, { 10, 1.5 },
  TA_SUCCESS, 9, 243, 3, {   /* alt: TimePeriod=10, NbDev=1.5 */
     { 0,   0, 2.2337922500009881       },
     { 0, 121, 21.520136000010098       },
     { 0, 242, 0.58713599999282451      },
  } },

/* ---- WCLPRICE ---------------------------------------------------------- */
{ "WCLPRICE", 0, { 0 },
  TA_SUCCESS, 0, 252, 3, {   /* default: no parameters */
     { 0,   0, 91.75                    },
     { 0, 126, 131.67250000000001       },
     { 0, 251, 107.965                  },
  } },

/* ---- WILLR ------------------------------------------------------------- */
{ "WILLR", 1, { 14 },
  TA_SUCCESS, 13, 239, 3, {   /* default: TimePeriod=14 */
     { 0,   0, -90.194264569842716      },
     { 0, 119, -11.003970504821318      },
     { 0, 238, -59.151515151515092      },
  } },
{ "WILLR", 1, { 19 },
  TA_SUCCESS, 18, 234, 3, {   /* alt: TimePeriod=19 */
     { 0,   0, -62.135922330097088      },
     { 0, 117, -25.865002836074893      },
     { 0, 233, -80.874006810442651      },
  } },

/* ---- WMA --------------------------------------------------------------- */
{ "WMA", 1, { 30 },
  TA_SUCCESS, 29, 223, 3, {   /* default: TimePeriod=30 */
     { 0,   0, 88.567709677419359       },
     { 0, 111, 130.06165591397846       },
     { 0, 222, 109.34129032258079       },
  } },
{ "WMA", 1, { 35 },
  TA_SUCCESS, 34, 218, 3, {   /* alt: TimePeriod=35 */
     { 0,   0, 88.203269841269844       },
     { 0, 109, 129.22063492063506       },
     { 0, 217, 108.76730158730153       },
  } },
};

#define TA_LEGACY_NB_CASE ((int)(sizeof(TA_LEGACY_CASE)/sizeof(TA_LEGACY_CASE[0])))

#endif
