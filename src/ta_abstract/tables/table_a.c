/*********************************************************************
 * This file contains only TA functions starting with the letter 'A' *
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

/* ATR BEGIN */
static const TA_InputParameterInfo    *TA_ATR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ATR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ATR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( ATR,                        /* name */
              TA_GroupId_VolatilityIndicators, /* groupId */
              "Average True Range",       /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER,       /* flags */
              NULL                        /* analysis function */
             );
/* ATR END */

/* APO BEGIN */
static const TA_InputParameterInfo *TA_APO_Inputs[] =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_APO_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_APO_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_MA_Method,
  &TA_DEF_UI_Compatibility_CL_MS,
  NULL
};

DEF_FUNCTION( APO,                         /* name */
              TA_GroupId_TrendIndicators,  /* groupId */
              "Absolute Price Oscillator", /* hint */
              NULL,                        /* helpFile */
              0,                           /* flags */
              NULL                         /* analysis function */
             );
/* APO END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableA[] =
{
   ADD_TO_TABLE(APO),
   ADD_TO_TABLE(ATR),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableASize =
              ((sizeof(TA_DEF_TableA)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
