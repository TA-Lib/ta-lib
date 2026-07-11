/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 * This file contains only TA functions starting with the letter 'B' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* BBANDS BEGIN */
static const TA_RealRange TA_DEF_BBANDS_NbDevUp =
{
   TA_REAL_MIN,
   TA_REAL_MAX,
   2,
   -2.0,
   2.0,
   0.2
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_BBANDS_NbDevUp =
{
   TA_OptInput_RealRange,
   "optInNbDevUp",
   0,

   "Deviations up",
   (const void *)&TA_DEF_BBANDS_NbDevUp,
   2.0,
   "Deviation multiplier for upper band",

   NULL
};

static const TA_RealRange TA_DEF_BBANDS_NbDevDn =
{
   TA_REAL_MIN,
   TA_REAL_MAX,
   2,
   -2.0,
   2.0,
   0.2
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_BBANDS_NbDevDn =
{
   TA_OptInput_RealRange,
   "optInNbDevDn",
   0,

   "Deviations down",
   (const void *)&TA_DEF_BBANDS_NbDevDn,
   2.0,
   "Deviation multiplier for lower band",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_BBANDS_UpperBand =
                               { TA_Output_Real, "outRealUpperBand", TA_OUT_UPPER_LIMIT };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_BBANDS_MiddleBand =
                               { TA_Output_Real, "outRealMiddleBand", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_BBANDS_LowerBand =
                               { TA_Output_Real, "outRealLowerBand", TA_OUT_LOWER_LIMIT };

static const TA_InputParameterInfo    *TA_BBANDS_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_BBANDS_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_BBANDS_UpperBand,
  &TA_DEF_UI_Output_Real_BBANDS_MiddleBand,
  &TA_DEF_UI_Output_Real_BBANDS_LowerBand,
  NULL
};

static const TA_OptInputParameterInfo *TA_BBANDS_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_5_MINIMUM2,
  &TA_DEF_UI_D_BBANDS_NbDevUp,
  &TA_DEF_UI_D_BBANDS_NbDevDn,
  &TA_DEF_UI_MA_Method,
  NULL
};

DEF_FUNCTION( BBANDS,
              TA_GroupId_OverlapStudies,
              "Bollinger Bands",
              "Bbands",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* BBANDS END */

/* BETA BEGIN */
static const TA_InputParameterInfo    *TA_BETA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real0,
  &TA_DEF_UI_Input_Real1,
  NULL
};

static const TA_OutputParameterInfo   *TA_BETA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_BETA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_5,
  NULL
};

DEF_FUNCTION( BETA,
              TA_GroupId_Statistic,
              "Beta",
              "Beta",
              TA_FUNC_FLG_STREAM
             );
/* BETA END */

/* BOP BEGIN */
static const TA_InputParameterInfo    *TA_BOP_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_BOP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_BOP_OptInputs[] =
{ NULL };

DEF_FUNCTION( BOP,
              TA_GroupId_MomentumIndicators,
              "Balance Of Power",
              "Bop",
              TA_FUNC_FLG_STREAM
             );
/* BOP END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableB[] =
{
   ADD_TO_TABLE(BBANDS),
   ADD_TO_TABLE(BETA),
   ADD_TO_TABLE(BOP),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableBSize =
              ((sizeof(TA_DEF_TableB)/sizeof(TA_FuncDef *))-1);

