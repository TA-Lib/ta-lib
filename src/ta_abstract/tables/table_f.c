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
 * This file contains only TA functions starting with the letter 'F' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* FLOOR BEGIN */
static const TA_InputParameterInfo    *TA_FLOOR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_FLOOR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_FLOOR_OptInputs[] =
{ NULL };

DEF_FUNCTION( FLOOR,
              TA_GroupId_MathTransform,
              "Vector Floor",
              TA_FUNC_FLG_STREAM
             );
/* FLOOR END */

/* FOSC BEGIN */
static const TA_IntegerRange TA_DEF_FOSC_TimePeriod =
{
   2,
   100000,
   2,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_FOSC_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_FOSC_TimePeriod,
   5,
   "Time period",

   NULL
};

static const TA_InputParameterInfo    *TA_FOSC_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_FOSC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_FOSC_OptInputs[] =
{ &TA_DEF_UI_D_FOSC_TimePeriod,
  NULL
};

DEF_FUNCTION( FOSC,
              TA_GroupId_MomentumIndicators,
              "Forecast Oscillator",
              TA_FUNC_FLG_STREAM
             );
/* FOSC END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableF[] =
{
   ADD_TO_TABLE(FLOOR),
   ADD_TO_TABLE(FOSC),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableFSize =
              ((sizeof(TA_DEF_TableF)/sizeof(TA_FuncDef *))-1);

