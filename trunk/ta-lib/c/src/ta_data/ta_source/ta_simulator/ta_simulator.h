#ifndef TA_SIMULATOR_H
#define TA_SIMULATOR_H

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

/* See ta_source.h and ta_source.c for more information about these
 * functions.
 */
TA_RetCode TA_SIMULATOR_InitializeSourceDriver(void);

TA_RetCode TA_SIMULATOR_ShutdownSourceDriver(void);

TA_RetCode TA_SIMULATOR_GetParameters( TA_DataSourceParameters *param );

TA_RetCode TA_SIMULATOR_OpenSource( const TA_AddDataSourceParamPriv *param,
                                    TA_DataSourceHandle **handle );

TA_RetCode TA_SIMULATOR_CloseSource( TA_DataSourceHandle *handle );


TA_RetCode TA_SIMULATOR_GetFirstCategoryHandle( TA_DataSourceHandle *handle,
                                                TA_CategoryHandle   *categoryHandle );

TA_RetCode TA_SIMULATOR_GetNextCategoryHandle( TA_DataSourceHandle *handle,
                                               TA_CategoryHandle   *categoryHandle,\
                                               unsigned int index );

TA_RetCode TA_SIMULATOR_GetFirstSymbolHandle( TA_DataSourceHandle *handle,
                                              TA_CategoryHandle   *categoryHandle,
                                              TA_SymbolHandle     *symbolHandle );

TA_RetCode TA_SIMULATOR_GetNextSymbolHandle( TA_DataSourceHandle *handle,
                                             TA_CategoryHandle   *categoryHandle,
                                             TA_SymbolHandle     *symbolHandle,
                                             unsigned int index );

TA_RetCode TA_SIMULATOR_GetHistoryData( TA_DataSourceHandle *handle,
                                    TA_CategoryHandle   *categoryHandle,
                                    TA_SymbolHandle     *symbolHandle,
                                    TA_Period            period,
                                    const TA_Timestamp  *start,
                                    const TA_Timestamp  *end,
                                    TA_Field             fieldToAlloc,
                                    TA_ParamForAddData  *paramForAddData );
#endif

