/*********************************************************************
 * This file contains only TA functions starting with the letter 'V' *
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

/* VAR BEGIN */
static const TA_InputParameterInfo    *TA_VAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_VAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_VAR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_5,
  &TA_DEF_UI_NbDeviation,
  NULL
};

DEF_FUNCTION( VAR,                      /* name */
              TA_GroupId_Statistic,     /* groupId */
              "Variance",               /* hint */
              NULL,                     /* helpFile */
              0,                        /* flags */
              NULL                      /* analysis function */
             );
/* VAR END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableV[] =
{
   ADD_TO_TABLE(VAR),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableVSize =
              ((sizeof(TA_DEF_TableV)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
