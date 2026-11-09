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
 * This file contains only TA functions starting with the letter 'G' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* GATOR BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_GATOR_JawPeriod =
{
   TA_OptInput_IntegerRange,
   "optInJawPeriod",
   0,

   "Jaw Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   13,
   "Time period for the jaw line",

   NULL
};

static const TA_IntegerRange TA_DEF_GATOR_JawShift =
{
   0,
   100000,
   0,
   30,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_GATOR_JawShift =
{
   TA_OptInput_IntegerRange,
   "optInJawShift",
   0,

   "Jaw Shift",
   (const void *)&TA_DEF_GATOR_JawShift,
   8,
   "Bars ahead the jaw line is drawn",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_GATOR_TeethPeriod =
{
   TA_OptInput_IntegerRange,
   "optInTeethPeriod",
   0,

   "Teeth Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   8,
   "Time period for the teeth line",

   NULL
};

static const TA_IntegerRange TA_DEF_GATOR_TeethShift =
{
   0,
   100000,
   0,
   30,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_GATOR_TeethShift =
{
   TA_OptInput_IntegerRange,
   "optInTeethShift",
   0,

   "Teeth Shift",
   (const void *)&TA_DEF_GATOR_TeethShift,
   5,
   "Bars ahead the teeth line is drawn",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_GATOR_LipsPeriod =
{
   TA_OptInput_IntegerRange,
   "optInLipsPeriod",
   0,

   "Lips Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   5,
   "Time period for the lips line",

   NULL
};

static const TA_IntegerRange TA_DEF_GATOR_LipsShift =
{
   0,
   100000,
   0,
   30,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_GATOR_LipsShift =
{
   TA_OptInput_IntegerRange,
   "optInLipsShift",
   0,

   "Lips Shift",
   (const void *)&TA_DEF_GATOR_LipsShift,
   3,
   "Bars ahead the lips line is drawn",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_GATOR_outUpper =
                               { TA_Output_Real, "outUpper", TA_OUT_HISTO | TA_OUT_DISPLAY_SHIFT | TA_OUT_ZERO | TA_OUT_POSITIVE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_GATOR_outLower =
                               { TA_Output_Real, "outLower", TA_OUT_HISTO | TA_OUT_DISPLAY_SHIFT | TA_OUT_ZERO | TA_OUT_NEGATIVE };

static const TA_InputParameterInfo    *TA_GATOR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_GATOR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_GATOR_outUpper,
  &TA_DEF_UI_Output_Real_GATOR_outLower,
  NULL
};

static const TA_OptInputParameterInfo *TA_GATOR_OptInputs[] =
{ &TA_DEF_UI_D_GATOR_JawPeriod,
  &TA_DEF_UI_D_GATOR_JawShift,
  &TA_DEF_UI_D_GATOR_TeethPeriod,
  &TA_DEF_UI_D_GATOR_TeethShift,
  &TA_DEF_UI_D_GATOR_LipsPeriod,
  &TA_DEF_UI_D_GATOR_LipsShift,
  NULL
};

DEF_FUNCTION( GATOR,
              TA_GroupId_MomentumIndicators,
              "Gator Oscillator",
              TA_FUNC_FLG_STREAM | TA_FUNC_FLG_DISPLAY_SHIFT
             );
/* GATOR END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableG[] =
{
   ADD_TO_TABLE(GATOR),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableGSize =
              ((sizeof(TA_DEF_TableG)/sizeof(TA_FuncDef *))-1);

