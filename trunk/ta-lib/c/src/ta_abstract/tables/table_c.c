/*********************************************************************
 * This file contains only TA functions starting with the letter 'C' *
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

/* CCI BEGIN */
static const TA_InputParameterInfo    *TA_CCI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CCI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_CCI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM5,
  NULL
};

DEF_FUNCTION( CCI,                           /* name */
              TA_GroupId_MomentumIndicators, /* groupId */
              "Commodity Channel Index",     /* hint */
              NULL,                          /* helpFile */
              0,                             /* flags */
              NULL                           /* analysis function */
             );

/* CCI END */


/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableC[] =
{
   ADD_TO_TABLE(CCI),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableCSize =
              ((sizeof(TA_DEF_TableC)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
