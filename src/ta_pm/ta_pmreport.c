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
 *   Allows to aggregate all the measurements within a 
 *   report.
 *
 *   This repot can be appended to a file with TA_PMReportToFile.
 *
 *   The user can also allocate this report in memory and do
 *   whatever they wish with it.
 */

/**** Headers ****/
#include <string.h>
#include <math.h>
#include "ta_pm.h"
#include "ta_pm_priv.h"
#include "ta_memory.h"
#include "ta_global.h"
#include "ta_magic_nb.h"
#include "ta_stream.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   unsigned int magicNb;
} TA_PMReportPriv;

/* Colums are large enough to display a float value
 * with a '$' or '%' symbol.
 */
#define TA_COLUMN_SIZE 16

#define COL0(line) (&theBuffer[((line)*lineSizeInByte)])
#define COL1(line) (&theBuffer[((line)*lineSizeInByte)+labelSizeInByte])
#define COL2(line) (&theBuffer[((line)*lineSizeInByte)+labelSizeInByte+TA_COLUMN_SIZE])
#define COL3(line) (&theBuffer[((line)*lineSizeInByte)+labelSizeInByte+TA_COLUMN_SIZE+TA_COLUMN_SIZE])

/**** Local functions declarations.    ****/
static unsigned int labelSize( void );
static TA_RetCode reportValueIdRange( TA_PM *pm, 
                                      char *theBuffer,
                                      int lineSizeInByte,
                                      int labelSizeInByte,
                                      unsigned int includeFlag,
                                      unsigned int excludeFlag, 
                                      int *callerCurLine );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_RetCode TA_PMReportAlloc( TA_PM *pm, TA_PMReport **newAllocatedReport )
{
   TA_PMReport      *newReport;
   TA_PMReportPriv  *newReportPriv;
   TA_PMPriv        *pmPriv;
   int               lineSizeInByte, labelSizeInByte, sizeInByte, nbLine, curLine;
   char             *theBuffer;
   int               i;

   /* Validate parameters. */
   if( !pm || !newAllocatedReport )
      return TA_BAD_PARAM;

   pmPriv = (TA_PMPriv *)pm->hiddenData;
   if( !pmPriv || (pmPriv->magicNb != TA_PMPRIV_MAGIC_NB) )
      return TA_BAD_PARAM;

   /* Allocate the report, including the hidden part. */
   newReport = TA_Malloc( sizeof( TA_PMReport ) + sizeof( TA_PMReportPriv ) );
   if( !newReport)
      return TA_ALLOC_ERR;
   memset( newReport, 0, sizeof( TA_PMReport ) + sizeof( TA_PMReportPriv ) );
   newReportPriv = (TA_PMReportPriv *)(((char *)newReport)+sizeof(TA_PMReport));
   newReportPriv->magicNb   = TA_PMREPORT_MAGIC_NB;
   newReport->hiddenData    = newReportPriv;

   /* TA_PMReportFree can be safely called from this point. */

   /* Format of the report is:
    * 
    * |---- labelSize ------|----- 1st Column -----|----- 2nd Column -----|----- 3rd Column -----|
    *
    * 10/22/2002 10:32:40
    *                                   Long Trades           Short Trades             All Trades
    * ===========================================================================================
    * <GENERAL MEASUREMENTS>               x.xx                   x.xx                   x.xx 
    * ------------------------------------------------------------------------------------------- 
    * <WINNING MEASUREMENTS>               x.xx                   x.xx                   x.xx 
    * ------------------------------------------------------------------------------------------- 
    * <LOSING MEASUREMENTS>                x.xx                   x.xx                   x.xx 
    * ------------------------------------------------------------------------------------------- 
    * <NOT RECOMMENDED MEASUREMENTS>       x.xx                   x.xx                   x.xx 
    * ------------------------------------------------------------------------------------------- 
    * 
    * END OF REPORT
    */

   /* Each line have all the same size determined here. 
    * Each line is NULL terminated.
    */
   labelSizeInByte =  labelSize();
   lineSizeInByte  =  labelSizeInByte + (3*TA_COLUMN_SIZE) + 1;

   /* Add some seperator or header lines. Add also a last line who
    * will starts with a NULL.
    */
   nbLine = (TA_PM_NB_VALUEID+11);
   sizeInByte = lineSizeInByte*nbLine;

   theBuffer = TA_Malloc( sizeInByte );

   if( !theBuffer )
   {
      TA_PMReportFree( newReport );
      return TA_ALLOC_ERR;
   }

   memset( theBuffer, ' ', sizeInByte );
   for( i=1; i <= nbLine; i++ )
      theBuffer[(i*lineSizeInByte)-1] = '\0';

   curLine = 0;

   /* First line is just a timestamp. */
   sprintf( COL0(curLine), "Date Range [%02d/%02d/%04d..%02d/%02d/%04d]",
                            TA_GetMonth(&pmPriv->startDate),
                            TA_GetDay(&pmPriv->startDate),
                            TA_GetYear(&pmPriv->startDate),
                            TA_GetMonth(&pmPriv->endDate),
                            TA_GetDay(&pmPriv->endDate),
                            TA_GetYear(&pmPriv->endDate) );
   curLine++;

   /* Header line */
   sprintf( COL1(curLine), "%*s", TA_COLUMN_SIZE, "Long Trades" );
   sprintf( COL2(curLine), "%*s", TA_COLUMN_SIZE, "Short Trades" );
   sprintf( COL3(curLine), "%*s", TA_COLUMN_SIZE, "All Trades" );
   curLine++;

   /* Add the '====' seperator. */
   for( i=0; i < (lineSizeInByte-1); i++ )
      COL0(curLine)[i] = '=';
   curLine++;

   /* Display general measurements, except losing, winning 
    * and not recomended measurements.
    */
   reportValueIdRange( pm, 
                       theBuffer,
                       lineSizeInByte,
                       labelSizeInByte,
                       TA_PM_VALUE_ID_GENERAL,
                       TA_PM_VALUE_ID_LOSING_RELATED  |
                       TA_PM_VALUE_ID_WINNING_RELATED |
                       TA_PM_VALUE_ID_NOT_RECOMMENDED,
                       &curLine );

   /* Add a '-----' seperator. */
   for( i=0; i < (lineSizeInByte-1); i++ )
      COL0(curLine)[i] = '-';
   curLine++;

   /* Display only measurements winning related.
    * Exclude the not recommended measurements.
    */
   reportValueIdRange( pm, 
                       theBuffer,
                       lineSizeInByte,
                       labelSizeInByte,
                       TA_PM_VALUE_ID_WINNING_RELATED,
                       TA_PM_VALUE_ID_GENERAL         |
                       TA_PM_VALUE_ID_LOSING_RELATED  |
                       TA_PM_VALUE_ID_NOT_RECOMMENDED,
                       &curLine );

   /* Add a '-----' seperator. */
   for( i=0; i < (lineSizeInByte-1); i++ )
      COL0(curLine)[i] = '-';
   curLine++;

   /* Display essential measurements, losing related.
    * Exclude the not recommended measurements.
    */
   reportValueIdRange( pm, 
                       theBuffer,
                       lineSizeInByte,
                       labelSizeInByte,
                       TA_PM_VALUE_ID_LOSING_RELATED,
                       TA_PM_VALUE_ID_GENERAL         |
                       TA_PM_VALUE_ID_WINNING_RELATED |
                       TA_PM_VALUE_ID_NOT_RECOMMENDED,
                       &curLine );

   /* Add a '-----' seperator. */
   for( i=0; i < (lineSizeInByte-1); i++ )
      COL0(curLine)[i] = '-';
   curLine++;

   /* Display the list of non-essential measurements */
   reportValueIdRange( pm, 
                       theBuffer,
                       lineSizeInByte,
                       labelSizeInByte,
                       TA_PM_VALUE_ID_NOT_RECOMMENDED,
                       0, /* Exclude none */
                       &curLine );
   
   /* Add the '====' seperator. */
   for( i=0; i < (lineSizeInByte-1); i++ )
      COL0(curLine)[i] = '=';
   curLine++;


   /* Footer line */
   sprintf( COL0(curLine), "%s", "End Of Report" );
   curLine++;

   /* All done, return the report to the caller. */
   newReport->buffer     = theBuffer;
   newReport->nbLine     = nbLine;
   newReport->lineLength = lineSizeInByte;
   *newAllocatedReport   = newReport;

   return TA_SUCCESS;
}

