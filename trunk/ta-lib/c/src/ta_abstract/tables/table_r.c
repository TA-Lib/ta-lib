/*********************************************************************
 * This file contains only TA functions starting with the letter 'R' *
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

/* ROC BEGIN */
static const TA_InputParameterInfo    *TA_ROC_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ROC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ROC_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( ROC,                     /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Rate of change : ((price/prevPrice)-1)*100", /* hint */
              NULL,             /* helpFile */
              0,                /* flags */
              NULL              /* analysis function */
             );
/* ROC END */

/* ROCR BEGIN */
static const TA_InputParameterInfo    *TA_ROCR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ROCR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ROCR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( ROCR,                    /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Rate of change ratio: (price/prevPrice)*100", /* hint */
              NULL,             /* helpFile */
              0,                /* flags */
              NULL              /* analysis function */
             );
/* ROCR END */

/* RSI BEGIN */
static const TA_InputParameterInfo    *TA_RSI_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_RSI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_RSI_OptInputs[] =
{
  &TA_DEF_UI_TimePeriod_14,
  &TA_DEF_UI_Compatibility_CL_MS,
  NULL
};

DEF_FUNCTION( RSI,                        /* name */
              TA_GroupId_MarketStrength,  /* groupId */
              "Relative Strength Index",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER,       /* flags */
              NULL                        /* analysis function */
             );
/* RSI END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableR[] =
{
   ADD_TO_TABLE(ROC),
   ADD_TO_TABLE(ROCR),
   ADD_TO_TABLE(RSI),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableRSize =
              ((sizeof(TA_DEF_TableR)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
