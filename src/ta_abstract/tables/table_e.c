/*********************************************************************
 * This file contains only TA functions starting with the letter 'E' *
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

/* EMA BEGIN */
static const TA_InputParameterInfo    *TA_EMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_EMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_EMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( EMA,                        /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Exponential Moving Average", /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP|TA_FUNC_FLG_UNST_PER, /* flags */
              NULL                        /* analysis function */
             );
/* EMA END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableE[] =
{
   ADD_TO_TABLE(EMA),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableESize =
              ((sizeof(TA_DEF_TableE)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
