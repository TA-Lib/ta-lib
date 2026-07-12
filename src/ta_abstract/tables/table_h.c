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
 * This file contains only TA functions starting with the letter 'H' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* HT_DCPERIOD BEGIN */
static const TA_InputParameterInfo    *TA_HT_DCPERIOD_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_DCPERIOD_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_DCPERIOD_OptInputs[] =
{ NULL };

DEF_FUNCTION( HT_DCPERIOD,
              TA_GroupId_CycleIndicators,
              "Hilbert Transform - Dominant Cycle Period",
              "HtDcPeriod",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* HT_DCPERIOD END */

/* HT_DCPHASE BEGIN */
static const TA_InputParameterInfo    *TA_HT_DCPHASE_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_DCPHASE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_DCPHASE_OptInputs[] =
{ NULL };

DEF_FUNCTION( HT_DCPHASE,
              TA_GroupId_CycleIndicators,
              "Hilbert Transform - Dominant Cycle Phase",
              "HtDcPhase",
              TA_FUNC_FLG_UNST_PER
             );
/* HT_DCPHASE END */

/* HT_PHASOR BEGIN */
const TA_OutputParameterInfo TA_DEF_UI_Output_Real_HT_PHASOR_outInPhase =
                               { TA_Output_Real, "outInPhase", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_HT_PHASOR_outQuadrature =
                               { TA_Output_Real, "outQuadrature", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_HT_PHASOR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_PHASOR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_HT_PHASOR_outInPhase,
  &TA_DEF_UI_Output_Real_HT_PHASOR_outQuadrature,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_PHASOR_OptInputs[] =
{ NULL };

DEF_FUNCTION( HT_PHASOR,
              TA_GroupId_CycleIndicators,
              "Hilbert Transform - Phasor Components",
              "HtPhasor",
              TA_FUNC_FLG_UNST_PER
             );
/* HT_PHASOR END */

/* HT_SINE BEGIN */
const TA_OutputParameterInfo TA_DEF_UI_Output_Real_HT_SINE_outSine =
                               { TA_Output_Real, "outSine", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_HT_SINE_outLeadSine =
                               { TA_Output_Real, "outLeadSine", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_HT_SINE_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_SINE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_HT_SINE_outSine,
  &TA_DEF_UI_Output_Real_HT_SINE_outLeadSine,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_SINE_OptInputs[] =
{ NULL };

DEF_FUNCTION( HT_SINE,
              TA_GroupId_CycleIndicators,
              "Hilbert Transform - SineWave",
              "HtSine",
              TA_FUNC_FLG_UNST_PER
             );
/* HT_SINE END */

/* HT_TRENDLINE BEGIN */
static const TA_InputParameterInfo    *TA_HT_TRENDLINE_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_TRENDLINE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_TRENDLINE_OptInputs[] =
{ NULL };

DEF_FUNCTION( HT_TRENDLINE,
              TA_GroupId_OverlapStudies,
              "Hilbert Transform - Instantaneous Trendline",
              "HtTrendline",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_OVERLAP
             );
/* HT_TRENDLINE END */

/* HT_TRENDMODE BEGIN */
static const TA_InputParameterInfo    *TA_HT_TRENDMODE_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_TRENDMODE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_TRENDMODE_OptInputs[] =
{ NULL };

DEF_FUNCTION( HT_TRENDMODE,
              TA_GroupId_CycleIndicators,
              "Hilbert Transform - Trend vs Cycle Mode",
              "HtTrendMode",
              TA_FUNC_FLG_UNST_PER
             );
/* HT_TRENDMODE END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableH[] =
{
   ADD_TO_TABLE(HT_DCPERIOD),
   ADD_TO_TABLE(HT_DCPHASE),
   ADD_TO_TABLE(HT_PHASOR),
   ADD_TO_TABLE(HT_SINE),
   ADD_TO_TABLE(HT_TRENDLINE),
   ADD_TO_TABLE(HT_TRENDMODE),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableHSize =
              ((sizeof(TA_DEF_TableH)/sizeof(TA_FuncDef *))-1);

