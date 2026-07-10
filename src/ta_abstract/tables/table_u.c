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
 * This file contains only TA functions starting with the letter 'U' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* ULTOSC BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_ULTOSC_TimePeriod1 =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod1",
   0,

   "First Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   7,
   "Number of bars for 1st period.",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_ULTOSC_TimePeriod2 =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod2",
   0,

   "Second Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   14,
   "Number of bars fro 2nd period",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_ULTOSC_TimePeriod3 =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod3",
   0,

   "Third Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   28,
   "Number of bars for 3rd period",

   NULL
};

static const TA_InputParameterInfo    *TA_ULTOSC_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ULTOSC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ULTOSC_OptInputs[] =
{ &TA_DEF_UI_D_ULTOSC_TimePeriod1,
  &TA_DEF_UI_D_ULTOSC_TimePeriod2,
  &TA_DEF_UI_D_ULTOSC_TimePeriod3,
  NULL
};

DEF_FUNCTION( ULTOSC,
              TA_GroupId_MomentumIndicators,
              "Ultimate Oscillator",
              "UltOsc",
              TA_FUNC_FLG_STREAM
             );
/* ULTOSC END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableU[] =
{
   ADD_TO_TABLE(ULTOSC),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableUSize =
              ((sizeof(TA_DEF_TableU)/sizeof(TA_FuncDef *))-1);