TA_RetCode TA_PMReportFree ( TA_PMReport *toBeFreed )
{
   TA_PMReportPriv  *reportPriv;

   if( toBeFreed )
   {
      reportPriv = (TA_PMReportPriv *)toBeFreed->hiddenData;
      if( reportPriv )
      {
         if( reportPriv->magicNb != TA_PMREPORT_MAGIC_NB )
            return TA_BAD_OBJECT;
         reportPriv->magicNb = 0;

         if( toBeFreed->buffer )
            TA_Free( (void *)toBeFreed->buffer );

         TA_Free( toBeFreed );
      }
   }

   return TA_SUCCESS;
}

TA_RetCode TA_PMReportToFile( TA_PM *pm, FILE *out )
{
   TA_PMReport *pmReport;
   unsigned int i;
   TA_RetCode retCode;

   retCode = TA_PMReportAlloc( pm, &pmReport );
   if( retCode != TA_SUCCESS )
      return retCode;

   for( i=0; i < pmReport->nbLine; i++ )
      fprintf( out, "%s\n", &pmReport->buffer[i*pmReport->lineLength] );

   TA_PMReportFree( pmReport );

   return TA_SUCCESS;
}


/**** Local functions definitions.     ****/

/* Return the minimum number of character needed
 * for the TA_PMValueId labels.
 */
