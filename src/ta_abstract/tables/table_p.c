/*********************************************************************
 * This file contains only TA functions starting with the letter 'P' *
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

/* PLUS_DI BEGIN */
static const TA_InputParameterInfo    *TA_PLUS_DI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_PLUS_DI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PLUS_DI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( PLUS_DI,                     /* name */
              TA_GroupId_MomentumIndicators,   /* groupId */
              "Plus Directional Indicator", /* hint */
              NULL,                         /* helpFile */
              TA_FUNC_FLG_UNST_PER,         /* flags */
              NULL                          /* analysis function */
             );

/* PLUS_DI END */

/* PLUS_DM BEGIN */
static const TA_InputParameterInfo    *TA_PLUS_DM_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_PLUS_DM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PLUS_DM_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( PLUS_DM,                     /* name */
              TA_GroupId_MomentumIndicators,   /* groupId */
              "Plus Directional Movement", /* hint */
              NULL,                         /* helpFile */
              TA_FUNC_FLG_UNST_PER,         /* flags */
              NULL                          /* analysis function */
             );

/* PLUS_DM END */

/* PPO BEGIN */
static const TA_InputParameterInfo *TA_PPO_Inputs[] =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_PPO_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PPO_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_MA_Method,
  NULL
};

DEF_FUNCTION( PPO,                        /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Percentage Price Oscillator", /* hint */
              NULL,                       /* helpFile */
              0,                          /* flags */
              NULL                        /* analysis function */
             );
/* PPO END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableP[] =
{
   ADD_TO_TABLE(PPO),
   ADD_TO_TABLE(PLUS_DI),
   ADD_TO_TABLE(PLUS_DM),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TablePSize =
              ((sizeof(TA_DEF_TableP)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
