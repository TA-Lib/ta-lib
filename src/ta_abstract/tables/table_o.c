/*********************************************************************
 * This file contains only TA functions starting with the letter 'O' *
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

/* OBV BEGIN */
static const TA_InputParameterInfo    *TA_OBV_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  &TA_DEF_UI_Input_Price_V,
  NULL
};

static const TA_OutputParameterInfo   *TA_OBV_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_OBV_OptInputs[] =
{ NULL };

DEF_FUNCTION( OBV,                          /* name */
              TA_GroupId_VolumeIndicators,  /* groupId */
              "On Balance Volume",          /* hint */
              NULL,                         /* helpFile */
              0,                            /* flags */
              NULL                          /* analysis function */
             );
/* OBV END */

/****************************************************************************
 * Step 2 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* None */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableO[] =
{
   ADD_TO_TABLE(OBV),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableOSize =
              ((sizeof(TA_DEF_TableO)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
