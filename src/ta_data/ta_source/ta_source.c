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
 *  110199 MF   First version.
 *
 */

/* Description:
 *    This file helps to make abstraction of the different data source.
 *
 *    It defines basically two global variable:
 *
 *    TA_gDataSourceTable
 *       This table provides all the functions needed for accessing
 *       all the different data source format.
 *       For each element of the TA_SourceId enumeration, there
 *       is one entry in the gDataSourceTable.
 *
 *    TA_gDataSourceTableSize
 *       The number of entry in the TA_gDataSourceTable.
 *       Take note that the number of entry MUST correspond
 *       to the number of entry in the TA_SourceId enumeration.
 *
 */

/**** Headers ****/
#include "ta_source.h"
#include "ta_ascii.h"
#include "ta_simulator.h"
#include "ta_yahoo.h"
#if defined(TA_SUPPORT_MYSQL)
#include "ta_mysql.h"
#endif

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
const TA_DataSourceDriver TA_gDataSourceTable[] =
{
    {   /* TA_ASCII_FILE format. */
        TA_ASCII_InitializeSourceDriver,
        TA_ASCII_ShutdownSourceDriver,
        TA_ASCII_GetParameters,
        TA_ASCII_OpenSource,
        TA_ASCII_CloseSource,
        TA_ASCII_GetFirstCategoryHandle,
        TA_ASCII_GetNextCategoryHandle,
        TA_ASCII_GetFirstSymbolHandle,
        TA_ASCII_GetNextSymbolHandle,
        TA_ASCII_GetHistoryData
    },

    {   /* TA_SIMULATOR data source. */
        TA_SIMULATOR_InitializeSourceDriver,
        TA_SIMULATOR_ShutdownSourceDriver,
        TA_SIMULATOR_GetParameters,
        TA_SIMULATOR_OpenSource,
        TA_SIMULATOR_CloseSource,
        TA_SIMULATOR_GetFirstCategoryHandle,
        TA_SIMULATOR_GetNextCategoryHandle,
        TA_SIMULATOR_GetFirstSymbolHandle,
        TA_SIMULATOR_GetNextSymbolHandle,
        TA_SIMULATOR_GetHistoryData
    },

    {   /* TA_YAHOO_WEB data source. */
        TA_YAHOO_InitializeSourceDriver,
        TA_YAHOO_ShutdownSourceDriver,
        TA_YAHOO_GetParameters,
        TA_YAHOO_OpenSource,
        TA_YAHOO_CloseSource,
        TA_YAHOO_GetFirstCategoryHandle,
        TA_YAHOO_GetNextCategoryHandle,
        TA_YAHOO_GetFirstSymbolHandle,
        TA_YAHOO_GetNextSymbolHandle,
        TA_YAHOO_GetHistoryData
    },

    {   /* TA_SQL data source. */
#if defined(TA_SUPPORT_MYSQL)
        TA_MYSQL_InitializeSourceDriver,
        TA_MYSQL_ShutdownSourceDriver,
        TA_MYSQL_GetParameters,
        TA_MYSQL_OpenSource,
        TA_MYSQL_CloseSource,
        TA_MYSQL_GetFirstCategoryHandle,
        TA_MYSQL_GetNextCategoryHandle,
        TA_MYSQL_GetFirstSymbolHandle,
        TA_MYSQL_GetNextSymbolHandle,
        TA_MYSQL_GetHistoryData
#else
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
#endif
    }

#if 0
    {   /* TA_CSI format. */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },

    {   /* TA_COMP format. */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
#endif

};

const unsigned int TA_gDataSourceTableSize = (sizeof(TA_gDataSourceTable)/sizeof(TA_DataSourceDriver));

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
/* None */

/**** Local functions definitions.     ****/
/* None */



