/* TA-LIB Copyright (c) 1999-2002, Mario Fortier
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
 *  010301 MF   First version.
 *
 */

/* Description:
 *   Utility functions describing the tokens used to parse
 *   strings containing TA related fields (like "[D][M][Y]").
 */

/**** Headers ****/
#include <stdlib.h>

#include "ta_token.h"
#include "ta_common.h"
#include "ta_trace.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

unsigned int TA_TokenMaxSize( TA_TokenId id )
{
   /* NOTE: The following data structure MUST correspond exactly to the
    *       TA_TokenId enumeration order!
    */

   /* Zero indicates that the size is not pre-determined. */

   static int tokenSize[] =
   {
      0, /* TA_TOK_FIX */

      4, /* TA_TOK_YYYY */
      2, /* TA_TOK_YY   */
      0, /* TA_TOK_Y    */

      0, /* TA_TOK_M   */
      2, /* TA_TOK_MM  */
      3, /* TA_TOK_MMM */

      0, /* TA_TOK_D */
      2, /* TA_TOK_DD */

      0, /* TA_TOK_CAT */
      0, /* TA_TOK_CATC */
      0, /* TA_TOK_CATX */
      0, /* TA_TOK_CATT */

      0, /* TA_TOK_SYM */
      0, /* TA_TOK_SYMF */
      1, /* TA_TOK_SEP */

      0, /* TA_TOK_WILD */
      1, /* TA_TOK_WILD_CHAR */

      0, /* TA_TOK_OPEN */
      0, /* TA_TOK_HIGH */
      0, /* TA_TOK_LOW */
      0, /* TA_TOK_CLOSE */
      0, /* TA_TOK_VOLUME */
      0, /* TA_TOK_OPENINTEREST */

      0, /* TA_TOK_HOUR */
      0, /* TA_TOK_MIN */
      0, /* TA_TOK_SEC */

      2, /* TA_TOK_HH */
      2, /* TA_TOK_MN */
      2, /* TA_TOK_SS */

      0,   /* TA_TOK_SKIP_REAL */
      0,   /* TA_TOK_SKIP_INTEGER */
      0,   /* TA_TOK_SKIP_N_CHAR */
      0,   /* TA_TOK_SKIP_N_HEADER_LINE */


      0, /* TA_END */
   };

   /* Note: 0 means unspecified number of character (wild) */

   if( (id == TA_INVALID_TOKEN_ID) || (id < 0) )
   {
      TA_FATAL_RET( NULL, id, TA_INVALID_TOKEN_ID, 0 );
   }

   return tokenSize[id];
}


const char *TA_TokenDebugString( TA_TokenId id )
{
   /* NOTE: The following data structure MUST correspond exactly to the
    *       TA_TokenId enumeration order!
    */
   static const char *tokenString[] =
   {
      "FIX",

      "YEAR",
      "YEAR",
      "YEAR",
      "MONTH",
      "MONTH",
      "MONTH",
      "DAY",
      "DAY",

      "CATEGORY",
      "COUNTRY",
      "EXCHANGE",
      "TYPE",
      "SYMBOL",

      "SYMBOL",
      "SEP",
      "WILD",
      "WILD CHAR",

      "OPEN",
      "HIGH",
      "LOW",
      "CLOSE",
      "VOLUME",
      "OPEN INTEREST",

      "HOUR",
      "MINUTE",
      "SECOND",

      "HOUR",
      "MINUTE",
      "SECOND",

      "SKIP REAL",
      "SKIP INTEGER",
      "SKIP CHAR",
      "SKIP HEADER_LINE",

      "END"
   };

   if( (id == TA_INVALID_TOKEN_ID) || (id < 0) )
      return (char *)NULL;

   return tokenString[id];
}

const char *TA_TokenString( TA_TokenId id )
{
   /* NOTE: The following data structure MUST correspond exactly to the
    *       TA_TokenId enumeration order!
    */
   static const char *tokenString[] =
   {
      NULL, /* TA_TOK_FIX */

      "YYYY",
      "YY",
      "Y",
      "M",
      "MM",
      "MMM",
      "D",
      "DD",

      "CAT",
      "CATC",
      "CATX",
      "CATT",
      "SYM",

      NULL, /* TA_TOK_SYMF */
      NULL, /* TA_TOK_SEP */
      NULL, /* TA_TOK_WILD */
      NULL, /* TA_TOK_WILDCHAR */

      "O",
      "H",
      "L",
      "C",
      "V",
      "OI",

      "HOUR",
      "MIN",
      "SEC",

      "HH",
      "MN",
      "SS",

      "-R", /* TA_TOK_SKIP_N_REAL */
      "-I", /* TA_TOK_SKIP_N_INTEGER */
      "-C", /* TA_TOK_SKIP_N_CHAR */
      "-H", /* TA_TOK_SKIP_N_HEADER_LINE */

      NULL  /* TA_TOK_END */
   };

   if( (id == TA_INVALID_TOKEN_ID) || (id < 0) )
      return (char *)NULL;

   return tokenString[id];
}

/**** Local functions definitions.     ****/
/* None */

