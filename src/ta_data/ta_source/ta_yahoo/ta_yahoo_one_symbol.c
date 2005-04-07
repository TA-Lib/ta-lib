/* TA-LIB Copyright (c) 1999-2005, Mario Fortier
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
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  040205 MF   Initial coding.
 */

/* Description:
 *    Allows to define how to access the Yahoo! web site data.
 */
#include "ta_yahoo_priv.h"

TA_DirectYahooDecoding tableDirectYahooDecoding[] =
{  
   /* Define decoding for the United State Yahoo! web site. */
   {  
      /* Identify the country. */  
      TA_Country_ID_US,   

      /* String to build the URL to get market data. */
      /* Server */ "ichart.yahoo.com", 
      /* Prefix */ "/table.csv?s=",
      /* Suffix */ "&a=1&b=1&c=1950&d=1&e=1&f=3000&g=d&q=q&y=0&z=file&x=.csv",

      /* String to build the URL to get dividend/split adjustment. */
      /* Server */ "finance.yahoo.com",
      /* Prefix */ "/q/hp?s=",
      /* Suffix */ "&a=1&b=1&c=1950&d=1&e=1&f=3000&g=m"
   },

   /* Define decoding for the France Yahoo! web site. */
   {  
      /* Identify the country. */  
      TA_Country_ID_FR,

      /* String to build the URL to get market data. */
      /* Server */ "ichart.yahoo.com", 
      /* Prefix */ "/table.csv?s=",
      /* Suffix */ "&a=1&b=1&c=1950&d=1&e=1&f=3000&g=d&q=q&y=0&z=file&x=.csv",

      /* String to build the URL to get dividend/split adjustment. */
      /* Server */ "finance.yahoo.com",
      /* Prefix */ "/q/hp?s=",
      /* Suffix */ "&a=1&b=1&c=1950&d=1&e=1&f=3000&g=m"
   }
};

/* Do not change the following. */
unsigned int tableDirectYahooDecodingSize =
  sizeof(tableDirectYahooDecoding)/sizeof(TA_DirectYahooDecoding);