static unsigned int labelSize( void )
{
   unsigned int  i, temp;
   const char   *label;
   unsigned int  largestLabelSize;

   largestLabelSize = 0;

   for( i=0; i < TA_PM_NB_VALUEID; i++ )
   {
      label = TA_PMValueIdString( i );
      if( label )
         temp = strlen( label );
      else
         temp = 0;

      if( temp > largestLabelSize )
         largestLabelSize = temp;
   }

   return largestLabelSize;
}

static TA_RetCode  reportValueIdRange( TA_PM *pm, 
                                       char *theBuffer,
                                       int lineSizeInByte,
                                       int labelSizeInByte,
                                       unsigned int includeFlag,
                                       unsigned int excludeFlag,
                                       int *callerCurLine )
{
   TA_PMValueId  theValueId;
   unsigned int  i;
   unsigned int  theFlags;
   char         *theSuffix;
   TA_Real       theValue;
   TA_Real       tempValue;
   TA_RetCode retCode;
   int curLine;

   curLine = *callerCurLine;

   for( theValueId=0; theValueId < TA_PM_NB_VALUEID; theValueId++ )
   {
      theFlags = TA_PMValueIdFlags( theValueId );

      /* Check if this measurements is excluded */
      if( excludeFlag && (theFlags & excludeFlag) )
         continue;

      /* Check if this measurements is included */
      if( !(theFlags & includeFlag) )
         continue;

      sprintf( COL0(curLine), "%-*s", labelSizeInByte,
                                      TA_PMValueIdString(theValueId) );

      for( i=0; i < 3; i++ )                       
      {
         retCode = TA_PMValue( pm, theValueId, i, &theValue );
         
         if( retCode != TA_SUCCESS )
         {
           sprintf( &COL1(curLine)[TA_COLUMN_SIZE*i], "%*s  ",
                    TA_COLUMN_SIZE-2, "N/A" );
         }
         else
         {
            if( theFlags & TA_PM_VALUE_ID_IS_PERCENT )
               theSuffix = "%";
            else if( theFlags & TA_PM_VALUE_ID_IS_CURRENCY )
               theSuffix = "$";
            else
               theSuffix = " ";

            if( theFlags & TA_PM_VALUE_ID_IS_INTEGER )
               sprintf( &COL1(curLine)[TA_COLUMN_SIZE*i], "%*d %s",
                        TA_COLUMN_SIZE-2, (int)theValue, theSuffix );
            else
            {
               tempValue = theValue >= 0.0? theValue : -theValue;
               if( ((tempValue <= 10000000.0) && (tempValue >= 0.0000001)) || tempValue == 0.0 )
                  sprintf( &COL1(curLine)[TA_COLUMN_SIZE*i], "%*.02f %s",
                           TA_COLUMN_SIZE-2, theValue, theSuffix );
               else
                  sprintf( &COL1(curLine)[TA_COLUMN_SIZE*i], "%*.02e %s",
                           TA_COLUMN_SIZE-2, theValue, theSuffix );
            }
         }
      }
      curLine++;
   }

   *callerCurLine = curLine;

   return TA_SUCCESS;
}

