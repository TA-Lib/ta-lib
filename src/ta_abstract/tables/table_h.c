/*********************************************************************
 * This file contains only TA functions starting with the letter 'H' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* Follow the 4 steps defined below for adding a new TA Function to this
 * file.
 */

/***************************************************************************
 * Step 1 - Define user inputs that are particular to your function.
 *          Consider the ones already defined in "ta_def_ui.c".
 ***************************************************************************/

/* None */

/****************************************************************************
 * Step 2 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* HT_DCPERIOD */
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
{ 
  NULL
};

DEF_FUNCTION( HT_DCPERIOD,                   /* name */
              TA_GroupId_CycleIndicators,  /* groupId */
              "Hilbert Transform - Dominant Cycle Period",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER, /* flags */
              NULL    /* analysis function */
             );
/* HT_DCPERIOD END */

/* HT_DCPHASE */
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
{ 
  NULL
};

DEF_FUNCTION( HT_DCPHASE,                   /* name */
              TA_GroupId_CycleIndicators,  /* groupId */
              "Hilbert Transform - Dominant Cycle Phase",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER, /* flags */
              NULL    /* analysis function */
             );
/* HT_DCPHASE END */

/* HT_PHASOR */
const TA_OutputParameterInfo TA_DEF_UI_Output_Real_InPhase =
                               { TA_Output_Real, "outInPhase", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_Quadrature =
                               { TA_Output_Real, "outQuadrature", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_HT_PHASOR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_PHASOR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_InPhase,
  &TA_DEF_UI_Output_Real_Quadrature,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_PHASOR_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( HT_PHASOR,                   /* name */
              TA_GroupId_CycleIndicators,  /* groupId */
              "Hilbert Transform - Phasor Components",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER, /* flags */
              NULL    /* analysis function */
             );
/* HT_PHASOR END */

/* HT_SINE */
const TA_OutputParameterInfo TA_DEF_UI_Output_Real_Sine =
                               { TA_Output_Real, "outSine", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_LeadSine =
                               { TA_Output_Real, "outLeadSine", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_HT_SINE_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_HT_SINE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_Sine,
  &TA_DEF_UI_Output_Real_LeadSine,
  NULL
};

static const TA_OptInputParameterInfo *TA_HT_SINE_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( HT_SINE,                   /* name */
              TA_GroupId_CycleIndicators,  /* groupId */
              "Hilbert Transform - SineWave",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER, /* flags */
              NULL    /* analysis function */
             );
/* HT_SINE END */

/* HT_TRENDLINE */
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
{ 
  NULL
};

DEF_FUNCTION( HT_TRENDLINE,                   /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Hilbert Transform - Instantaneous Trendline",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER|TA_FUNC_FLG_OVERLAP, /* flags */
              NULL    /* analysis function */
             );
/* HT_TRENDLINE END */

/* HT_TRENDMODE */
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
{ 
  NULL
};

DEF_FUNCTION( HT_TRENDMODE,                   /* name */
              TA_GroupId_CycleIndicators,  /* groupId */
              "Hilbert Transform - Trend vs Cycle Mode",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER, /* flags */
              NULL    /* analysis function */
             );
/* HT_TRENDMODE END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
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


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
