/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
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

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  061902 MF   First version.
 *
 */

/* Description:
 *   Return strings for a given TA_PMValueId
 *
 *   These strings are aim to be used in
 *   user interfaces.
 */

/**** Headers ****/
#include "ta_pm.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_PMValueId  id;
   const char   *idString;
   const char   *hintString;
   unsigned int  flags;
} TA_PMStrings;

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/

static TA_PMStrings pmStringTable[] =
{
   { TA_PM_TOTAL_NB_OF_TRADE,
     "Number Of Trades",
     "Number of completed trades within the evaluated period",
     TA_PM_VALUE_ID_GENERAL |
     TA_PM_VALUE_ID_IS_INTEGER
   },

   { TA_PM_STARTING_CAPITAL,
     "Starting Capital",
     "Capital at the beginning of the measured period.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED
   },

   { TA_PM_ENDING_CAPITAL,
     "Ending Capital",
     "Capital at the end of the measured period.",
     TA_PM_VALUE_ID_IS_CURRENCY      |
     TA_PM_VALUE_ID_NOT_RECOMMENDED
   },   

   { TA_PM_TOTAL_NET_PROFIT,
     "Total Net Profit",
     "Profit minus loss of all trades.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED |
     TA_PM_VALUE_ID_WINNING_RELATED
   },

   { TA_PM_PROFIT_FACTOR,
     "Profit Factor",
     "Gross profit divided by gross loss.",
     TA_PM_VALUE_ID_GENERAL
   },

   { TA_PM_PERCENT_PROFITABLE,
     "Profitable (%)",
     "Nb winning trade divided by number of trades.",
     TA_PM_VALUE_ID_GENERAL |
     TA_PM_VALUE_ID_IS_PERCENT
   },

   { TA_PM_RATE_OF_RETURN,
     "Rate Of Return",
     "One-period rate of return:\n"
     "  (profit / initialCapital)\n",
     TA_PM_VALUE_ID_IS_PERCENT       |
     TA_PM_VALUE_ID_NOT_RECOMMENDED
   },
   
   { TA_PM_ANNUALIZED_RETURN,
     "Annualized Return",
     "Annualized rate of return on a simple-interest basis:\n"
     "\n"
     "   (Ending Value - Starting Value)     365\n"
     "   -------------------------------  *  ---\n"
     "           Starting Value               n\n"
     "\n"
     " Where 'n' is the number of day between the end and\n"
     " start date when the TA_PM was created.\n",
     TA_PM_VALUE_ID_IS_PERCENT       |
     TA_PM_VALUE_ID_NOT_RECOMMENDED
   },
   

   { TA_PM_ANNUALIZED_COMPOUNDED_RETURN,
     "Annualized Compounded Return",
     "Annualized compounded rate of return:\n"
     "\n"
     " ((Ending Value / Starting Value)^(1/y)) - 1\n"
     "\n"
     " Where 'y' is the number of year between the end\n"
     " and start date when the TA_PM was created.\n",
     TA_PM_VALUE_ID_IS_PERCENT       |
     TA_PM_VALUE_ID_NOT_RECOMMENDED
   },

   { TA_PM_NB_WINNING_TRADE,
     "Nb Winning Trade",
     "Number of completed trades resulting in a profit",
     TA_PM_VALUE_ID_IS_INTEGER |
     TA_PM_VALUE_ID_WINNING_RELATED
   },

   { TA_PM_NB_LOSING_TRADE,
     "Nb Losing Trade",
     "Number of completed trades resulting in a loss",
     TA_PM_VALUE_ID_IS_INTEGER |
     TA_PM_VALUE_ID_LOSING_RELATED
   },

   { TA_PM_GROSS_PROFIT,
     "Gross Profit",
     "Summation of the profit of all winning trades.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED |
     TA_PM_VALUE_ID_WINNING_RELATED
   },

   { TA_PM_GROSS_LOSS,
     "Gross Loss",
     "Summation of all losing trades.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED |
     TA_PM_VALUE_ID_LOSING_RELATED
   },

   { TA_PM_LARGEST_PROFIT, 
     "Largest Net Profit",
     "Trade with the largest net profit.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED |
     TA_PM_VALUE_ID_WINNING_RELATED
   },

   { TA_PM_LARGEST_LOSS,
     "Largest Net Loss",
     "Trade with the largest net loss.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED |
     TA_PM_VALUE_ID_LOSING_RELATED
   },

   { TA_PM_LARGEST_PROFIT_PERCENT,
     "Largest Profit (%)",
     "Trade with the largest profit.",
     TA_PM_VALUE_ID_IS_PERCENT |
     TA_PM_VALUE_ID_WINNING_RELATED
   },

   { TA_PM_LARGEST_LOSS_PERCENT,
     "Largest Loss (%)",
     "Trade with the largest loss.",
     TA_PM_VALUE_ID_IS_PERCENT |
     TA_PM_VALUE_ID_LOSING_RELATED
   },
   
   { TA_PM_AVG_LOSS,
     "Average Loss",
     "Average net loss of all losing trade.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED |
     TA_PM_VALUE_ID_LOSING_RELATED
   },

   { TA_PM_AVG_PROFIT,
     "Average Profit",
     "Average net profit of all winning trade.",
     TA_PM_VALUE_ID_IS_CURRENCY     |
     TA_PM_VALUE_ID_NOT_RECOMMENDED |
     TA_PM_VALUE_ID_WINNING_RELATED
   },

   { TA_PM_AVG_LOSS_PERCENT,
     "Average Loss (%)",
     "Average Loss",
     TA_PM_VALUE_ID_IS_PERCENT |
     TA_PM_VALUE_ID_LOSING_RELATED
   },


   { TA_PM_AVG_PROFIT_PERCENT,
     "Average Profit (%)",
     "Average Profit",
     TA_PM_VALUE_ID_IS_PERCENT |
     TA_PM_VALUE_ID_WINNING_RELATED
   }     

};

#define NB_VALUEID_STRING (sizeof(pmStringTable)/sizeof(TA_PMStrings))
 
/**** Global functions definitions.   ****/
const char *TA_PMValueIdString( TA_PMValueId valueId )
{
   unsigned int i;

   /* Sequentially search for the valueId and
    * simply return the corresponding string.
    */
   for( i=0; i < NB_VALUEID_STRING; i++ )
   {
      if( pmStringTable[i].id == valueId )
         return pmStringTable[i].idString;
   }

   return TA_PMSTRING_ERROR;
}

const char *TA_PMValueIdHint( TA_PMValueId valueId )
{
   unsigned int i;

   /* Sequentially search for the valueId and
    * simply return the corresponding string.
    */
   for( i=0; i < NB_VALUEID_STRING; i++ )
   {
      if( pmStringTable[i].id == valueId )
         return pmStringTable[i].hintString;
   }

   return TA_PMSTRING_ERROR;
}

unsigned int TA_PMValueIdFlags( TA_PMValueId valueId )
{
   unsigned int i;

   /* Sequentially search for the valueId and
    * simply return the corresponding string.
    */
   for( i=0; i < NB_VALUEID_STRING; i++ )
   {
      if( pmStringTable[i].id == valueId )
         return pmStringTable[i].flags;
   }

   return 0;
}


/**** Local functions definitions.     ****/
/* None */
