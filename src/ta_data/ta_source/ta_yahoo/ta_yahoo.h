#ifndef TA_YAHOO_H
#define TA_YAHOO_H

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

/* See ta_source.h and ta_source.c for more information about these
 * functions.
 */
TA_RetCode TA_YAHOO_InitializeSourceDriver( void );

TA_RetCode TA_YAHOO_ShutdownSourceDriver( void );

TA_RetCode TA_YAHOO_GetParameters( TA_DataSourceParameters *param );

TA_RetCode TA_YAHOO_OpenSource( const TA_AddDataSourceParamPriv *param,
                                TA_DataSourceHandle **handle );

TA_RetCode TA_YAHOO_CloseSource( TA_DataSourceHandle *handle );


TA_RetCode TA_YAHOO_GetFirstCategoryHandle( TA_DataSourceHandle *handle,
                                            TA_CategoryHandle   *categoryHandle );

TA_RetCode TA_YAHOO_GetNextCategoryHandle(  TA_DataSourceHandle *handle,
                                            TA_CategoryHandle   *categoryHandle,
                                            unsigned int index );

TA_RetCode TA_YAHOO_GetFirstSymbolHandle(   TA_DataSourceHandle *handle,
                                            TA_CategoryHandle   *categoryHandle,
                                            TA_SymbolHandle     *symbolHandle );

TA_RetCode TA_YAHOO_GetNextSymbolHandle(    TA_DataSourceHandle *handle,
                                            TA_CategoryHandle   *categoryHandle,
                                            TA_SymbolHandle     *symbolHandle,
                                            unsigned int index );

TA_RetCode TA_YAHOO_GetHistoryData( TA_DataSourceHandle *handle,
                                    TA_CategoryHandle   *categoryHandle,
                                    TA_SymbolHandle     *symbolHandle,
                                    TA_Period            period,
                                    const TA_Timestamp  *start,
                                    const TA_Timestamp  *end,
                                    TA_Field             fieldToAlloc,
                                    TA_ParamForAddData  *paramForAddData );
#endif

