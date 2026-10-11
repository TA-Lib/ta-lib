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

/*********************************************************************
 * This file contains only TA functions starting with the letter 'Z' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* ZIGZAG BEGIN */
static const TA_RealRange TA_DEF_ZIGZAG_Sensitivity =
{
   0.0,
   100.0,
   2,
   1.0,
   20.0,
   1.0
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_ZIGZAG_Sensitivity =
{
   TA_OptInput_RealRange,
   "optInSensitivity",
   TA_OPTIN_IS_PERCENT,

   "Sensitivity",
   (const void *)&TA_DEF_ZIGZAG_Sensitivity,
   5.0,
   "Minimum move away from the current extreme that reverses the leg, in percent",

   NULL
};

static const TA_IntegerRange TA_DEF_ZIGZAG_MinTrendLength =
{
   1,
   100000,
   1,
   20,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_ZIGZAG_MinTrendLength =
{
   TA_OptInput_IntegerRange,
   "optInMinTrendLength",
   0,

   "Minimum Trend Length",
   (const void *)&TA_DEF_ZIGZAG_MinTrendLength,
   1,
   "Minimum number of bars between two pivots",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ZIGZAG_outZigZag =
                               { TA_Output_Real, "outZigZag", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Integer_ZIGZAG_outTrend =
                               { TA_Output_Integer, "outTrend", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Integer_ZIGZAG_outPivotIdx =
                               { TA_Output_Integer, "outPivotIdx", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_ZIGZAG_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_ZIGZAG_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_ZIGZAG_outZigZag,
  &TA_DEF_UI_Output_Integer_ZIGZAG_outTrend,
  &TA_DEF_UI_Output_Integer_ZIGZAG_outPivotIdx,
  NULL
};

static const TA_OptInputParameterInfo *TA_ZIGZAG_OptInputs[] =
{ &TA_DEF_UI_D_ZIGZAG_Sensitivity,
  &TA_DEF_UI_D_ZIGZAG_MinTrendLength,
  NULL
};

DEF_FUNCTION( ZIGZAG,
              TA_GroupId_OverlapStudies,
              "Zig Zag",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PATH_DEP
             );
/* ZIGZAG END */

/* ZLEMA BEGIN */
static const TA_InputParameterInfo    *TA_ZLEMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ZLEMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ZLEMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( ZLEMA,
              TA_GroupId_OverlapStudies,
              "Zero-Lag Exponential Moving Average",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PERIOD1_IDENTITY
             );
/* ZLEMA END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableZ[] =
{
   ADD_TO_TABLE(ZIGZAG),
   ADD_TO_TABLE(ZLEMA),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableZSize =
              ((sizeof(TA_DEF_TableZ)/sizeof(TA_FuncDef *))-1);

